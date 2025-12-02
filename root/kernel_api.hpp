#pragma once

#include <cstddef>

#if defined(SXS_KERNEL_BUILD)
#include <slp/slp.hpp>
#else
#include <slp/slp.hpp>
#endif

namespace pkg::kernel {

using registry_t = void *;
using context_t = void *;
using system_t = void *;

struct system_info_s {
  const char *root_working_path;
};

using kernel_fn_t = slp::slp_object_c (*)(context_t ctx,
                                          const slp::slp_object_c &args);

using register_fn_t = void (*)(registry_t registry, const char *name,
                               kernel_fn_t function,
                               slp::slp_type_e return_type, int variadic);

using eval_fn_t = slp::slp_object_c (*)(context_t ctx,
                                        const slp::slp_object_c &obj);

using get_system_info_fn_t = const system_info_s *(*)(system_t sys);

struct api_table_s {
  register_fn_t register_function;
  eval_fn_t eval;
  get_system_info_fn_t get_system_info;
  system_t system;
};

} // namespace pkg::kernel

extern "C" {
void kernel_init(pkg::kernel::registry_t registry,
                 const pkg::kernel::api_table_s *api);
void kernel_shutdown(const pkg::kernel::api_table_s *api);
}
