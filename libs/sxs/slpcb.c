#include "slp/slp.h"
#include "sxs/sxs.h"

#include <stdio.h>
#include <stdlib.h>

extern sxs_context_t *sxs_context_new(size_t context_id, sxs_context_t *parent);
extern void sxs_context_free(sxs_context_t *context);
extern slp_object_t *sxs_eval_object(sxs_context_t *context,
                                     slp_object_t *object);

slp_object_t *
sxs_get_builtin_load_store_object_for_context(sxs_context_t *context) {
  if (!context) {
    fprintf(
        stderr,
        "Failed to get builtin load store object for context (nil context)\n");
    return NULL;
  }

  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(
        stderr,
        "Failed to get builtin load store object for context (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = NULL; // TODO

  return builtin;
}

int sxs_context_push_object(sxs_context_t *context, slp_object_t *object) {
  if (!context) {
    fprintf(stderr, "Failed to push object (nil context)\n");
    return 1;
  }

  if (context->proc_list_count >= SXS_OBJECT_PROC_LIST_SIZE) {
    fprintf(stderr, "Failed to push object (proc list full)\n");
    return 1;
  }

  context->object_proc_list[context->proc_list_count] = object;
  context->proc_list_count++;
  return 0;
}

/*
  This is
*/
static void sxs_handle_object_from_slp_callback(slp_object_t *object,
                                                void *context) {
  if (!object) {
    fprintf(stderr, "Failed to process object (nil)\n");
    return;
  }

  if (!context) {
    fprintf(stderr, "Failed to process object (nil context)\n");
    return;
  }

  sxs_runtime_t *runtime = (sxs_runtime_t *)context;
  sxs_context_t *sxs_context = runtime->current_context;

  switch (object->type) {
  case SLP_TYPE_INTEGER:
    printf("[INTEGER] %lld\n", object->value.integer);
    break;
  case SLP_TYPE_REAL:
    printf("[REAL] %f\n", object->value.real);
    break;
  case SLP_TYPE_SYMBOL:
    printf("[SYMBOL] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    /*
       If this is the first item in the proc list and it matches a builtin
       we need to free the current object and replace it with a builtin
       so if we haveanything in the list we are already complete with this
       case
    */
    if (sxs_context->proc_list_count > 0) {
      break;
    }

    if (SXS_BUILTIN_LOAD_STORE_SYMBOL == object->value.buffer->data[0]) {
      printf("[BUILTIN LOAD STORE SYMBOL FOUND - UPDATING OBJECT]\n");
      slp_object_t *builtin =
          sxs_get_builtin_load_store_object_for_context(sxs_context);
      if (!builtin) {
        fprintf(stderr, "Failed to get builtin load store object for context "
                        "(nil builtin)\n");
        return;
      }
      slp_free_object(object);
      object = builtin;
    }
    break;
  case SLP_TYPE_QUOTED:
    printf("[QUOTED] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  case SLP_TYPE_LIST_P:
  case SLP_TYPE_LIST_B:
  case SLP_TYPE_LIST_C:
    printf("[LIST] count=%zu\n", object->value.list.count);
    break;
  default:
    printf("[UNKNOWN TYPE]\n");
    break;
  }

  sxs_context_push_object(sxs_context, object);
}

static void sxs_handle_list_start_from_slp_callback(slp_type_e list_type,
                                                    void *context) {
  if (!context) {
    fprintf(stderr, "Failed to handle list start (nil context)\n");
    return;
  }

  sxs_runtime_t *runtime = (sxs_runtime_t *)context;

  sxs_context_t *new_context =
      sxs_context_new(runtime->next_context_id++, runtime->current_context);
  if (!new_context) {
    fprintf(stderr, "Failed to create new context for list\n");
    return;
  }

  runtime->current_context = new_context;

  if (list_type == SLP_TYPE_LIST_P) {
    printf("[LIST_START (] context_id=%zu\n", new_context->context_id);
  } else if (list_type == SLP_TYPE_LIST_B) {
    printf("[LIST_START []] context_id=%zu\n", new_context->context_id);
  } else if (list_type == SLP_TYPE_LIST_C) {
    printf("[LIST_START {}] context_id=%zu\n", new_context->context_id);
  }
}

slp_object_t *sxs_convert_proc_list_to_objects_and_free(sxs_context_t *context,
                                                        slp_type_e list_type) {
  if (!context) {
    fprintf(stderr,
            "Failed to convert proc list to objects and free (nil context)\n");
    return NULL;
  }

  slp_object_t *list = malloc(sizeof(slp_object_t));
  if (!list) {
    fprintf(stderr,
            "Failed to convert proc list to objects and free (nil list)\n");
    return NULL;
  }

  list->type = list_type;
  list->value.list.count = context->proc_list_count;

  if (context->proc_list_count > 0) {
    list->value.list.items =
        malloc(sizeof(slp_object_t *) * context->proc_list_count);
    if (!list->value.list.items) {
      fprintf(
          stderr,
          "Failed to convert proc list to objects and free (nil list items)\n");
      free(list);
      return NULL;
    }

    for (size_t i = 0; i < context->proc_list_count; i++) {
      list->value.list.items[i] = slp_object_copy(context->object_proc_list[i]);
      if (!list->value.list.items[i]) {
        fprintf(stderr,
                "Failed to convert proc list to objects and free (nil object "
                "at index %zu)\n",
                i);
        for (size_t j = 0; j < i; j++) {
          slp_free_object(list->value.list.items[j]);
        }
        free(list->value.list.items);
        free(list);
        return NULL;
      }
      slp_free_object(context->object_proc_list[i]);
      context->object_proc_list[i] = NULL;
    }
  } else {
    list->value.list.items = NULL;
  }

  context->proc_list_count = 0;

  return list;
}

static void sxs_handle_list_end_from_slp_callback(slp_type_e list_type,
                                                  void *context) {
  if (!context) {
    fprintf(stderr, "Failed to handle list end (nil context)\n");
    return;
  }

  sxs_runtime_t *runtime = (sxs_runtime_t *)context;
  sxs_context_t *sxs_context = runtime->current_context;

  if (list_type == SLP_TYPE_LIST_B) {
    printf("[LIST_END []]\n");
  } else if (list_type == SLP_TYPE_LIST_C) {
    printf("[LIST_END {}]\n");
  } else if (list_type == SLP_TYPE_LIST_P) {
    printf("[LIST_END )]\n");
  }

  slp_object_t *list_object =
      sxs_convert_proc_list_to_objects_and_free(sxs_context, list_type);

  if (!list_object) {
    fprintf(stderr, "Failed to convert proc list to objects \n");
    return;
  }

  slp_object_t *result_to_hand_to_parent = NULL;

  if (list_type == SLP_TYPE_LIST_P) {
    result_to_hand_to_parent = sxs_eval_object(sxs_context, list_object);
    if (!result_to_hand_to_parent) {
      fprintf(stderr, "Failed to evaluate object\n");
      slp_free_object(list_object);
      return;
    }
    slp_free_object(list_object);
  } else {
    result_to_hand_to_parent = list_object;
  }

  if (NULL == sxs_context->parent) {
    sxs_context_free(sxs_context);
    slp_free_object(result_to_hand_to_parent);
    return;
  }

  sxs_context_t *parent = sxs_context->parent;
  runtime->current_context = parent;

  sxs_context_push_object(parent, result_to_hand_to_parent);

  sxs_context_free(sxs_context);
}

slp_callbacks_t *sxs_runtime_get_callbacks(sxs_runtime_t *runtime) {
  if (!runtime) {
    return NULL;
  }

  static slp_callbacks_t callbacks;
  callbacks.on_object = sxs_handle_object_from_slp_callback;
  callbacks.on_list_start = sxs_handle_list_start_from_slp_callback;
  callbacks.on_list_end = sxs_handle_list_end_from_slp_callback;
  callbacks.context = runtime;
  return &callbacks;
}