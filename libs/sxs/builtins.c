#include "sxs/errors.h"
#include "sxs/impls/impls.h"
#include "sxs/sxs.h"
#include "sxs/typecheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
==========================================================================
This file serves to map together the implementations of the commands for
the system and symbols that are used to invoke them
This sort of "static dispatch" is setup so we can decide on startup
what commands we want to support and offer the ability to "bring your own
commands"
==========================================================================
*/

static sxs_callable_t *g_builtin_load_store_callable = NULL;
static sxs_callable_t *g_builtin_debug_callable = NULL;
static sxs_callable_t *g_builtin_rotl_callable = NULL;
static sxs_callable_t *g_builtin_rotr_callable = NULL;
static sxs_callable_t *g_builtin_insist_callable = NULL;
static sxs_callable_t *g_builtin_catch_callable = NULL;
static sxs_callable_t *g_builtin_proc_callable = NULL;
static sxs_callable_t *g_builtin_do_callable = NULL;

extern slp_object_t *sxs_builtin_load_store(sxs_runtime_t *runtime,
                                            sxs_callable_t *callable,
                                            slp_object_t **args,
                                            size_t arg_count);

extern slp_object_t *sxs_builtin_debug(sxs_runtime_t *runtime,
                                       sxs_callable_t *callable,
                                       slp_object_t **args, size_t arg_count);

extern slp_object_t *sxs_builtin_rotl(sxs_runtime_t *runtime,
                                      sxs_callable_t *callable,
                                      slp_object_t **args, size_t arg_count);

extern slp_object_t *sxs_builtin_rotr(sxs_runtime_t *runtime,
                                      sxs_callable_t *callable,
                                      slp_object_t **args, size_t arg_count);

extern slp_object_t *sxs_builtin_insist(sxs_runtime_t *runtime,
                                        sxs_callable_t *callable,
                                        slp_object_t **args, size_t arg_count);

extern slp_object_t *sxs_builtin_catch(sxs_runtime_t *runtime,
                                       sxs_callable_t *callable,
                                       slp_object_t **args, size_t arg_count);

extern slp_object_t *sxs_builtin_proc(sxs_runtime_t *runtime,
                                      sxs_callable_t *callable,
                                      slp_object_t **args, size_t arg_count);

extern slp_object_t *sxs_builtin_do(sxs_runtime_t *runtime,
                                    sxs_callable_t *callable,
                                    slp_object_t **args, size_t arg_count);

extern int sxs_typecheck_insist(sxs_typecheck_context_t *ctx,
                                sxs_callable_t *callable, slp_object_t **args,
                                size_t arg_count);

extern int sxs_typecheck_load_store(sxs_typecheck_context_t *ctx,
                                    sxs_callable_t *callable,
                                    slp_object_t **args, size_t arg_count);

extern int sxs_typecheck_proc(sxs_typecheck_context_t *ctx,
                              sxs_callable_t *callable, slp_object_t **args,
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

  def->is_variadic =
      (type == FORM_TYPE_INTEGER_VARIADIC || type == FORM_TYPE_REAL_VARIADIC ||
       type == FORM_TYPE_SYMBOL_VARIADIC || type == FORM_TYPE_LIST_S_VARIADIC ||
       type == FORM_TYPE_LIST_P_VARIADIC || type == FORM_TYPE_LIST_B_VARIADIC ||
       type == FORM_TYPE_LIST_C_VARIADIC || type == FORM_TYPE_SOME_VARIADIC ||
       type == FORM_TYPE_FN_VARIADIC || type == FORM_TYPE_ANY_VARIADIC);

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
Builtin function definitions are not something we copy around. Because of this
we opt to create a single object for the builtins that serves as the one slp
object in memory to handle calls.

Below is our init functions to ensure that the runtime type system knows
what types to expect from the callables

These builtins are to be moddeld such-that all return types are "any"
as each of them MAY produce an error given that they are runtime objects

If in the "C" implementation an "error" CAN be returned, then it disqualifies
the command from containing a strongly typed result

tags:
  return types, builtins, builtin functions, return type of builtin functions
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

  g_builtin_load_store_callable->name = "@";
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
  g_builtin_load_store_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY); // <-------------------------- Return ANY

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
  g_builtin_load_store_callable->variants[1].return_type =
      create_form_def(FORM_TYPE_ANY); // <-------------------------- Return ANY

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
  g_builtin_load_store_callable->variants[2].return_type =
      create_form_def(FORM_TYPE_ANY); // <-------------------------- Return ANY

  g_builtin_load_store_callable->impl.builtin_fn = sxs_builtin_load_store;
  g_builtin_load_store_callable->typecheck_fn = sxs_typecheck_load_store;
}

static void sxs_deinit_load_store_callable(void) {
  if (g_builtin_load_store_callable) {
    sxs_callable_free(g_builtin_load_store_callable);
    g_builtin_load_store_callable = NULL;
  }
}

static void sxs_init_debug_callable(void) {
  if (g_builtin_debug_callable) {
    return;
  }

  g_builtin_debug_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_debug_callable) {
    fprintf(stderr, "Failed to allocate callable for debug builtin\n");
    return;
  }

  g_builtin_debug_callable->name = "debug";
  g_builtin_debug_callable->is_builtin = true;
  g_builtin_debug_callable->variant_count = 1;

  g_builtin_debug_callable->variants[0].param_count = 1;
  g_builtin_debug_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 1);
  if (g_builtin_debug_callable->variants[0].params) {
    g_builtin_debug_callable->variants[0].params[0].name = NULL;
    g_builtin_debug_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_ANY_VARIADIC);
  }
  g_builtin_debug_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY);

  g_builtin_debug_callable->impl.builtin_fn = sxs_builtin_debug;
  g_builtin_debug_callable->typecheck_fn = sxs_typecheck_generic;
}

static void sxs_deinit_debug_callable(void) {
  if (g_builtin_debug_callable) {
    sxs_callable_free(g_builtin_debug_callable);
    g_builtin_debug_callable = NULL;
  }
}

static void sxs_init_rotl_callable(void) {
  if (g_builtin_rotl_callable) {
    return;
  }

  g_builtin_rotl_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_rotl_callable) {
    fprintf(stderr, "Failed to allocate callable for rotl builtin\n");
    return;
  }

  g_builtin_rotl_callable->name = "rotl";
  g_builtin_rotl_callable->is_builtin = true;
  g_builtin_rotl_callable->variant_count = 1;

  g_builtin_rotl_callable->variants[0].param_count = 2;
  g_builtin_rotl_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 2);
  if (g_builtin_rotl_callable->variants[0].params) {
    g_builtin_rotl_callable->variants[0].params[0].name = NULL;
    g_builtin_rotl_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_ANY);
    g_builtin_rotl_callable->variants[0].params[1].name = NULL;
    g_builtin_rotl_callable->variants[0].params[1].form =
        create_form_def(FORM_TYPE_INTEGER);
  }
  g_builtin_rotl_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY);

  g_builtin_rotl_callable->impl.builtin_fn = sxs_builtin_rotl;
  g_builtin_rotl_callable->typecheck_fn = sxs_typecheck_generic;
}

static void sxs_deinit_rotl_callable(void) {
  if (g_builtin_rotl_callable) {
    sxs_callable_free(g_builtin_rotl_callable);
    g_builtin_rotl_callable = NULL;
  }
}

static void sxs_init_rotr_callable(void) {
  if (g_builtin_rotr_callable) {
    return;
  }

  g_builtin_rotr_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_rotr_callable) {
    fprintf(stderr, "Failed to allocate callable for rotr builtin\n");
    return;
  }

  g_builtin_rotr_callable->name = "rotr";
  g_builtin_rotr_callable->is_builtin = true;
  g_builtin_rotr_callable->variant_count = 1;

  g_builtin_rotr_callable->variants[0].param_count = 2;
  g_builtin_rotr_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 2);
  if (g_builtin_rotr_callable->variants[0].params) {
    g_builtin_rotr_callable->variants[0].params[0].name = NULL;
    g_builtin_rotr_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_ANY);
    g_builtin_rotr_callable->variants[0].params[1].name = NULL;
    g_builtin_rotr_callable->variants[0].params[1].form =
        create_form_def(FORM_TYPE_INTEGER);
  }
  g_builtin_rotr_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY);

  g_builtin_rotr_callable->impl.builtin_fn = sxs_builtin_rotr;
  g_builtin_rotr_callable->typecheck_fn = sxs_typecheck_generic;
}

static void sxs_deinit_rotr_callable(void) {
  if (g_builtin_rotr_callable) {
    sxs_callable_free(g_builtin_rotr_callable);
    g_builtin_rotr_callable = NULL;
  }
}

static void sxs_init_insist_callable(void) {
  if (g_builtin_insist_callable) {
    return;
  }

  g_builtin_insist_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_insist_callable) {
    fprintf(stderr, "Failed to allocate callable for insist builtin\n");
    return;
  }

  g_builtin_insist_callable->name = "insist";
  g_builtin_insist_callable->is_builtin = true;
  g_builtin_insist_callable->variant_count = 1;

  g_builtin_insist_callable->variants[0].param_count = 2;
  g_builtin_insist_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 2);
  if (g_builtin_insist_callable->variants[0].params) {
    g_builtin_insist_callable->variants[0].params[0].name = NULL;
    g_builtin_insist_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_SYMBOL);
    g_builtin_insist_callable->variants[0].params[1].name = NULL;
    g_builtin_insist_callable->variants[0].params[1].form =
        create_form_def(FORM_TYPE_ANY);
  }
  g_builtin_insist_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY);

  g_builtin_insist_callable->impl.builtin_fn = sxs_builtin_insist;
  g_builtin_insist_callable->typecheck_fn = sxs_typecheck_insist;
}

static void sxs_deinit_insist_callable(void) {
  if (g_builtin_insist_callable) {
    sxs_callable_free(g_builtin_insist_callable);
    g_builtin_insist_callable = NULL;
  }
}

static void sxs_init_catch_callable(void) {
  if (g_builtin_catch_callable) {
    return;
  }

  g_builtin_catch_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_catch_callable) {
    fprintf(stderr, "Failed to allocate callable for catch builtin\n");
    return;
  }

  g_builtin_catch_callable->name = "catch";
  g_builtin_catch_callable->is_builtin = true;
  g_builtin_catch_callable->variant_count = 1;

  g_builtin_catch_callable->variants[0].param_count = 1;
  g_builtin_catch_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 1);
  if (g_builtin_catch_callable->variants[0].params) {
    g_builtin_catch_callable->variants[0].params[0].name = NULL;
    g_builtin_catch_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_ANY_VARIADIC);
  }
  g_builtin_catch_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY);

  g_builtin_catch_callable->impl.builtin_fn = sxs_builtin_catch;
  g_builtin_catch_callable->typecheck_fn = sxs_typecheck_generic;
}

static void sxs_deinit_catch_callable(void) {
  if (g_builtin_catch_callable) {
    sxs_callable_free(g_builtin_catch_callable);
    g_builtin_catch_callable = NULL;
  }
}

static void sxs_init_proc_callable(void) {
  if (g_builtin_proc_callable) {
    return;
  }

  g_builtin_proc_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_proc_callable) {
    fprintf(stderr, "Failed to allocate callable for proc builtin\n");
    return;
  }

  g_builtin_proc_callable->name = "proc";
  g_builtin_proc_callable->is_builtin = true;
  g_builtin_proc_callable->variant_count = 1;

  g_builtin_proc_callable->variants[0].param_count = 2;
  g_builtin_proc_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 2);
  if (g_builtin_proc_callable->variants[0].params) {
    g_builtin_proc_callable->variants[0].params[0].name = NULL;
    g_builtin_proc_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_INTEGER);
    g_builtin_proc_callable->variants[0].params[1].name = NULL;
    g_builtin_proc_callable->variants[0].params[1].form =
        create_form_def(FORM_TYPE_LIST_C);
  }
  g_builtin_proc_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_NONE);

  g_builtin_proc_callable->impl.builtin_fn = sxs_builtin_proc;
  g_builtin_proc_callable->typecheck_fn = sxs_typecheck_proc;
}

static void sxs_deinit_proc_callable(void) {
  if (g_builtin_proc_callable) {
    sxs_callable_free(g_builtin_proc_callable);
    g_builtin_proc_callable = NULL;
  }
}

static void sxs_init_do_callable(void) {
  if (g_builtin_do_callable) {
    return;
  }

  g_builtin_do_callable = malloc(sizeof(sxs_callable_t));
  if (!g_builtin_do_callable) {
    fprintf(stderr, "Failed to allocate callable for do builtin\n");
    return;
  }

  g_builtin_do_callable->name = "do";
  g_builtin_do_callable->is_builtin = true;
  g_builtin_do_callable->variant_count = 1;

  g_builtin_do_callable->variants[0].param_count = 1;
  g_builtin_do_callable->variants[0].params =
      malloc(sizeof(sxs_callable_param_t) * 1);
  if (g_builtin_do_callable->variants[0].params) {
    g_builtin_do_callable->variants[0].params[0].name = NULL;
    g_builtin_do_callable->variants[0].params[0].form =
        create_form_def(FORM_TYPE_INTEGER);
  }
  g_builtin_do_callable->variants[0].return_type =
      create_form_def(FORM_TYPE_ANY);

  g_builtin_do_callable->impl.builtin_fn = sxs_builtin_do;
  g_builtin_do_callable->typecheck_fn = sxs_typecheck_generic;
}

static void sxs_deinit_do_callable(void) {
  if (g_builtin_do_callable) {
    sxs_callable_free(g_builtin_do_callable);
    g_builtin_do_callable = NULL;
  }
}

void sxs_builtins_init(void) {
  sxs_init_load_store_callable();
  sxs_init_debug_callable();
  sxs_init_rotl_callable();
  sxs_init_rotr_callable();
  sxs_init_insist_callable();
  sxs_init_catch_callable();
  sxs_init_proc_callable();
  sxs_init_do_callable();
}

void sxs_builtins_deinit(void) {
  sxs_deinit_load_store_callable();
  sxs_deinit_debug_callable();
  sxs_deinit_rotl_callable();
  sxs_deinit_rotr_callable();
  sxs_deinit_insist_callable();
  sxs_deinit_catch_callable();
  sxs_deinit_proc_callable();
  sxs_deinit_do_callable();
}

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

slp_object_t *sxs_get_builtin_debug_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin debug object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_debug_callable;

  return builtin;
}

slp_object_t *sxs_get_builtin_rotl_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin rotl object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_rotl_callable;

  return builtin;
}

slp_object_t *sxs_get_builtin_rotr_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin rotr object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_rotr_callable;

  return builtin;
}

slp_object_t *sxs_get_builtin_insist_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin insist object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_insist_callable;

  return builtin;
}

slp_object_t *sxs_get_builtin_catch_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin catch object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_catch_callable;

  return builtin;
}

slp_object_t *sxs_get_builtin_proc_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin proc object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_proc_callable;

  return builtin;
}

slp_object_t *sxs_get_builtin_do_object(void) {
  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(stderr, "Failed to get builtin do object (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)g_builtin_do_callable;

  return builtin;
}

sxs_command_impl_t sxs_impl_get_load_store(void) {
  sxs_command_impl_t impl;
  impl.command = "@";
  impl.handler = sxs_builtin_load_store;
  return impl;
}

sxs_command_impl_t sxs_impl_get_debug(void) {
  sxs_command_impl_t impl;
  impl.command = "debug";
  impl.handler = sxs_builtin_debug;
  return impl;
}

sxs_command_impl_t sxs_impl_get_rotl(void) {
  sxs_command_impl_t impl;
  impl.command = "rotl";
  impl.handler = sxs_builtin_rotl;
  return impl;
}

sxs_command_impl_t sxs_impl_get_rotr(void) {
  sxs_command_impl_t impl;
  impl.command = "rotr";
  impl.handler = sxs_builtin_rotr;
  return impl;
}

sxs_command_impl_t sxs_impl_get_insist(void) {
  sxs_command_impl_t impl;
  impl.command = "insist";
  impl.handler = sxs_builtin_insist;
  return impl;
}

sxs_command_impl_t sxs_impl_get_catch(void) {
  sxs_command_impl_t impl;
  impl.command = "catch";
  impl.handler = sxs_builtin_catch;
  return impl;
}

sxs_command_impl_t sxs_impl_get_proc(void) {
  sxs_command_impl_t impl;
  impl.command = "proc";
  impl.handler = sxs_builtin_proc;
  return impl;
}

sxs_command_impl_t sxs_impl_get_do(void) {
  sxs_command_impl_t impl;
  impl.command = "do";
  impl.handler = sxs_builtin_do;
  return impl;
}

sxs_callable_t *sxs_get_callable_for_handler(sxs_handler_fn_t handler) {
  if (handler == sxs_builtin_load_store) {
    return g_builtin_load_store_callable;
  } else if (handler == sxs_builtin_debug) {
    return g_builtin_debug_callable;
  } else if (handler == sxs_builtin_rotl) {
    return g_builtin_rotl_callable;
  } else if (handler == sxs_builtin_rotr) {
    return g_builtin_rotr_callable;
  } else if (handler == sxs_builtin_insist) {
    return g_builtin_insist_callable;
  } else if (handler == sxs_builtin_catch) {
    return g_builtin_catch_callable;
  } else if (handler == sxs_builtin_proc) {
    return g_builtin_proc_callable;
  } else if (handler == sxs_builtin_do) {
    return g_builtin_do_callable;
  }
  return NULL;
}