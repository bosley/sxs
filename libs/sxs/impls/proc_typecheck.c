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

int sxs_typecheck_proc(sxs_typecheck_context_t *ctx, sxs_callable_t *callable,
                       slp_object_t **args, size_t arg_count) {
  if (!ctx || !callable) {
    return 1;
  }

  if (arg_count != 2) {
    sxs_typecheck_add_error(ctx, "proc builtin: expected 2 arguments", 0);
    return 1;
  }

  if (!args[0] || args[0]->type != SLP_TYPE_INTEGER) {
    size_t pos = args[0] ? args[0]->source_position : 0;
    sxs_typecheck_add_error(ctx, "proc builtin: first arg must be integer", pos);
    return 1;
  }

  if (!args[1]) {
    sxs_typecheck_add_error(ctx, "proc builtin: nil second argument", 0);
    return 1;
  }

  if (args[1]->type != SLP_TYPE_LIST_C) {
    sxs_typecheck_add_error(ctx, "proc builtin: second arg must be list-c",
                            args[1]->source_position);
    return 1;
  }

  form_definition_t *body_type = sxs_typecheck_object(ctx, args[1]);
  if (!body_type) {
    return 1;
  }

  int64_t reg_idx = args[0]->value.integer;
  if (reg_idx >= 0 && (size_t)reg_idx < SXS_OBJECT_STORAGE_SIZE) {
    if (ctx->register_types[reg_idx]) {
      free_form_def(ctx->register_types[reg_idx]);
    }
    ctx->register_types[reg_idx] = create_form_def_for_type(FORM_TYPE_FN);
  }

  free_form_def(body_type);
  return 0;
}
