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

static bool is_list_type(slp_type_e type) {
  return type == SLP_TYPE_LIST_P || type == SLP_TYPE_LIST_B ||
         type == SLP_TYPE_LIST_C;
}

static slp_object_t *rotate_list(slp_object_t *list, int64_t rotation,
                                 bool rotate_left, sxs_runtime_t *runtime) {
  if (!list || !is_list_type(list->type)) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotate: first argument must be a list", 0,
                                   runtime->source_buffer);
  }

  if (list->value.list.count == 0) {
    return slp_object_copy(list);
  }

  size_t count = list->value.list.count;
  int64_t effective_rotation = rotation % (int64_t)count;

  if (effective_rotation < 0) {
    effective_rotation += (int64_t)count;
  }

  if (!rotate_left) {
    effective_rotation = (int64_t)count - effective_rotation;
    if (effective_rotation == (int64_t)count) {
      effective_rotation = 0;
    }
  }

  slp_object_t *result = malloc(sizeof(slp_object_t));
  if (!result) {
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   "rotate: failed to allocate result", 0,
                                   runtime->source_buffer);
  }

  result->type = list->type;
  result->source_position = list->source_position;
  result->value.list.count = count;
  result->value.list.items = malloc(sizeof(slp_object_t *) * count);

  if (!result->value.list.items) {
    free(result);
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   "rotate: failed to allocate list items", 0,
                                   runtime->source_buffer);
  }

  for (size_t i = 0; i < count; i++) {
    size_t src_index = ((size_t)effective_rotation + i) % count;
    result->value.list.items[i] =
        slp_object_copy(list->value.list.items[src_index]);
    if (!result->value.list.items[i]) {
      for (size_t j = 0; j < i; j++) {
        slp_object_free(result->value.list.items[j]);
      }
      free(result->value.list.items);
      free(result);
      return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                     "rotate: failed to copy list item", 0,
                                     runtime->source_buffer);
    }
  }

  return result;
}

slp_object_t *sxs_builtin_rotl(sxs_runtime_t *runtime, sxs_callable_t *callable,
                               slp_object_t **args, size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotl builtin: nil runtime", 0, NULL);
  }

  if (!callable) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotl builtin: nil callable", 0,
                                   runtime->source_buffer);
  }

  if (arg_count != 2) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotl builtin: expected 2 arguments", 0,
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
                                     "rotl builtin: nil argument", 0,
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
                                     "rotl builtin: eval failed", 0,
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
        sxs_create_type_mismatch_error("rotl", eval_args, arg_count, callable,
                                       error_position, runtime->source_buffer);

    for (size_t i = 0; i < arg_count; i++) {
      if (eval_args[i]) {
        slp_object_free(eval_args[i]);
      }
    }
    return error;
  }

  if (!is_list_type(eval_args[0]->type)) {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotl: first argument must be a list", 0,
                                   runtime->source_buffer);
  }

  if (eval_args[1]->type != SLP_TYPE_INTEGER) {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotl: second argument must be an integer",
                                   0, runtime->source_buffer);
  }

  int64_t rotation = eval_args[1]->value.integer;
  slp_object_t *result = rotate_list(eval_args[0], rotation, true, runtime);

  slp_object_free(eval_args[0]);
  slp_object_free(eval_args[1]);

  return result;
}

slp_object_t *sxs_builtin_rotr(sxs_runtime_t *runtime, sxs_callable_t *callable,
                               slp_object_t **args, size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotr builtin: nil runtime", 0, NULL);
  }

  if (!callable) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotr builtin: nil callable", 0,
                                   runtime->source_buffer);
  }

  if (arg_count != 2) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotr builtin: expected 2 arguments", 0,
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
                                     "rotr builtin: nil argument", 0,
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
                                     "rotr builtin: eval failed", 0,
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
        sxs_create_type_mismatch_error("rotr", eval_args, arg_count, callable,
                                       error_position, runtime->source_buffer);

    for (size_t i = 0; i < arg_count; i++) {
      if (eval_args[i]) {
        slp_object_free(eval_args[i]);
      }
    }
    return error;
  }

  if (!is_list_type(eval_args[0]->type)) {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotr: first argument must be a list", 0,
                                   runtime->source_buffer);
  }

  if (eval_args[1]->type != SLP_TYPE_INTEGER) {
    slp_object_free(eval_args[0]);
    slp_object_free(eval_args[1]);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "rotr: second argument must be an integer",
                                   0, runtime->source_buffer);
  }

  int64_t rotation = eval_args[1]->value.integer;
  slp_object_t *result = rotate_list(eval_args[0], rotation, false, runtime);

  slp_object_free(eval_args[0]);
  slp_object_free(eval_args[1]);

  return result;
}
