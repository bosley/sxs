#include "sxs/forms.h"
#include "sxs/sxs.h"

#include <stdlib.h>
#include <string.h>

extern sxs_callable_variant_t *find_matching_variant(sxs_callable_t *callable,
                                                     slp_object_t **eval_args,
                                                     size_t arg_count);

extern slp_object_t *
sxs_create_type_mismatch_error(const char *symbol, slp_object_t **eval_args,
                               size_t arg_count, sxs_callable_t *callable,
                               size_t error_position,
                               slp_buffer_unowned_ptr_t source_buffer);

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

slp_object_t *sxs_builtin_insist(sxs_runtime_t *runtime,
                                 sxs_callable_t *callable, slp_object_t **args,
                                 size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "insist builtin: nil runtime", 0, NULL);
  }

  if (!callable) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "insist builtin: nil callable", 0,
                                   runtime->source_buffer);
  }

  if (arg_count != 2) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "insist builtin: expected 2 arguments", 0,
                                   runtime->source_buffer);
  }

  slp_object_t *eval_args[2] = {NULL, NULL};
  for (size_t i = 0; i < arg_count; i++) {
    if (!args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "insist builtin: nil argument", 0,
                                     runtime->source_buffer);
    }
    eval_args[i] = sxs_eval_object(runtime, args[i]);
    if (!eval_args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "insist builtin: eval failed", 0,
                                     runtime->source_buffer);
    }
    if (eval_args[i]->type == SLP_TYPE_ERROR) {
      slp_object_t *error = eval_args[i];
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
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
        sxs_create_type_mismatch_error("insist", eval_args, arg_count, callable,
                                       error_position, runtime->source_buffer);

    for (size_t i = 0; i < arg_count; i++) {
      if (eval_args[i]) {
        slp_object_free(eval_args[i]);
      }
    }
    return error;
  }

  if (eval_args[0]->type != SLP_TYPE_SYMBOL) {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "insist: first argument must be a symbol", 0,
                                   runtime->source_buffer);
  }

  if (!eval_args[0]->value.buffer || eval_args[0]->value.buffer->count < 2 ||
      eval_args[0]->value.buffer->data[0] != ':') {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN,
        "insist: first argument must be a form symbol (e.g., :int, :real)", 0,
        runtime->source_buffer);
  }

  form_type_e expected_form = symbol_to_form_type(eval_args[0]->value.buffer);
  if (expected_form == FORM_TYPE_NONE &&
      eval_args[0]->value.buffer->count > 2) {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "insist: unknown form type", 0,
                                   runtime->source_buffer);
  }

  form_type_e actual_form = sxs_forms_get_form_type(eval_args[1]);

  if (expected_form != actual_form && expected_form != FORM_TYPE_ANY) {
    char error_msg[256];
    const char *expected_name = sxs_forms_get_form_type_name(expected_form);
    const char *actual_name = sxs_forms_get_form_type_name(actual_form);
    snprintf(error_msg, sizeof(error_msg),
             "insist: type mismatch - expected %s, got %s", expected_name,
             actual_name);

    size_t error_position = args[1]->source_position;

    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);

    slp_object_t *error =
        sxs_create_error_object(SLP_ERROR_PARSE_TOKEN, error_msg,
                                error_position, runtime->source_buffer);

    runtime->exception_active = true;

    return error;
  }

  slp_object_t *result = slp_object_copy(eval_args[1]);
  slp_object_free(eval_args[0]);
  slp_object_free(eval_args[1]);

  return result;
}
