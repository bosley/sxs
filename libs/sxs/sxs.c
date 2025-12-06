#include "sxs/sxs.h"
#include "map/map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUILTIN_REGISTRY_INITIAL_CAPACITY 16

extern slp_callbacks_t *sxs_runtime_get_callbacks(sxs_runtime_t *runtime);

sxs_builtin_registry_t *sxs_builtin_registry_create(size_t initial_capacity) {
  (void)initial_capacity;

  sxs_builtin_registry_t *registry = malloc(sizeof(sxs_builtin_registry_t));
  if (!registry) {
    fprintf(stderr, "Failed to allocate builtin registry\n");
    return NULL;
  }

  map_init(&registry->command_map);

  return registry;
}

void sxs_builtin_registry_free(sxs_builtin_registry_t *registry) {
  if (!registry) {
    return;
  }

  map_iter_t iter = map_iter(&registry->command_map);
  const char *key;
  while ((key = map_next(&registry->command_map, &iter))) {
    void **value = map_get(&registry->command_map, key);
    if (value && *value) {
      free(*value);
    }
  }

  map_deinit(&registry->command_map);

  free(registry);
}

int sxs_builtin_registry_add(sxs_builtin_registry_t *registry,
                             sxs_command_impl_t impl) {
  if (!registry) {
    fprintf(stderr, "Failed to add to builtin registry (nil registry)\n");
    return 1;
  }

  if (!impl.command) {
    fprintf(stderr, "Failed to add to builtin registry (nil command)\n");
    return 1;
  }

  void **existing = map_get(&registry->command_map, impl.command);
  if (existing && *existing) {
    free(*existing);
  }

  sxs_command_impl_t *cmd = malloc(sizeof(sxs_command_impl_t));
  if (!cmd) {
    fprintf(stderr, "Failed to allocate command impl\n");
    return 1;
  }

  *cmd = impl;

  if (map_set(&registry->command_map, impl.command, cmd) != 0) {
    fprintf(stderr, "Failed to add command to map: %s\n", impl.command);
    free(cmd);
    return 1;
  }

  return 0;
}

sxs_command_impl_t *
sxs_builtin_registry_lookup(sxs_builtin_registry_t *registry,
                            slp_buffer_t *symbol) {
  if (!registry || !symbol || !symbol->data) {
    return NULL;
  }

  char *key = malloc(symbol->count + 1);
  if (!key) {
    return NULL;
  }
  memcpy(key, symbol->data, symbol->count);
  key[symbol->count] = '\0';

  void **result = map_get(&registry->command_map, key);
  free(key);

  if (!result) {
    return NULL;
  }

  return (sxs_command_impl_t *)*result;
}

static void *sxs_callable_copy_impl(void *fn_data) {
  if (!fn_data) {
    return NULL;
  }

  sxs_callable_t *original = (sxs_callable_t *)fn_data;
  sxs_callable_t *cloned = malloc(sizeof(sxs_callable_t));
  if (!cloned) {
    return NULL;
  }

  cloned->is_builtin = original->is_builtin;
  cloned->variant_count = original->variant_count;

  for (size_t v = 0; v < original->variant_count; v++) {
    cloned->variants[v].param_count = original->variants[v].param_count;
    cloned->variants[v].return_type = original->variants[v].return_type;

    if (original->variants[v].params && original->variants[v].param_count > 0) {
      cloned->variants[v].params = malloc(sizeof(sxs_callable_param_t) *
                                          original->variants[v].param_count);
      if (!cloned->variants[v].params) {
        for (size_t pv = 0; pv < v; pv++) {
          if (cloned->variants[pv].params) {
            for (size_t i = 0; i < cloned->variants[pv].param_count; i++) {
              if (cloned->variants[pv].params[i].name) {
                free(cloned->variants[pv].params[i].name);
              }
            }
            free(cloned->variants[pv].params);
          }
        }
        free(cloned);
        return NULL;
      }

      for (size_t i = 0; i < original->variants[v].param_count; i++) {
        if (original->variants[v].params[i].name) {
          cloned->variants[v].params[i].name =
              malloc(strlen(original->variants[v].params[i].name) + 1);
          if (!cloned->variants[v].params[i].name) {
            for (size_t j = 0; j < i; j++) {
              if (cloned->variants[v].params[j].name) {
                free(cloned->variants[v].params[j].name);
              }
            }
            free(cloned->variants[v].params);
            for (size_t pv = 0; pv < v; pv++) {
              if (cloned->variants[pv].params) {
                for (size_t pi = 0; pi < cloned->variants[pv].param_count;
                     pi++) {
                  if (cloned->variants[pv].params[pi].name) {
                    free(cloned->variants[pv].params[pi].name);
                  }
                }
                free(cloned->variants[pv].params);
              }
            }
            free(cloned);
            return NULL;
          }
          strcpy(cloned->variants[v].params[i].name,
                 original->variants[v].params[i].name);
        } else {
          cloned->variants[v].params[i].name = NULL;
        }
        cloned->variants[v].params[i].form =
            original->variants[v].params[i].form;
      }
    } else {
      cloned->variants[v].params = NULL;
    }
  }

  if (original->impl.lambda_body) {
    cloned->impl.lambda_body = slp_object_copy(original->impl.lambda_body);
    if (!cloned->impl.lambda_body) {
      for (size_t v = 0; v < cloned->variant_count; v++) {
        if (cloned->variants[v].params) {
          for (size_t i = 0; i < cloned->variants[v].param_count; i++) {
            if (cloned->variants[v].params[i].name) {
              free(cloned->variants[v].params[i].name);
            }
          }
          free(cloned->variants[v].params);
        }
      }
      free(cloned);
      return NULL;
    }
  } else {
    cloned->impl.lambda_body = NULL;
  }

  return cloned;
}

static void sxs_callable_free_impl(void *fn_data) {
  sxs_callable_free((sxs_callable_t *)fn_data);
}

static bool sxs_callable_equal_impl(void *fn_data_a, void *fn_data_b) {
  if (!fn_data_a && !fn_data_b) {
    return true;
  }
  if (!fn_data_a || !fn_data_b) {
    return false;
  }
  sxs_callable_t *a_callable = (sxs_callable_t *)fn_data_a;
  sxs_callable_t *b_callable = (sxs_callable_t *)fn_data_b;

  if (!a_callable->impl.lambda_body && !b_callable->impl.lambda_body) {
    return true;
  }
  if (!a_callable->impl.lambda_body || !b_callable->impl.lambda_body) {
    return false;
  }
  return slp_objects_equal(a_callable->impl.lambda_body,
                           b_callable->impl.lambda_body);
}

slp_object_t *sxs_create_error_object(slp_error_type_e error_type,
                                      const char *message, size_t position,
                                      slp_buffer_unowned_ptr_t source_buffer) {
  slp_object_t *error_obj = malloc(sizeof(slp_object_t));
  if (!error_obj) {
    fprintf(stderr, "Failed to allocate error object\n");
    return NULL;
  }

  slp_error_data_t *error_data = malloc(sizeof(slp_error_data_t));
  if (!error_data) {
    fprintf(stderr, "Failed to allocate error data\n");
    free(error_obj);
    return NULL;
  }

  error_data->position = position;
  error_data->error_type = error_type;

  if (message) {
    error_data->message = malloc(strlen(message) + 1);
    if (!error_data->message) {
      fprintf(stderr, "Failed to allocate error message\n");
      free(error_data);
      free(error_obj);
      return NULL;
    }
    strcpy(error_data->message, message);
  } else {
    error_data->message = NULL;
  }

  if (source_buffer) {
    error_data->source_buffer = slp_buffer_copy(source_buffer);
  } else {
    error_data->source_buffer = NULL;
  }

  error_obj->type = SLP_TYPE_ERROR;
  error_obj->value.fn_data = error_data;

  return error_obj;
}

sxs_context_t *sxs_context_new(size_t context_id, sxs_context_t *parent) {
  sxs_context_t *context = malloc(sizeof(sxs_context_t));
  if (!context) {
    fprintf(stderr, "Failed to create context (nil)\n");
    return NULL;
  }

  context->context_id = context_id;
  context->parent = parent;
  context->proc_list_count = 0;

  for (size_t i = 0; i < SXS_OBJECT_PROC_LIST_SIZE; i++) {
    context->object_proc_list[i] = NULL;
  }

  return context;
}

void sxs_context_free(sxs_context_t *context) {
  if (!context) {
    fprintf(stderr, "Failed to free sxs context (nil)\n");
    return;
  }

  for (size_t i = 0; i < context->proc_list_count; i++) {
    if (context->object_proc_list[i]) {
      slp_object_free(context->object_proc_list[i]);
    }
  }

  free(context);
}

sxs_runtime_t *sxs_runtime_new(sxs_builtin_registry_t *registry) {
  slp_register_builtin_handlers(NULL, NULL);
  slp_register_lambda_handlers(sxs_callable_free_impl, sxs_callable_copy_impl,
                               sxs_callable_equal_impl);

  sxs_runtime_t *runtime = malloc(sizeof(sxs_runtime_t));
  if (!runtime) {
    fprintf(stderr, "Failed to create runtime (nil)\n");
    return NULL;
  }

  runtime->current_context = sxs_context_new(0, NULL);
  if (!runtime->current_context) {
    free(runtime);
    return NULL;
  }

  runtime->next_context_id = 1;
  runtime->runtime_has_error = false;
  runtime->exception_active = false;
  runtime->parsing_quoted_expression = false;
  runtime->source_buffer = NULL;
  runtime->builtin_registry = registry;

  runtime->symbols = ctx_create(NULL);
  if (!runtime->symbols) {
    sxs_context_free(runtime->current_context);
    free(runtime);
    return NULL;
  }

  for (size_t i = 0; i < SXS_OBJECT_STORAGE_SIZE; i++) {
    runtime->object_storage[i] = NULL;
  }

  return runtime;
}

void sxs_runtime_free(sxs_runtime_t *runtime) {
  if (!runtime) {
    return;
  }

  if (runtime->current_context) {
    sxs_context_free(runtime->current_context);
  }

  if (runtime->source_buffer) {
    slp_buffer_free(runtime->source_buffer);
  }

  if (runtime->builtin_registry) {
    sxs_builtin_registry_free(runtime->builtin_registry);
  }

  if (runtime->symbols) {
    ctx_free(runtime->symbols);
  }

  for (size_t i = 0; i < SXS_OBJECT_STORAGE_SIZE; i++) {
    if (runtime->object_storage[i]) {
      slp_object_free(runtime->object_storage[i]);
    }
  }

  free(runtime);
}

int sxs_runtime_process_file(sxs_runtime_t *runtime, char *file_name) {
  if (!runtime) {
    fprintf(stderr, "Failed to process file (nil runtime)\n");
    return 1;
  }

  printf("[DEBUG] Processing file: %s\n", file_name);

  slp_buffer_t *buffer = slp_buffer_from_file(file_name);
  if (!buffer) {
    fprintf(stderr, "Failed to load file: %s\n", file_name);
    return 1;
  }

  runtime->source_buffer = buffer;

  slp_callbacks_t *callbacks = sxs_runtime_get_callbacks(runtime);
  if (!callbacks) {
    fprintf(stderr, "Failed to get callbacks\n");
    return 1;
  }

  return slp_process_buffer(buffer, callbacks);
}

slp_object_t *sxs_runtime_get_last_eval_obj(sxs_runtime_t *runtime) {
  if (!runtime) {
    fprintf(stderr, "Failed to get last eval obj (nil runtime)\n");
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
    }
    return none;
  }

  if (!runtime->current_context) {
    fprintf(stderr, "Failed to get last eval obj (nil current context)\n");
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
    }
    return none;
  }

  if (runtime->current_context->proc_list_count == 0) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
    }
    return none;
  }

  slp_object_t *last_object =
      runtime->current_context
          ->object_proc_list[runtime->current_context->proc_list_count - 1];

  if (!last_object) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
    }
    return none;
  }

  slp_object_t *result = slp_object_copy(last_object);

  if (!result) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
    }
    return none;
  }

  return result;
}

void sxs_callable_free(sxs_callable_t *callable) {
  if (!callable) {
    return;
  }

  for (size_t v = 0; v < callable->variant_count; v++) {
    if (callable->variants[v].params) {
      for (size_t i = 0; i < callable->variants[v].param_count; i++) {
        if (callable->variants[v].params[i].name) {
          free(callable->variants[v].params[i].name);
        }
        if (callable->variants[v].params[i].form) {
          if (callable->variants[v].params[i].form->name) {
            free(callable->variants[v].params[i].form->name);
          }
          if (callable->variants[v].params[i].form->types) {
            free(callable->variants[v].params[i].form->types);
          }
          free(callable->variants[v].params[i].form);
        }
      }
      free(callable->variants[v].params);
    }
    if (callable->variants[v].return_type) {
      if (callable->variants[v].return_type->name) {
        free(callable->variants[v].return_type->name);
      }
      if (callable->variants[v].return_type->types) {
        free(callable->variants[v].return_type->types);
      }
      free(callable->variants[v].return_type);
    }
  }

  if (!callable->is_builtin && callable->impl.lambda_body) {
    slp_object_free(callable->impl.lambda_body);
  }

  free(callable);
}
