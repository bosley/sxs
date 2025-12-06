#include "slp/slp.h"
#include "sxs/sxs.h"
#include "sxs/typecheck.h"

#include <stdio.h>
#include <stdlib.h>

extern sxs_callable_t *sxs_get_callable_for_handler(sxs_handler_fn_t handler);

static int
sxs_typecheck_context_push_object(sxs_typecheck_context_stack_t *context,
                                  slp_object_t *object) {
  if (!context) {
    return 1;
  }

  if (context->proc_list_count >= SXS_OBJECT_PROC_LIST_SIZE) {
    return 1;
  }

  context->object_proc_list[context->proc_list_count] = object;
  context->proc_list_count++;
  return 0;
}

static slp_object_t *is_symbol_builtin_typecheck(sxs_typecheck_context_t *ctx,
                                                 slp_buffer_t *symbol) {
  if (!ctx || !ctx->builtin_registry || !symbol) {
    return NULL;
  }

  sxs_command_impl_t *impl =
      sxs_builtin_registry_lookup(ctx->builtin_registry, symbol);
  if (!impl) {
    return NULL;
  }

  sxs_callable_t *callable = sxs_get_callable_for_handler(impl->handler);
  if (!callable) {
    return NULL;
  }

  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)callable;

  return builtin;
}

static void
sxs_typecheck_clear_context_proc_list(sxs_typecheck_context_stack_t *context) {
  if (!context) {
    return;
  }

  for (size_t i = 0; i < context->proc_list_count; i++) {
    if (context->object_proc_list[i]) {
      slp_object_free(context->object_proc_list[i]);
      context->object_proc_list[i] = NULL;
    }
  }
  context->proc_list_count = 0;
}

static void sxs_typecheck_handle_object(slp_object_t *object, void *context) {
  if (!object || !context) {
    if (object)
      slp_object_free(object);
    return;
  }

  sxs_typecheck_context_t *ctx = (sxs_typecheck_context_t *)context;
  sxs_typecheck_context_stack_t *tc_context = ctx->current_context;

  if (ctx->has_error) {
    slp_object_free(object);
    return;
  }

  if (object->type == SLP_TYPE_SYMBOL) {
    if (tc_context->proc_list_count > 0) {
      sxs_typecheck_context_push_object(tc_context, object);
      return;
    }

    slp_object_t *builtin =
        is_symbol_builtin_typecheck(ctx, object->value.buffer);
    if (builtin) {
      slp_object_free(object);
      object = builtin;
    }
  }

  sxs_typecheck_context_push_object(tc_context, object);
}

static void sxs_typecheck_handle_list_start(slp_type_e list_type,
                                            void *context) {
  if (!context) {
    return;
  }

  sxs_typecheck_context_t *ctx = (sxs_typecheck_context_t *)context;

  if (ctx->has_error) {
    return;
  }

  sxs_typecheck_context_stack_t *new_context =
      malloc(sizeof(sxs_typecheck_context_stack_t));
  if (!new_context) {
    sxs_typecheck_add_error(ctx, "failed to create typecheck context", 0);
    return;
  }

  new_context->context_id = ctx->next_context_id++;
  new_context->parent = ctx->current_context;
  new_context->proc_list_count = 0;
  new_context->result_type = NULL;

  for (size_t i = 0; i < SXS_OBJECT_PROC_LIST_SIZE; i++) {
    new_context->object_proc_list[i] = NULL;
  }

  ctx->current_context = new_context;
}

static slp_object_t *sxs_typecheck_convert_proc_list_to_objects(
    sxs_typecheck_context_stack_t *context, slp_type_e list_type) {
  if (!context) {
    return NULL;
  }

  slp_object_t *list = malloc(sizeof(slp_object_t));
  if (!list) {
    return NULL;
  }

  list->type = list_type;
  list->value.list.count = context->proc_list_count;
  list->source_position = 0;

  if (context->proc_list_count > 0) {
    list->source_position = context->object_proc_list[0]->source_position;

    list->value.list.items =
        malloc(sizeof(slp_object_t *) * context->proc_list_count);
    if (!list->value.list.items) {
      free(list);
      return NULL;
    }

    for (size_t i = 0; i < context->proc_list_count; i++) {
      list->value.list.items[i] = slp_object_copy(context->object_proc_list[i]);
      if (!list->value.list.items[i]) {
        for (size_t j = 0; j < i; j++) {
          slp_object_free(list->value.list.items[j]);
        }
        free(list->value.list.items);
        free(list);
        return NULL;
      }
      slp_object_free(context->object_proc_list[i]);
      context->object_proc_list[i] = NULL;
    }
  } else {
    list->value.list.items = NULL;
  }

  context->proc_list_count = 0;

  return list;
}

static void sxs_typecheck_handle_list_end(slp_type_e list_type, void *context) {
  if (!context) {
    return;
  }

  sxs_typecheck_context_t *ctx = (sxs_typecheck_context_t *)context;

  if (ctx->has_error) {
    return;
  }

  sxs_typecheck_context_stack_t *tc_context = ctx->current_context;

  slp_object_t *list_object =
      sxs_typecheck_convert_proc_list_to_objects(tc_context, list_type);

  if (!list_object) {
    sxs_typecheck_add_error(ctx, "failed to convert proc list to objects", 0);
    return;
  }

  slp_object_t *result_to_hand_to_parent = NULL;

  if (list_type == SLP_TYPE_LIST_P && !ctx->parsing_quoted_expression) {
    form_definition_t *result_type = sxs_typecheck_object(ctx, list_object);
    if (result_type) {
      if (result_type->types)
        free(result_type->types);
      free(result_type);
    }
    result_to_hand_to_parent = list_object;
  } else {
    result_to_hand_to_parent = list_object;
  }

  if (NULL == tc_context->parent) {
    sxs_typecheck_context_push_object(tc_context, result_to_hand_to_parent);
    return;
  }

  sxs_typecheck_context_stack_t *parent = tc_context->parent;
  ctx->current_context = parent;

  sxs_typecheck_context_push_object(parent, result_to_hand_to_parent);

  sxs_typecheck_clear_context_proc_list(tc_context);
  free(tc_context);
}

static void sxs_typecheck_handle_virtual_list_start(void *context) {
  if (!context) {
    return;
  }

  sxs_typecheck_context_t *ctx = (sxs_typecheck_context_t *)context;

  if (ctx->has_error) {
    return;
  }

  sxs_typecheck_context_stack_t *new_context =
      malloc(sizeof(sxs_typecheck_context_stack_t));
  if (!new_context) {
    sxs_typecheck_add_error(ctx, "failed to create typecheck context", 0);
    return;
  }

  new_context->context_id = ctx->next_context_id++;
  new_context->parent = ctx->current_context;
  new_context->proc_list_count = 0;
  new_context->result_type = NULL;

  for (size_t i = 0; i < SXS_OBJECT_PROC_LIST_SIZE; i++) {
    new_context->object_proc_list[i] = NULL;
  }

  ctx->current_context = new_context;
}

static void sxs_typecheck_handle_virtual_list_end(void *context) {
  if (!context) {
    return;
  }

  sxs_typecheck_context_t *ctx = (sxs_typecheck_context_t *)context;

  if (ctx->has_error) {
    return;
  }

  sxs_typecheck_context_stack_t *tc_context = ctx->current_context;

  slp_object_t *list_object =
      sxs_typecheck_convert_proc_list_to_objects(tc_context, SLP_TYPE_LIST_P);

  if (!list_object) {
    sxs_typecheck_add_error(ctx, "failed to convert proc list to objects", 0);
    return;
  }

  slp_object_t *result_to_hand_to_parent = NULL;

  if (!ctx->parsing_quoted_expression) {
    form_definition_t *result_type = sxs_typecheck_object(ctx, list_object);
    if (result_type) {
      if (result_type->types)
        free(result_type->types);
      free(result_type);
    }
    result_to_hand_to_parent = list_object;
  } else {
    result_to_hand_to_parent = list_object;
  }

  if (NULL == tc_context->parent) {
    sxs_typecheck_context_push_object(tc_context, result_to_hand_to_parent);
    return;
  }

  sxs_typecheck_context_stack_t *parent = tc_context->parent;
  ctx->current_context = parent;

  sxs_typecheck_context_push_object(parent, result_to_hand_to_parent);

  sxs_typecheck_clear_context_proc_list(tc_context);
  free(tc_context);
}

static void sxs_typecheck_handle_error(slp_error_type_e error_type,
                                       const char *message, size_t position,
                                       slp_buffer_t *buffer, void *context) {
  if (!context) {
    return;
  }

  sxs_typecheck_context_t *ctx = (sxs_typecheck_context_t *)context;
  sxs_typecheck_context_stack_t *tc_context = ctx->current_context;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Parse error: %s", message);
  sxs_typecheck_add_error(ctx, error_msg, position);

  sxs_typecheck_clear_context_proc_list(tc_context);
}

slp_callbacks_t *sxs_typecheck_get_callbacks(sxs_typecheck_context_t *ctx) {
  if (!ctx) {
    return NULL;
  }

  static slp_callbacks_t callbacks;
  callbacks.on_object = sxs_typecheck_handle_object;
  callbacks.on_list_start = sxs_typecheck_handle_list_start;
  callbacks.on_list_end = sxs_typecheck_handle_list_end;
  callbacks.on_virtual_list_start = sxs_typecheck_handle_virtual_list_start;
  callbacks.on_virtual_list_end = sxs_typecheck_handle_virtual_list_end;
  callbacks.on_error = sxs_typecheck_handle_error;
  callbacks.context = ctx;
  return &callbacks;
}
