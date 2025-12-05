#include "sxs/sxs.h"

#include <stdlib.h>

extern sxs_callable_variant_t *find_matching_variant(sxs_callable_t *callable,
                                                     slp_object_t **eval_args,
                                                     size_t arg_count);

extern slp_object_t *
sxs_create_type_mismatch_error(const char *symbol, slp_object_t **eval_args,
                               size_t arg_count, sxs_callable_t *callable,
                               size_t error_position,
                               slp_buffer_unowned_ptr_t source_buffer);
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
slp_object_t *sxs_builtin_load_store(sxs_runtime_t *runtime,
                                     sxs_callable_t *callable,
                                     slp_object_t **args, size_t arg_count) {
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
          slp_object_free(eval_args[j]);
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
          slp_object_free(eval_args[j]);
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
        sxs_create_type_mismatch_error("@", eval_args, arg_count, callable,
                                       error_position, runtime->source_buffer);

    for (size_t i = 0; i < arg_count; i++) {
      if (eval_args[i]) {
        slp_object_free(eval_args[i]);
      }
    }
    return error;
  }

  /*
  GET: (@ <integer) -> returns copy of new (current) object or NONE if nil
  */
  if (arg_count == 1) {
    int64_t index = eval_args[0]->value.integer;
    slp_object_free(eval_args[0]);

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
    slp_object_free(eval_args[0]);

    if (dest_index < 0 || (size_t)dest_index >= SXS_OBJECT_STORAGE_SIZE) {
      slp_object_free(eval_args[1]);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ setter: dest index out of bounds", 0,
                                     runtime->source_buffer);
    }

    if (runtime->object_storage[dest_index]) {
      slp_object_free(runtime->object_storage[dest_index]);
    }

    runtime->object_storage[dest_index] = slp_object_copy(eval_args[1]);
    slp_object_t *result = slp_object_copy(eval_args[1]);
    slp_object_free(eval_args[1]);
    return result;
  }

  /*
  SET: (@ <integer> <any> <any>) -> returns copy of  value
  */
  if (arg_count == 3) {
    int64_t dest_index = eval_args[0]->value.integer;
    slp_object_free(eval_args[0]);

    if (dest_index < 0 || (size_t)dest_index >= SXS_OBJECT_STORAGE_SIZE) {
      slp_object_free(eval_args[1]);
      slp_object_free(eval_args[2]);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "@ CAS: dest index out of bounds", 0,
                                     runtime->source_buffer);
    }

    slp_object_t *current = runtime->object_storage[dest_index];
    slp_object_t *compare_val = eval_args[1];
    slp_object_t *new_val = eval_args[2];

    bool should_swap = slp_objects_equal(current, compare_val);

    if (should_swap) {
      if (runtime->object_storage[dest_index]) {
        slp_object_free(runtime->object_storage[dest_index]);
      }
      runtime->object_storage[dest_index] = slp_object_copy(new_val);
    }

    slp_object_free(compare_val);
    slp_object_free(new_val);

    slp_object_t *result = malloc(sizeof(slp_object_t));
    if (!result) {
      return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                     "@ CAS: failed to allocate result", 0,
                                     runtime->source_buffer);
    }
    result->type = SLP_TYPE_INTEGER;
    result->value.integer = should_swap ? 1 : 0;
    result->source_position = 0;

    return result;
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (eval_args[i]) {
      slp_object_free(eval_args[i]);
    }
  }
  return sxs_create_error_object(
      SLP_ERROR_PARSE_TOKEN,
      "@ builtin: invalid arg count (expected 1, 2, or 3)", 0,
      runtime->source_buffer);
}
