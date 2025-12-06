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

int sxs_typecheck_dot_map(sxs_typecheck_context_t *ctx,
                          sxs_callable_t *callable, slp_object_t **args,
                          size_t arg_count) {
  if (!ctx || !callable) {
    return 1;
  }

  if (arg_count != 2) {
    sxs_typecheck_add_error(ctx, ".map builtin: expected 2 arguments", 0);
    return 1;
  }

  if (!args[0] || args[0]->type != SLP_TYPE_SYMBOL) {
    size_t pos = args[0] ? args[0]->source_position : 0;
    sxs_typecheck_add_error(ctx, ".map builtin: first arg must be symbol", pos);
    return 1;
  }

  if (!args[1]) {
    sxs_typecheck_add_error(ctx, ".map builtin: nil second argument", 0);
    return 1;
  }

  form_definition_t *value_type = sxs_typecheck_object(ctx, args[1]);
  if (!value_type) {
    return 1;
  }

  if (!ctx->symbols) {
    free_form_def(value_type);
    sxs_typecheck_add_error(ctx, ".map builtin: nil symbols context", 0);
    return 1;
  }

  if (!args[0]->value.buffer || !args[0]->value.buffer->data) {
    free_form_def(value_type);
    sxs_typecheck_add_error(ctx, ".map builtin: symbol has nil buffer",
                            args[0]->source_position);
    return 1;
  }

  char *symbol_name = malloc(args[0]->value.buffer->count + 1);
  if (!symbol_name) {
    free_form_def(value_type);
    sxs_typecheck_add_error(ctx, ".map builtin: failed to allocate symbol name",
                            0);
    return 1;
  }
  memcpy(symbol_name, args[0]->value.buffer->data,
         args[0]->value.buffer->count);
  symbol_name[args[0]->value.buffer->count] = '\0';

  slp_object_t *type_obj = malloc(sizeof(slp_object_t));
  if (!type_obj) {
    free(symbol_name);
    free_form_def(value_type);
    sxs_typecheck_add_error(ctx, ".map builtin: failed to allocate type object",
                            0);
    return 1;
  }

  type_obj->type = SLP_TYPE_NONE;
  type_obj->value.fn_data = value_type;
  type_obj->source_position = args[1]->source_position;

  if (ctx_set(ctx->symbols, symbol_name, type_obj) != 0) {
    free(symbol_name);
    slp_object_free(type_obj);
    sxs_typecheck_add_error(ctx, ".map builtin: failed to set symbol type",
                            args[0]->source_position);
    return 1;
  }

  free(symbol_name);
  slp_object_free(type_obj);

  return 0;
}
