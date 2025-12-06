#include "sxs/sxs.h"

#include <stdlib.h>

slp_object_t *sxs_builtin_proc(sxs_runtime_t *runtime, sxs_callable_t *callable,
                               slp_object_t **args, size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "proc builtin: nil runtime", 0, NULL);
  }

  if (arg_count != 2) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "proc builtin: expected 2 arguments", 0,
                                   runtime->source_buffer);
  }

  if (!args[0] || args[0]->type != SLP_TYPE_INTEGER) {
    size_t pos = args[0] ? args[0]->source_position : 0;
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "proc builtin: first arg must be integer",
                                   pos, runtime->source_buffer);
  }

  int64_t dest_index = args[0]->value.integer;

  if (dest_index < 0 || (size_t)dest_index >= SXS_OBJECT_STORAGE_SIZE) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, "proc builtin: index out of bounds",
        args[0]->source_position, runtime->source_buffer);
  }

  if (!args[1]) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "proc builtin: nil second argument", 0,
                                   runtime->source_buffer);
  }

  slp_object_t *body_arg = sxs_eval_object(runtime, args[1]);
  if (!body_arg) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, "proc builtin: eval failed on body",
        args[1]->source_position, runtime->source_buffer);
  }

  if (body_arg->type == SLP_TYPE_ERROR) {
    return body_arg;
  }

  if (body_arg->type != SLP_TYPE_LIST_C) {
    size_t pos = args[1]->source_position;
    slp_object_free(body_arg);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "proc builtin: second arg must be list-c",
                                   pos, runtime->source_buffer);
  }

  sxs_callable_t *lambda_callable = malloc(sizeof(sxs_callable_t));
  if (!lambda_callable) {
    slp_object_free(body_arg);
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   "proc builtin: failed to allocate callable",
                                   0, runtime->source_buffer);
  }

  lambda_callable->name = "proc";
  lambda_callable->is_builtin = false;
  lambda_callable->variant_count = 0;
  lambda_callable->impl.lambda_body = body_arg;
  lambda_callable->typecheck_fn = NULL;

  slp_object_t *lambda_obj = malloc(sizeof(slp_object_t));
  if (!lambda_obj) {
    slp_object_free(body_arg);
    free(lambda_callable);
    return sxs_create_error_object(
        SLP_ERROR_ALLOCATION, "proc builtin: failed to allocate lambda obj", 0,
        runtime->source_buffer);
  }

  lambda_obj->type = SLP_TYPE_LAMBDA;
  lambda_obj->value.fn_data = lambda_callable;
  lambda_obj->source_position = args[0]->source_position;

  if (runtime->object_storage[dest_index]) {
    slp_object_free(runtime->object_storage[dest_index]);
  }
  runtime->object_storage[dest_index] = lambda_obj;

  slp_object_t *none = malloc(sizeof(slp_object_t));
  if (!none) {
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   "proc builtin: failed to allocate none", 0,
                                   runtime->source_buffer);
  }
  none->type = SLP_TYPE_NONE;
  none->source_position = 0;

  return none;
}
