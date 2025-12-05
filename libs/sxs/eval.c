#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern slp_callbacks_t *sxs_runtime_get_callbacks(sxs_runtime_t *runtime);

static slp_object_t *sxs_create_error_object(slp_error_type_e error_type,
                                             const char *message,
                                             size_t position) {
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

  error_obj->type = SLP_TYPE_ERROR;
  error_obj->value.fn_data = error_data;

  return error_obj;
}

static slp_object_t *sxs_eval_list(sxs_runtime_t *runtime, slp_object_t *list) {
  if (!list || list->type != SLP_TYPE_LIST_P) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "invalid list type for evaluation", 0);
  }

  if (list->value.list.count == 0) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "empty list evaluation", 0);
  }

  slp_object_t *first = list->value.list.items[0];
  if (!first) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "nil first item in list", 0);
  }

  slp_object_t **args = NULL;
  size_t arg_count = 0;
  if (list->value.list.count > 1) {
    arg_count = list->value.list.count - 1;
    args = &list->value.list.items[1];
  }

  if (first->type == SLP_TYPE_BUILTIN) {
    sxs_callable_t *callable = (sxs_callable_t *)first->value.fn_data;
    if (!callable) {
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "nil builtin callable", 0);
    }
    if (!callable->impl.builtin_fn) {
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "nil builtin function pointer", 0);
    }
    return callable->impl.builtin_fn(runtime, args, arg_count);
  }

  if (first->type == SLP_TYPE_LAMBDA) {
    sxs_callable_t *callable = (sxs_callable_t *)first->value.fn_data;
    if (!callable) {
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "nil lambda callable", 0);
    }
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "lambda evaluation not yet implemented", 0);
  }

  if (first->type == SLP_TYPE_SYMBOL) {
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "unknown function: ");
    size_t prefix_len = strlen(error_msg);
    if (first->value.buffer && first->value.buffer->count > 0) {
      size_t copy_len = first->value.buffer->count;
      if (prefix_len + copy_len >= sizeof(error_msg)) {
        copy_len = sizeof(error_msg) - prefix_len - 1;
      }
      memcpy(error_msg + prefix_len, first->value.buffer->data, copy_len);
      error_msg[prefix_len + copy_len] = '\0';
    }
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN, error_msg, 0);
  }

  return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN, "invalid function type",
                                 0);
}

slp_object_t *sxs_eval_object(sxs_runtime_t *runtime, slp_object_t *object) {
  if (!object) {
    fprintf(stderr, "Failed to eval object (nil)\n");
    return NULL;
  }

  switch (object->type) {
  case SLP_TYPE_INTEGER:
    printf("[EVAL INTEGER] %lld\n", object->value.integer);
    return slp_object_copy(object);

  case SLP_TYPE_REAL:
    printf("[EVAL REAL] %f\n", object->value.real);
    return slp_object_copy(object);

  case SLP_TYPE_SYMBOL:
    printf("[EVAL SYMBOL] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    return slp_object_copy(object);

  case SLP_TYPE_QUOTED: {
    printf("[EVAL QUOTED] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");

    if (!object->value.buffer) {
      return sxs_create_error_object(SLP_ERROR_PARSE_QUOTED_TOKEN,
                                     "quoted expression has nil buffer", 0);
    }

    slp_callbacks_t *callbacks = sxs_runtime_get_callbacks(runtime);
    if (!callbacks) {
      return sxs_create_error_object(
          SLP_ERROR_ALLOCATION, "failed to get callbacks for quoted eval", 0);
    }

    bool prev_error_state = runtime->runtime_has_error;
    runtime->runtime_has_error = false;

    int result = slp_process_buffer(object->value.buffer, callbacks);

    if (result != 0 || runtime->runtime_has_error) {
      runtime->runtime_has_error = prev_error_state;
      return sxs_create_error_object(SLP_ERROR_PARSE_QUOTED_TOKEN,
                                     "quoted expression evaluation failed", 0);
    }

    runtime->runtime_has_error = prev_error_state;

    if (runtime->current_context->proc_list_count == 0) {
      slp_object_t *none = malloc(sizeof(slp_object_t));
      if (none) {
        none->type = SLP_TYPE_NONE;
      }
      return none;
    }

    slp_object_t *result_obj = slp_object_copy(
        runtime->current_context
            ->object_proc_list[runtime->current_context->proc_list_count - 1]);
    return result_obj;
  }

  case SLP_TYPE_LIST_P:
    printf("[EVAL LIST_P] count=%zu\n", object->value.list.count);
    return sxs_eval_list(runtime, object);

  case SLP_TYPE_LIST_B:
    printf("[EVAL LIST_B] count=%zu\n", object->value.list.count);
    return slp_object_copy(object);

  case SLP_TYPE_LIST_C:
    printf("[EVAL LIST_C] count=%zu\n", object->value.list.count);
    return slp_object_copy(object);

  case SLP_TYPE_BUILTIN:
    printf("[EVAL BUILTIN]\n");
    return slp_object_copy(object);

  case SLP_TYPE_LAMBDA:
    printf("[EVAL LAMBDA]\n");
    return slp_object_copy(object);

  case SLP_TYPE_NONE:
    printf("[EVAL NONE]\n");
    return slp_object_copy(object);

  case SLP_TYPE_ERROR:
    printf("[EVAL ERROR]\n");
    return slp_object_copy(object);

  default:
    printf("[EVAL UNKNOWN TYPE]\n");
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "unknown object type in eval", 0);
  }
}
