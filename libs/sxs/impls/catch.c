#include "sxs/sxs.h"

#include <stdlib.h>

slp_object_t *sxs_builtin_catch(sxs_runtime_t *runtime,
                                sxs_callable_t *callable, slp_object_t **args,
                                size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "catch builtin: nil runtime", 0, NULL);
  }

  if (arg_count == 0) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
      none->source_position = 0;
    }
    return none;
  }

  slp_object_t *last_result = NULL;

  for (size_t i = 0; i < arg_count; i++) {
    if (!args[i]) {
      if (last_result) {
        slp_object_free(last_result);
      }
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "catch builtin: nil argument", 0,
                                     runtime->source_buffer);
    }

    slp_object_t *eval_result = sxs_eval_object(runtime, args[i]);
    if (!eval_result) {
      if (last_result) {
        slp_object_free(last_result);
      }
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "catch builtin: eval failed", 0,
                                     runtime->source_buffer);
    }

    if (eval_result->type == SLP_TYPE_ERROR) {
      if (runtime->exception_active) {
        runtime->exception_active = false;

        if (last_result) {
          slp_object_free(last_result);
        }

        return eval_result;
      } else {
        if (last_result) {
          slp_object_free(last_result);
        }
        return eval_result;
      }
    }

    if (last_result) {
      slp_object_free(last_result);
    }
    last_result = eval_result;
  }

  return last_result;
}
