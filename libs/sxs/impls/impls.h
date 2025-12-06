#ifndef SXS_IMPLS_H
#define SXS_IMPLS_H

#include "sxs/sxs.h"

sxs_command_impl_t sxs_impl_get_load_store(void);
sxs_command_impl_t sxs_impl_get_debug(void);
sxs_command_impl_t sxs_impl_get_rotl(void);
sxs_command_impl_t sxs_impl_get_rotr(void);

slp_object_t *sxs_builtin_load_store(sxs_runtime_t *runtime,
                                     sxs_callable_t *callable,
                                     slp_object_t **args, size_t arg_count);

slp_object_t *sxs_builtin_debug(sxs_runtime_t *runtime,
                                sxs_callable_t *callable, slp_object_t **args,
                                size_t arg_count);

slp_object_t *sxs_builtin_rotl(sxs_runtime_t *runtime, sxs_callable_t *callable,
                               slp_object_t **args, size_t arg_count);

slp_object_t *sxs_builtin_rotr(sxs_runtime_t *runtime, sxs_callable_t *callable,
                               slp_object_t **args, size_t arg_count);

#endif
