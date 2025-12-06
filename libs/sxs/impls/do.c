#include "sxs/sxs.h"

#include <stdlib.h>

slp_object_t *sxs_builtin_do(sxs_runtime_t *runtime, sxs_callable_t *callable,
                             slp_object_t **args, size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: nil runtime", 0, NULL);
  }

  if (arg_count != 1) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: expected 1 argument", 0,
                                   runtime->source_buffer);
  }

  if (!args[0]) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: nil argument", 0,
                                   runtime->source_buffer);
  }

  slp_object_t *index_obj = sxs_eval_object(runtime, args[0]);
  if (!index_obj) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, "do builtin: eval failed",
        args[0]->source_position, runtime->source_buffer);
  }

  if (index_obj->type == SLP_TYPE_ERROR) {
    return index_obj;
  }

  if (index_obj->type != SLP_TYPE_INTEGER) {
    size_t pos = args[0]->source_position;
    slp_object_free(index_obj);
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: argument must be integer", pos,
                                   runtime->source_buffer);
  }

  int64_t proc_index = index_obj->value.integer;
  slp_object_free(index_obj);

  if (proc_index < 0 || (size_t)proc_index >= SXS_OBJECT_STORAGE_SIZE) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, "do builtin: index out of bounds",
        args[0]->source_position, runtime->source_buffer);
  }

  slp_object_t *proc_obj = runtime->object_storage[proc_index];
  if (!proc_obj) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, "do builtin: no proc at index",
        args[0]->source_position, runtime->source_buffer);
  }

  if (proc_obj->type != SLP_TYPE_LAMBDA) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, "do builtin: object is not a lambda",
        args[0]->source_position, runtime->source_buffer);
  }

  sxs_callable_t *lambda_callable = (sxs_callable_t *)proc_obj->value.fn_data;
  if (!lambda_callable) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: nil lambda callable", 0,
                                   runtime->source_buffer);
  }

  slp_object_t *lambda_body = lambda_callable->impl.lambda_body;
  if (!lambda_body) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: nil lambda body", 0,
                                   runtime->source_buffer);
  }

  if (lambda_body->type != SLP_TYPE_LIST_C) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "do builtin: lambda body is not list-c", 0,
                                   runtime->source_buffer);
  }

  if (lambda_body->value.list.count == 0) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (!none) {
      return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                     "do builtin: failed to allocate none", 0,
                                     runtime->source_buffer);
    }
    none->type = SLP_TYPE_NONE;
    none->source_position = 0;
    return none;
  }

  slp_object_t *last_result = NULL;

  for (size_t i = 0; i < lambda_body->value.list.count; i++) {
    if (last_result) {
      slp_object_free(last_result);
      last_result = NULL;
    }

    slp_object_t *item = lambda_body->value.list.items[i];
    if (!item) {
      return sxs_create_error_object(
          SLP_ERROR_PARSE_TOKEN, "do builtin: nil item in lambda body",
          args[0]->source_position, runtime->source_buffer);
    }

    last_result = sxs_eval_object(runtime, item);
    if (!last_result) {
      return sxs_create_error_object(
          SLP_ERROR_PARSE_TOKEN, "do builtin: eval failed on item",
          item->source_position, runtime->source_buffer);
    }

    if (runtime->exception_active) {
      return last_result;
    }
  }

  if (!last_result) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (!none) {
      return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                     "do builtin: failed to allocate none", 0,
                                     runtime->source_buffer);
    }
    none->type = SLP_TYPE_NONE;
    none->source_position = 0;
    return none;
  }

  return last_result;
}
