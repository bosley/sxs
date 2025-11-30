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

using create_int_fn_t = slp::slp_object_c (*)(long long value);
using create_real_fn_t = slp::slp_object_c (*)(double value);
using create_string_fn_t = slp::slp_object_c (*)(const char *value);
using create_none_fn_t = slp::slp_object_c (*)();
using create_symbol_fn_t = slp::slp_object_c (*)(const char *name);
using create_paren_list_fn_t =
    slp::slp_object_c (*)(const slp::slp_object_c *objects, size_t count);
using create_bracket_list_fn_t =
    slp::slp_object_c (*)(const slp::slp_object_c *objects, size_t count);
using create_brace_list_fn_t =
    slp::slp_object_c (*)(const slp::slp_object_c *objects, size_t count);

struct api_table_s {
  register_fn_t register_function;
  eval_fn_t eval;
  create_int_fn_t create_int;
  create_real_fn_t create_real;
  create_string_fn_t create_string;
  create_none_fn_t create_none;
  create_symbol_fn_t create_symbol;
  create_paren_list_fn_t create_paren_list;
  create_bracket_list_fn_t create_bracket_list;
  create_brace_list_fn_t create_brace_list;
};

} // namespace pkg::kernel

extern "C" {
void kernel_init(pkg::kernel::registry_t registry,
                 const pkg::kernel::api_table_s *api);
void kernel_shutdown(const pkg::kernel::api_table_s *api);
}
