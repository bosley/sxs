#include "errors.h"
#include "forms.h"
#include "sxs.h"
#include <stdio.h>
#include <string.h>

slp_object_t *
sxs_create_type_mismatch_error(const char *function_name,
                               slp_object_t **eval_args, size_t arg_count,
                               sxs_callable_t *callable, size_t error_position,
                               slp_buffer_unowned_ptr_t source_buffer) {

  char error_msg[1024];
  int offset = snprintf(error_msg, sizeof(error_msg), "%s builtin|received (",
                        function_name);

  // Build received types string
  for (size_t i = 0; i < arg_count && offset < (int)sizeof(error_msg) - 1;
       i++) {
    form_type_e arg_form = sxs_forms_get_form_type(eval_args[i]);
    const char *form_name = sxs_forms_get_form_type_name(arg_form);
    if (i > 0) {
      offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, " ");
    }
    offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, "%s",
                       form_name);
  }

  offset +=
      snprintf(error_msg + offset, sizeof(error_msg) - offset, ")|expected ");

  // Build expected types string from callable variants
  for (size_t v = 0;
       v < callable->variant_count && offset < (int)sizeof(error_msg) - 1;
       v++) {
    if (v > 0) {
      offset +=
          snprintf(error_msg + offset, sizeof(error_msg) - offset, " or ");
    }
    offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, "(");
    for (size_t p = 0; p < callable->variants[v].param_count &&
                       offset < (int)sizeof(error_msg) - 1;
         p++) {
      if (p > 0) {
        offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, " ");
      }
      if (callable->variants[v].params &&
          callable->variants[v].params[p].form) {
        for (size_t t = 0; t < callable->variants[v].params[p].form->type_count;
             t++) {
          if (t > 0) {
            offset +=
                snprintf(error_msg + offset, sizeof(error_msg) - offset, "|");
          }
          const char *type_name = sxs_forms_get_form_type_name(
              callable->variants[v].params[p].form->types[t]);
          offset += snprintf(error_msg + offset, sizeof(error_msg) - offset,
                             "%s", type_name);
        }
      }
    }
    offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, ")");
  }

  return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN, error_msg,
                                 error_position, source_buffer);
}
