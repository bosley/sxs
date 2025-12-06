#include "typecheck.h"
#include "forms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_ERROR_CAPACITY 16

static form_definition_t *create_form_def_for_type(form_type_e type) {
  form_definition_t *def = malloc(sizeof(form_definition_t));
  if (!def) {
    return NULL;
  }

  def->name = NULL;
  def->types = malloc(sizeof(form_type_e));
  if (!def->types) {
    free(def);
    return NULL;
  }
  def->types[0] = type;
  def->type_count = 1;
  def->list_constraint = SLP_TYPE_NONE;
  def->is_variadic = false;

  return def;
}

static sxs_typecheck_context_stack_t *
sxs_typecheck_context_stack_new(size_t context_id,
                                sxs_typecheck_context_stack_t *parent) {
  sxs_typecheck_context_stack_t *context =
      malloc(sizeof(sxs_typecheck_context_stack_t));
  if (!context) {
    return NULL;
  }

  context->context_id = context_id;
  context->parent = parent;
  context->proc_list_count = 0;
  context->result_type = NULL;

  for (size_t i = 0; i < SXS_OBJECT_PROC_LIST_SIZE; i++) {
    context->object_proc_list[i] = NULL;
  }

  return context;
}

static void
sxs_typecheck_context_stack_free(sxs_typecheck_context_stack_t *context) {
  if (!context) {
    return;
  }

  for (size_t i = 0; i < context->proc_list_count; i++) {
    if (context->object_proc_list[i]) {
      slp_object_free(context->object_proc_list[i]);
    }
  }

  free(context);
}

sxs_typecheck_context_t *
sxs_typecheck_context_create(sxs_builtin_registry_t *registry) {
  sxs_typecheck_context_t *ctx = malloc(sizeof(sxs_typecheck_context_t));
  if (!ctx) {
    return NULL;
  }

  ctx->current_context = sxs_typecheck_context_stack_new(0, NULL);
  if (!ctx->current_context) {
    free(ctx);
    return NULL;
  }

  ctx->next_context_id = 1;
  ctx->builtin_registry = registry;
  ctx->source_buffer = NULL;
  ctx->has_error = false;
  ctx->parsing_quoted_expression = false;

  for (size_t i = 0; i < SXS_OBJECT_STORAGE_SIZE; i++) {
    ctx->register_types[i] = NULL;
  }

  ctx->errors = malloc(sizeof(sxs_typecheck_error_t) * INITIAL_ERROR_CAPACITY);
  if (!ctx->errors) {
    sxs_typecheck_context_stack_free(ctx->current_context);
    free(ctx);
    return NULL;
  }
  ctx->error_count = 0;
  ctx->error_capacity = INITIAL_ERROR_CAPACITY;

  return ctx;
}

void sxs_typecheck_context_free(sxs_typecheck_context_t *ctx) {
  if (!ctx) {
    return;
  }

  if (ctx->current_context) {
    sxs_typecheck_context_stack_free(ctx->current_context);
  }

  if (ctx->source_buffer) {
    slp_buffer_free(ctx->source_buffer);
  }

  for (size_t i = 0; i < ctx->error_count; i++) {
    if (ctx->errors[i].message) {
      free(ctx->errors[i].message);
    }
    if (ctx->errors[i].function_name) {
      free(ctx->errors[i].function_name);
    }
    if (ctx->errors[i].expected_signature) {
      free(ctx->errors[i].expected_signature);
    }
    if (ctx->errors[i].received_signature) {
      free(ctx->errors[i].received_signature);
    }
  }
  if (ctx->errors) {
    free(ctx->errors);
  }

  for (size_t i = 0; i < SXS_OBJECT_STORAGE_SIZE; i++) {
    if (ctx->register_types[i]) {
      if (ctx->register_types[i]->name) {
        free(ctx->register_types[i]->name);
      }
      if (ctx->register_types[i]->types) {
        free(ctx->register_types[i]->types);
      }
      free(ctx->register_types[i]);
    }
  }

  free(ctx);
}

void sxs_typecheck_add_error(sxs_typecheck_context_t *ctx, const char *message,
                             size_t position) {
  sxs_typecheck_add_detailed_error(ctx, message, position, NULL, NULL, NULL);
}

void sxs_typecheck_add_detailed_error(sxs_typecheck_context_t *ctx,
                                      const char *message, size_t position,
                                      const char *function_name,
                                      const char *expected_sig,
                                      const char *received_sig) {
  if (!ctx || !message) {
    return;
  }

  if (ctx->error_count >= ctx->error_capacity) {
    size_t new_capacity = ctx->error_capacity * 2;
    sxs_typecheck_error_t *new_errors =
        realloc(ctx->errors, sizeof(sxs_typecheck_error_t) * new_capacity);
    if (!new_errors) {
      return;
    }
    ctx->errors = new_errors;
    ctx->error_capacity = new_capacity;
  }

  sxs_typecheck_error_t *err = &ctx->errors[ctx->error_count];

  err->message = malloc(strlen(message) + 1);
  if (err->message) {
    strcpy(err->message, message);
  }

  err->position = position;

  if (function_name) {
    err->function_name = malloc(strlen(function_name) + 1);
    if (err->function_name) {
      strcpy(err->function_name, function_name);
    }
  } else {
    err->function_name = NULL;
  }

  if (expected_sig) {
    err->expected_signature = malloc(strlen(expected_sig) + 1);
    if (err->expected_signature) {
      strcpy(err->expected_signature, expected_sig);
    }
  } else {
    err->expected_signature = NULL;
  }

  if (received_sig) {
    err->received_signature = malloc(strlen(received_sig) + 1);
    if (err->received_signature) {
      strcpy(err->received_signature, received_sig);
    }
  } else {
    err->received_signature = NULL;
  }

  ctx->error_count++;
  ctx->has_error = true;
}

static void print_source_context_typecheck(slp_buffer_t *buffer,
                                           size_t error_position) {
  if (!buffer || !buffer->data || error_position >= buffer->count) {
    return;
  }

  size_t line = 1;
  size_t col = 1;
  size_t line_start = 0;

  for (size_t i = 0; i < error_position && i < buffer->count; i++) {
    if (buffer->data[i] == '\n') {
      line++;
      col = 1;
      line_start = i + 1;
    } else {
      col++;
    }
  }

  size_t line_end = line_start;
  while (line_end < buffer->count && buffer->data[line_end] != '\n') {
    line_end++;
  }

  fprintf(stderr, "\n  \033[90mSource:\033[0m\n");
  fprintf(stderr, "  \033[90m%4zu |\033[0m ", line);

  for (size_t i = line_start; i < line_end; i++) {
    fprintf(stderr, "%c", buffer->data[i]);
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "  \033[90m     |\033[0m ");
  for (size_t i = 0; i < col - 1; i++) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "\033[31m^\033[0m\n");

  fprintf(stderr, "  \033[90m     |\033[0m ");
  for (size_t i = 0; i < col - 1; i++) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "\033[31m└─ here\033[0m\n");
}

void sxs_typecheck_print_errors(sxs_typecheck_context_t *ctx) {
  if (!ctx) {
    return;
  }

  for (size_t i = 0; i < ctx->error_count; i++) {
    sxs_typecheck_error_t *err = &ctx->errors[i];

    fprintf(stderr, "\n");
    fprintf(stderr,
            "╔════════════════════════════════════════════════════════════════"
            "════════════╗\n");
    fprintf(stderr,
            "║ TYPE ERROR                                                     "
            "            ║\n");
    fprintf(stderr,
            "╚════════════════════════════════════════════════════════════════"
            "════════════╝\n");

    fprintf(stderr, "\n  %s\n", err->message);

    if (err->function_name) {
      fprintf(stderr, "\n  Function: \033[1m%s\033[0m\n", err->function_name);
    }

    if (err->expected_signature) {
      fprintf(stderr, "  Expected: \033[32m%s\033[0m\n",
              err->expected_signature);
    }

    if (err->received_signature) {
      fprintf(stderr, "  Received: \033[31m%s\033[0m\n",
              err->received_signature);
    }

    if (ctx->source_buffer && err->position > 0) {
      print_source_context_typecheck(ctx->source_buffer, err->position);
    }

    fprintf(stderr,
            "───────────────────────────────────────────────────────────────"
            "─────────────────\n");
  }

  fprintf(stderr, "\n");
}

form_definition_t *sxs_typecheck_object(sxs_typecheck_context_t *ctx,
                                        slp_object_t *object) {
  if (!ctx || !object) {
    return NULL;
  }

  switch (object->type) {
  case SLP_TYPE_INTEGER:
    return create_form_def_for_type(FORM_TYPE_INTEGER);

  case SLP_TYPE_REAL:
    return create_form_def_for_type(FORM_TYPE_REAL);

  case SLP_TYPE_SYMBOL:
    return create_form_def_for_type(FORM_TYPE_SYMBOL);

  case SLP_TYPE_QUOTED:
    return create_form_def_for_type(FORM_TYPE_SOME);

  case SLP_TYPE_LIST_S:
    return create_form_def_for_type(FORM_TYPE_LIST_S);

  case SLP_TYPE_LIST_B:
    return create_form_def_for_type(FORM_TYPE_LIST_B);

  case SLP_TYPE_LIST_C:
    return create_form_def_for_type(FORM_TYPE_LIST_C);

  case SLP_TYPE_LIST_P:
    return sxs_typecheck_list(ctx, object);

  case SLP_TYPE_BUILTIN:
  case SLP_TYPE_LAMBDA:
    return create_form_def_for_type(FORM_TYPE_FN);

  case SLP_TYPE_NONE:
    return create_form_def_for_type(FORM_TYPE_NONE);

  case SLP_TYPE_ERROR:
    return create_form_def_for_type(FORM_TYPE_NONE);

  default:
    sxs_typecheck_add_error(ctx, "unknown object type in typecheck",
                            object->source_position);
    return NULL;
  }
}

form_definition_t *sxs_typecheck_list(sxs_typecheck_context_t *ctx,
                                      slp_object_t *list) {
  if (!ctx || !list || list->type != SLP_TYPE_LIST_P) {
    sxs_typecheck_add_error(ctx, "invalid list type for typecheck", 0);
    return NULL;
  }

  if (list->value.list.count == 0) {
    sxs_typecheck_add_error(ctx, "empty list in typecheck", 0);
    return NULL;
  }

  slp_object_t *first = list->value.list.items[0];
  if (!first) {
    sxs_typecheck_add_error(ctx, "nil first item in list", 0);
    return NULL;
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
      sxs_typecheck_add_error(ctx, "nil builtin callable", 0);
      return NULL;
    }
    if (!callable->typecheck_fn) {
      sxs_typecheck_add_error(ctx, "builtin missing typecheck function", 0);
      return NULL;
    }

    int result = callable->typecheck_fn(ctx, callable, args, arg_count);
    if (result != 0) {
      return NULL;
    }

    if (callable->variant_count > 0 && callable->variants[0].return_type) {
      return create_form_def_for_type(
          callable->variants[0].return_type->types[0]);
    }
    return create_form_def_for_type(FORM_TYPE_ANY);
  }

  if (first->type == SLP_TYPE_LAMBDA) {
    sxs_typecheck_add_error(ctx, "lambda typecheck not yet implemented", 0);
    return NULL;
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
    sxs_typecheck_add_error(ctx, error_msg, first->source_position);
    return NULL;
  }

  sxs_typecheck_add_error(ctx, "invalid function type", 0);
  return NULL;
}

int sxs_typecheck_file(const char *filename, sxs_builtin_registry_t *registry,
                       sxs_typecheck_context_t **out_ctx) {
  if (!filename || !registry || !out_ctx) {
    return 1;
  }

  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);
  if (!ctx) {
    return 1;
  }

  slp_buffer_t *buffer = slp_buffer_from_file((char *)filename);
  if (!buffer) {
    sxs_typecheck_context_free(ctx);
    return 1;
  }

  ctx->source_buffer = buffer;

  slp_callbacks_t *callbacks = sxs_typecheck_get_callbacks(ctx);
  if (!callbacks) {
    sxs_typecheck_context_free(ctx);
    return 1;
  }

  int result = slp_process_buffer(buffer, callbacks);

  if (result != 0 || ctx->error_count > 0) {
    *out_ctx = ctx;
    return 1;
  }

  sxs_typecheck_context_free(ctx);
  *out_ctx = NULL;
  return 0;
}

static form_definition_t *copy_form_def(form_definition_t *src) {
  if (!src) {
    return NULL;
  }

  form_definition_t *copy = malloc(sizeof(form_definition_t));
  if (!copy) {
    return NULL;
  }

  copy->name = NULL;
  copy->types = malloc(sizeof(form_type_e) * src->type_count);
  if (!copy->types) {
    free(copy);
    return NULL;
  }

  memcpy(copy->types, src->types, sizeof(form_type_e) * src->type_count);
  copy->type_count = src->type_count;
  copy->list_constraint = src->list_constraint;
  copy->is_variadic = src->is_variadic;

  return copy;
}

static void free_form_def(form_definition_t *def) {
  if (!def) {
    return;
  }
  if (def->name) {
    free(def->name);
  }
  if (def->types) {
    free(def->types);
  }
  free(def);
}

static bool check_arg_matches_form(form_definition_t *arg_type,
                                   form_definition_t *form) {
  if (!arg_type || !form || !arg_type->types || !form->types) {
    return false;
  }

  form_type_e arg_form = arg_type->types[0];

  for (size_t i = 0; i < form->type_count; i++) {
    form_type_e form_type = form->types[i];

    if (form_type == FORM_TYPE_ANY || form_type == FORM_TYPE_ANY_VARIADIC) {
      return true;
    }

    if (form_type == arg_form) {
      return true;
    }

    if (form->is_variadic) {
      form_type_e base_type = form_type;
      switch (form_type) {
      case FORM_TYPE_INTEGER_VARIADIC:
        base_type = FORM_TYPE_INTEGER;
        break;
      case FORM_TYPE_REAL_VARIADIC:
        base_type = FORM_TYPE_REAL;
        break;
      case FORM_TYPE_SYMBOL_VARIADIC:
        base_type = FORM_TYPE_SYMBOL;
        break;
      case FORM_TYPE_LIST_S_VARIADIC:
        base_type = FORM_TYPE_LIST_S;
        break;
      case FORM_TYPE_LIST_P_VARIADIC:
        base_type = FORM_TYPE_LIST_P;
        break;
      case FORM_TYPE_LIST_B_VARIADIC:
        base_type = FORM_TYPE_LIST_B;
        break;
      case FORM_TYPE_LIST_C_VARIADIC:
        base_type = FORM_TYPE_LIST_C;
        break;
      case FORM_TYPE_SOME_VARIADIC:
        base_type = FORM_TYPE_SOME;
        break;
      case FORM_TYPE_FN_VARIADIC:
        base_type = FORM_TYPE_FN;
        break;
      default:
        break;
      }
      if (base_type == arg_form) {
        return true;
      }
    }
  }

  return false;
}

static sxs_callable_variant_t *find_matching_variant_by_types(
    sxs_callable_t *callable, form_definition_t **arg_types, size_t arg_count) {
  if (!callable) {
    return NULL;
  }

  for (size_t v = 0; v < callable->variant_count; v++) {
    sxs_callable_variant_t *variant = &callable->variants[v];

    bool has_variadic = false;
    if (variant->param_count > 0 && variant->params &&
        variant->params[variant->param_count - 1].form &&
        variant->params[variant->param_count - 1].form->is_variadic) {
      has_variadic = true;
    }

    if (has_variadic) {
      if (arg_count < variant->param_count) {
        continue;
      }
    } else {
      if (variant->param_count != arg_count) {
        continue;
      }
    }

    bool all_match = true;
    for (size_t i = 0; i < arg_count; i++) {
      if (!variant->params) {
        all_match = false;
        break;
      }

      size_t param_idx = i;
      if (has_variadic && i >= variant->param_count) {
        param_idx = variant->param_count - 1;
      }

      if (!variant->params[param_idx].form) {
        all_match = false;
        break;
      }

      if (!check_arg_matches_form(arg_types[i],
                                  variant->params[param_idx].form)) {
        all_match = false;
        break;
      }
    }

    if (all_match) {
      return variant;
    }
  }

  return NULL;
}

static const char *form_type_to_string(form_type_e type) {
  switch (type) {
  case FORM_TYPE_INTEGER:
    return "int";
  case FORM_TYPE_REAL:
    return "real";
  case FORM_TYPE_SYMBOL:
    return "symbol";
  case FORM_TYPE_LIST_S:
    return "list-s";
  case FORM_TYPE_LIST_P:
    return "list-p";
  case FORM_TYPE_LIST_B:
    return "list-b";
  case FORM_TYPE_LIST_C:
    return "list-c";
  case FORM_TYPE_SOME:
    return "some";
  case FORM_TYPE_FN:
    return "fn";
  case FORM_TYPE_ANY:
    return "any";
  case FORM_TYPE_NONE:
    return "none";
  case FORM_TYPE_INTEGER_VARIADIC:
    return "int..";
  case FORM_TYPE_REAL_VARIADIC:
    return "real..";
  case FORM_TYPE_SYMBOL_VARIADIC:
    return "symbol..";
  case FORM_TYPE_LIST_S_VARIADIC:
    return "list-s..";
  case FORM_TYPE_LIST_P_VARIADIC:
    return "list-p..";
  case FORM_TYPE_LIST_B_VARIADIC:
    return "list-b..";
  case FORM_TYPE_LIST_C_VARIADIC:
    return "list-c..";
  case FORM_TYPE_SOME_VARIADIC:
    return "some..";
  case FORM_TYPE_FN_VARIADIC:
    return "fn..";
  case FORM_TYPE_ANY_VARIADIC:
    return "any..";
  default:
    return "unknown";
  }
}

static void build_signature_string(char *buffer, size_t buffer_size,
                                   const char *func_name,
                                   form_definition_t **arg_types,
                                   size_t arg_count) {
  size_t offset = 0;
  offset += snprintf(buffer + offset, buffer_size - offset, "(%s", func_name);

  for (size_t i = 0; i < arg_count && offset < buffer_size; i++) {
    if (arg_types[i] && arg_types[i]->types && arg_types[i]->type_count > 0) {
      offset += snprintf(buffer + offset, buffer_size - offset, " %s",
                         form_type_to_string(arg_types[i]->types[0]));
    } else {
      offset += snprintf(buffer + offset, buffer_size - offset, " ?");
    }
  }

  if (offset < buffer_size) {
    snprintf(buffer + offset, buffer_size - offset, ")");
  }
}

static void build_variant_signature(char *buffer, size_t buffer_size,
                                    const char *func_name,
                                    sxs_callable_variant_t *variant) {
  size_t offset = 0;
  offset += snprintf(buffer + offset, buffer_size - offset, "(%s", func_name);

  for (size_t i = 0; i < variant->param_count && offset < buffer_size; i++) {
    if (variant->params[i].form && variant->params[i].form->types &&
        variant->params[i].form->type_count > 0) {
      offset +=
          snprintf(buffer + offset, buffer_size - offset, " %s",
                   form_type_to_string(variant->params[i].form->types[0]));
    } else {
      offset += snprintf(buffer + offset, buffer_size - offset, " ?");
    }
  }

  if (offset < buffer_size) {
    snprintf(buffer + offset, buffer_size - offset, ")");
  }
}

static char *get_function_name(slp_object_t **args, size_t arg_count) {
  if (arg_count > 0 && args[0] && args[0]->type == SLP_TYPE_BUILTIN) {
    sxs_callable_t *callable = (sxs_callable_t *)args[0]->value.fn_data;
    if (callable && callable->variant_count > 0 &&
        callable->variants[0].params && callable->variants[0].params[0].name) {
      return callable->variants[0].params[0].name;
    }
  }
  return "unknown";
}

int sxs_typecheck_generic(sxs_typecheck_context_t *ctx,
                          sxs_callable_t *callable, slp_object_t **args,
                          size_t arg_count) {
  if (!ctx || !callable) {
    return 1;
  }

  form_definition_t *arg_types[16] = {NULL};
  for (size_t i = 0; i < arg_count && i < 16; i++) {
    if (!args[i]) {
      sxs_typecheck_add_error(ctx, "nil argument", 0);
      goto cleanup;
    }

    arg_types[i] = sxs_typecheck_object(ctx, args[i]);
    if (!arg_types[i]) {
      goto cleanup;
    }
  }

  sxs_callable_variant_t *variant =
      find_matching_variant_by_types(callable, arg_types, arg_count);

  if (!variant) {
    char error_msg[512];
    char expected_sig[256];
    char received_sig[256];
    const char *func_name = callable->name ? callable->name : "unknown";

    if (callable->variant_count > 0) {
      build_variant_signature(expected_sig, sizeof(expected_sig), func_name,
                              &callable->variants[0]);
    } else {
      snprintf(expected_sig, sizeof(expected_sig), "(%s)", func_name);
    }

    build_signature_string(received_sig, sizeof(received_sig), func_name,
                           arg_types, arg_count);

    snprintf(error_msg, sizeof(error_msg),
             "Function called with incompatible argument types");

    sxs_typecheck_add_detailed_error(
        ctx, error_msg, arg_count > 0 ? args[0]->source_position : 0, func_name,
        expected_sig, received_sig);
    goto cleanup;
  }

  if (arg_count == 2 && args[0]->type == SLP_TYPE_INTEGER) {
    int64_t reg_idx = args[0]->value.integer;
    if (reg_idx >= 0 && (size_t)reg_idx < SXS_OBJECT_STORAGE_SIZE) {
      if (ctx->register_types[reg_idx]) {
        free_form_def(ctx->register_types[reg_idx]);
      }
      ctx->register_types[reg_idx] = copy_form_def(arg_types[1]);
    }
  }

  for (size_t i = 0; i < arg_count && i < 16; i++) {
    if (arg_types[i]) {
      free_form_def(arg_types[i]);
    }
  }
  return 0;

cleanup:
  for (size_t i = 0; i < arg_count && i < 16; i++) {
    if (arg_types[i]) {
      free_form_def(arg_types[i]);
    }
  }
  return 1;
}
