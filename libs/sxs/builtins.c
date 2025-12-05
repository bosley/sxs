#include "sxs/errors.h"
#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sxs_callable_t *g_builtin_load_store_callable = NULL;
extern slp_object_t *sxs_builtin_load_store(sxs_runtime_t *runtime,
                                            sxs_callable_t *callable,
                                            slp_object_t **args,
                                            size_t arg_count);

static form_definition_t *create_form_def(form_type_e type) {
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

static bool check_arg_matches_form(slp_object_t *arg, form_definition_t *form) {
  if (!arg || !form) {
    return false;
  }

  form_type_e arg_form = sxs_forms_get_form_type(arg);

  for (size_t i = 0; i < form->type_count; i++) {
    if (form->types[i] == FORM_TYPE_ANY || form->types[i] == arg_form) {
      return true;
    }
  }

  return false;
}

sxs_callable_variant_t *find_matching_variant(sxs_callable_t *callable,
                                              slp_object_t **eval_args,
                                              size_t arg_count) {
  if (!callable) {
    return NULL;
  }

  for (size_t v = 0; v < callable->variant_count; v++) {
    if (callable->variants[v].param_count != arg_count) {
      continue;
    }

    bool all_match = true;
    for (size_t i = 0; i < arg_count; i++) {
      if (!callable->variants[v].params ||
          !callable->variants[v].params[i].form) {
        all_match = false;
        break;
      }
      if (!check_arg_matches_form(eval_args[i],
                                  callable->variants[v].params[i].form)) {
        all_match = false;
        break;
      }
    }

    if (all_match) {
      return &callable->variants[v];
    }
  }

  return NULL;
}

/*
==========================================================================
builtin implementations
==========================================================================
*/

/*
==========================================================================
get builtins as objects
==========================================================================
*/

static void sxs_init_load_store_callable(void) {
  if (g_builtin_load_store_callable) {
    return;
  }

  g_builtin_load_store_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_load_store_callable) {
    fprintf(stderr, "Failed to allocate callable for builtin\n");
    return;
  }

  g_builtin_load_store_callable->is_builtin = true;
  g_builtin_load_store_callable->variant_count = 3;

  g_builtin_load_store_callable->variants[0].param_count = 1;
  g_builtin_load_store_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 1);
  if (g_builtin_load_store_callable->variants[0].params) {
    g_builtin_load_store_callable->variants[0].params[0].name = NULL;
    g_builtin_load_store_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_INTEGER);
  }

  g_builtin_load_store_callable->variants[1].param_count = 2;
  g_builtin_load_store_callable->variants[1].params =
      malloc(sizeof(sxs_callable_param_t) * 2);
  if (g_builtin_load_store_callable->variants[1].params) {
    g_builtin_load_store_callable->variants[1].params[0].name = NULL;
    g_builtin_load_store_callable->variants[1].params[0].form =
        create_form_def(FORM_TYPE_INTEGER);
    g_builtin_load_store_callable->variants[1].params[1].name = NULL;
    g_builtin_load_store_callable->variants[1].params[1].form =
        create_form_def(FORM_TYPE_ANY);
  }

  g_builtin_load_store_callable->variants[2].param_count = 3;
  g_builtin_load_store_callable->variants[2].params =
      malloc(sizeof(sxs_callable_param_t) * 3);
  if (g_builtin_load_store_callable->variants[2].params) {
    g_builtin_load_store_callable->variants[2].params[0].name = NULL;
    g_builtin_load_store_callable->variants[2].params[0].form =
        create_form_def(FORM_TYPE_INTEGER);
    g_builtin_load_store_callable->variants[2].params[1].name = NULL;
    g_builtin_load_store_callable->variants[2].params[1].form =
        create_form_def(FORM_TYPE_ANY);
    g_builtin_load_store_callable->variants[2].params[2].name = NULL;
    g_builtin_load_store_callable->variants[2].params[2].form =
        create_form_def(FORM_TYPE_ANY);
  }

  g_builtin_load_store_callable->impl.builtin_fn = sxs_builtin_load_store;
}

static void sxs_deinit_load_store_callable(void) {
  if (g_builtin_load_store_callable) {
    sxs_callable_free(g_builtin_load_store_callable);
    g_builtin_load_store_callable = NULL;
  }
}

void sxs_builtins_init(void) { sxs_init_load_store_callable(); }

void sxs_builtins_deinit(void) { sxs_deinit_load_store_callable(); }

slp_object_t *sxs_get_builtin_load_store_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin load store object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_load_store_callable;

  return builtin;
}