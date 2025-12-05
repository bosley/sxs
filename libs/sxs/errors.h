#ifndef SXS_ERRORS_H
#define SXS_ERRORS_H

#include "slp/slp.h"
#include "sxs.h"

slp_object_t *
sxs_create_type_mismatch_error(const char *function_name,
                               slp_object_t **eval_args, size_t arg_count,
                               sxs_callable_t *callable, size_t error_position,
                               slp_buffer_unowned_ptr_t source_buffer);

#endif // SXS_ERRORS_H
