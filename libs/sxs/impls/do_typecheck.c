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

int sxs_typecheck_do(sxs_typecheck_context_t *ctx, sxs_callable_t *callable,
                     slp_object_t **args, size_t arg_count) {
  if (!ctx || !callable) {
    return 1;
  }

  if (arg_count != 1) {
    sxs_typecheck_add_error(ctx, "do builtin: expected 1 argument", 0);
    return 1;
  }

  if (!args[0]) {
    sxs_typecheck_add_error(ctx, "do builtin: nil argument", 0);
    return 1;
  }

  form_definition_t *index_type = sxs_typecheck_object(ctx, args[0]);
  if (!index_type) {
    return 1;
  }

  if (index_type->type_count > 0 && index_type->types[0] != FORM_TYPE_INTEGER &&
      index_type->types[0] != FORM_TYPE_SYMBOL) {
    free_form_def(index_type);
    sxs_typecheck_add_error(ctx,
                            "do builtin: argument must be integer or symbol",
                            args[0]->source_position);
    return 1;
  }

  free_form_def(index_type);
  return 0;
}
