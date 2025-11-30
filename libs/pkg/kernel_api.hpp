#pragma once

#include <cstddef>

#if defined(SXS_KERNEL_BUILD)
#include <slp/slp.hpp>
#else
#include <sxs/slp/slp.hpp>
#endif

namespace pkg::kernel {

using registry_t = void *;
using context_t = void *;

using kernel_fn_t = slp::slp_object_c (*)(context_t ctx,
                                          const slp::slp_object_c &args);

using register_fn_t = void (*)(registry_t registry, const char *name,
                               kernel_fn_t function,
                               slp::slp_type_e return_type, int variadic);

using eval_fn_t = slp::slp_object_c (*)(context_t ctx,
                                        const slp::slp_object_c &obj);

struct api_table_s {
  register_fn_t register_function;
  eval_fn_t eval;
};

} // namespace pkg::kernel

extern "C" {
void kernel_init(pkg::kernel::registry_t registry,
                 const pkg::kernel::api_table_s *api);
void kernel_shutdown(const pkg::kernel::api_table_s *api);
}
