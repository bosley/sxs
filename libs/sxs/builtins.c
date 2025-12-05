#include "sxs/errors.h"
#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sxs_callable_t *g_builtin_load_store_callable = NULL;

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

static sxs_callable_variant_t *find_matching_variant(sxs_callable_t *callable,
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
function: @

acts as setter:     (@ <dest> <source>)
acts as getter:     (@ <source>)
acts as atomic CAS: (@ <dest> <compare_val> <source>)

Each argument is evaluated before checking for type

@ :int :int
@ :int
@ :int <any> :int


*/
static slp_object_t *sxs_builtin_load_store(sxs_runtime_t *runtime,
                                            sxs_callable_t *callable,
                                            slp_object_t **args,
                                            size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "@ builtin: nil runtime", 0, NULL);
  }

  if (!callable) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "@ builtin: nil callable", 0,
                                   runtime->source_buffer);
  }

  slp_object_t *eval_args[3] = {NULL, NULL, NULL};
  for (size_t i = 0; i < arg_count; i++) {
    if (!args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_free_object(eval_args[j]);
        }
      }
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ builtin: nil argument", 0,
                                     runtime->source_buffer);
    }
    eval_args[i] = sxs_eval_object(runtime, args[i]);
    if (!eval_args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_free_object(eval_args[j]);
        }
      }
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ builtin: eval failed", 0,
                                     runtime->source_buffer);
    }
    if (eval_args[i]->type == SLP_TYPE_ERROR) {
      slp_object_t *error = eval_args[i];
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_free_object(eval_args[j]);
        }
      }
      return error;
    }
  }

  sxs_callable_variant_t *variant =
      find_matching_variant(callable, eval_args, arg_count);
  if (!variant) {
    size_t error_position = arg_count > 0 ? args[0]->source_position : 0;
    slp_object_t *error =
        sxs_create_type_mismatch_error("@", eval_args, arg_count, callable,
                                       error_position, runtime->source_buffer);

    for (size_t i = 0; i < arg_count; i++) {
      if (eval_args[i]) {
        slp_free_object(eval_args[i]);
      }
    }
    return error;
  }

  /*
  GET: (@ <integer) -> returns copy of new (current) object or NONE if nil
  */
  if (arg_count == 1) {
    int64_t index = eval_args[0]->value.integer;
    slp_free_object(eval_args[0]);

    if (index < 0 || (size_t)index >= SXS_OBJECT_STORAGE_SIZE) {
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ getter: index out of bounds", 0,
                                     runtime->source_buffer);
    }

    slp_object_t *stored = runtime->object_storage[index];
    if (!stored) {
      slp_object_t *none = malloc(sizeof(slp_object_t));
      if (none) {
        none->type = SLP_TYPE_NONE;
      }
      return none;
    }

    return slp_object_copy(stored);
  }

  /*
  SET: (@ <integer> <any>) -> returns copy of value-in (for chaining)
  */
  if (arg_count == 2) {
    int64_t dest_index = eval_args[0]->value.integer;
    slp_free_object(eval_args[0]);

    if (dest_index < 0 || (size_t)dest_index >= SXS_OBJECT_STORAGE_SIZE) {
      slp_free_object(eval_args[1]);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ setter: dest index out of bounds", 0,
                                     runtime->source_buffer);
    }

    if (runtime->object_storage[dest_index]) {
      slp_free_object(runtime->object_storage[dest_index]);
    }

    runtime->object_storage[dest_index] = slp_object_copy(eval_args[1]);
    slp_object_t *result = slp_object_copy(eval_args[1]);
    slp_free_object(eval_args[1]);
    return result;
  }

  /*
  SET: (@ <integer> <any> <any>) -> returns copy of  value
  */
  if (arg_count == 3) {
    int64_t dest_index = eval_args[0]->value.integer;
    slp_free_object(eval_args[0]);

    if (dest_index < 0 || (size_t)dest_index >= SXS_OBJECT_STORAGE_SIZE) {
      slp_free_object(eval_args[1]);
      slp_free_object(eval_args[2]);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ CAS: dest index out of bounds", 0,
                                     runtime->source_buffer);
    }

    slp_object_t *current = runtime->object_storage[dest_index];
    slp_object_t *compare_val = eval_args[1];
    slp_object_t *new_val = eval_args[2];

    bool should_swap = false;
    if (!current && compare_val->type == SLP_TYPE_NONE) {
      should_swap = true;
    } else if (current && compare_val->type == current->type) {
      if (current->type == SLP_TYPE_INTEGER) {
        should_swap = (current->value.integer == compare_val->value.integer);
      } else if (current->type == SLP_TYPE_REAL) {
        should_swap = (current->value.real == compare_val->value.real);
      }
    }

    slp_object_t *old_value = NULL;
    if (current) {
      old_value = slp_object_copy(current);
    } else {
      old_value = malloc(sizeof(slp_object_t));
      if (old_value) {
        old_value->type = SLP_TYPE_NONE;
      }
    }

    if (should_swap) {
      if (runtime->object_storage[dest_index]) {
        slp_free_object(runtime->object_storage[dest_index]);
      }
      runtime->object_storage[dest_index] = slp_object_copy(new_val);
    }

    slp_free_object(compare_val);
    slp_free_object(new_val);

    return old_value;
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (eval_args[i]) {
      slp_free_object(eval_args[i]);
    }
  }
  return sxs_create_error_object(
      SLP_ERROR_PARSE_TOKEN,
      "@ builtin: invalid arg count (expected 1, 2, or 3)", 0,
      runtime->source_buffer);
}

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