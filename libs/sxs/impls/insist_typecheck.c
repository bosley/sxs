#include "sxs/forms.h"
#include "sxs/sxs.h"
#include "sxs/typecheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern form_definition_t *sxs_typecheck_object(sxs_typecheck_context_t *ctx,
                                               slp_object_t *object);

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

static form_type_e symbol_to_form_type(slp_buffer_t *buffer) {
  if (!buffer || buffer->count < 2 || buffer->data[0] != ':') {
    return FORM_TYPE_NONE;
  }

  size_t name_len = buffer->count - 1;
  const char *name_start = (const char *)&buffer->data[1];

  if (name_len == 3 && memcmp(name_start, "int", 3) == 0) {
    return FORM_TYPE_INTEGER;
  } else if (name_len == 4 && memcmp(name_start, "real", 4) == 0) {
    return FORM_TYPE_REAL;
  } else if (name_len == 6 && memcmp(name_start, "symbol", 6) == 0) {
    return FORM_TYPE_SYMBOL;
  } else if (name_len == 6 && memcmp(name_start, "list-s", 6) == 0) {
    return FORM_TYPE_LIST_S;
  } else if (name_len == 6 && memcmp(name_start, "list-p", 6) == 0) {
    return FORM_TYPE_LIST_P;
  } else if (name_len == 6 && memcmp(name_start, "list-b", 6) == 0) {
    return FORM_TYPE_LIST_B;
  } else if (name_len == 6 && memcmp(name_start, "list-c", 6) == 0) {
    return FORM_TYPE_LIST_C;
  } else if (name_len == 4 && memcmp(name_start, "some", 4) == 0) {
    return FORM_TYPE_SOME;
  } else if (name_len == 2 && memcmp(name_start, "fn", 2) == 0) {
    return FORM_TYPE_FN;
  } else if (name_len == 3 && memcmp(name_start, "any", 3) == 0) {
    return FORM_TYPE_ANY;
  } else if (name_len == 4 && memcmp(name_start, "none", 4) == 0) {
    return FORM_TYPE_NONE;
  }

  return FORM_TYPE_NONE;
}

int sxs_typecheck_insist(sxs_typecheck_context_t *ctx, sxs_callable_t *callable,
                         slp_object_t **args, size_t arg_count) {
  if (!ctx || !callable) {
    return 1;
  }

  if (arg_count != 2) {
    sxs_typecheck_add_error(ctx, "insist: expected 2 arguments", 0);
    return 1;
  }

  if (!args[0] || !args[1]) {
    sxs_typecheck_add_error(ctx, "insist: nil argument", 0);
    return 1;
  }

  form_definition_t *arg0_type = sxs_typecheck_object(ctx, args[0]);
  if (!arg0_type) {
    return 1;
  }

  if (arg0_type->types[0] != FORM_TYPE_SYMBOL) {
    sxs_typecheck_add_error(ctx, "insist: first argument must be a symbol",
                            args[0]->source_position);
    free_form_def(arg0_type);
    return 1;
  }

  if (args[0]->type != SLP_TYPE_SYMBOL || !args[0]->value.buffer ||
      args[0]->value.buffer->count < 2 ||
      args[0]->value.buffer->data[0] != ':') {
    sxs_typecheck_add_error(
        ctx, "insist: first argument must be a form symbol (e.g., :int, :real)",
        args[0]->source_position);
    free_form_def(arg0_type);
    return 1;
  }

  form_type_e insisted_type = symbol_to_form_type(args[0]->value.buffer);
  if (insisted_type == FORM_TYPE_NONE && args[0]->value.buffer->count > 2) {
    sxs_typecheck_add_error(ctx, "insist: unknown form type",
                            args[0]->source_position);
    free_form_def(arg0_type);
    return 1;
  }

  free_form_def(arg0_type);

  form_definition_t *arg1_type = sxs_typecheck_object(ctx, args[1]);
  if (!arg1_type) {
    return 1;
  }

  if (insisted_type != FORM_TYPE_ANY && arg1_type->types[0] != FORM_TYPE_ANY) {
    if (insisted_type != arg1_type->types[0]) {
      char error_msg[512];
      const char *expected_name = sxs_forms_get_form_type_name(insisted_type);
      const char *actual_name =
          sxs_forms_get_form_type_name(arg1_type->types[0]);
      snprintf(error_msg, sizeof(error_msg),
               "Type assertion failed: insist expects '%s' but expression "
               "evaluates to '%s'",
               expected_name, actual_name);
      size_t error_pos = args[1]->source_position > 0
                             ? args[1]->source_position
                             : args[0]->source_position;
      sxs_typecheck_add_detailed_error(ctx, error_msg, error_pos, "insist",
                                       expected_name, actual_name);
      free_form_def(arg1_type);
      return 1;
    }
  }

  if (args[1]->type == SLP_TYPE_LIST_P && args[1]->value.list.count == 2) {
    slp_object_t *first = args[1]->value.list.items[0];
    slp_object_t *second = args[1]->value.list.items[1];

    if (first && first->type == SLP_TYPE_BUILTIN && second &&
        second->type == SLP_TYPE_INTEGER) {
      sxs_callable_t *inner_callable = (sxs_callable_t *)first->value.fn_data;
      if (inner_callable && inner_callable->name &&
          strcmp(inner_callable->name, "@") == 0) {
        int64_t reg_idx = second->value.integer;
        if (reg_idx >= 0 && (size_t)reg_idx < SXS_OBJECT_STORAGE_SIZE) {
          if (ctx->register_types[reg_idx]) {
            free_form_def(ctx->register_types[reg_idx]);
          }
          ctx->register_types[reg_idx] =
              create_form_def_for_type(insisted_type);
        }
      }
    }
  }

  free_form_def(arg1_type);

  return 0;
}
