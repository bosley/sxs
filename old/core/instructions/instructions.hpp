#pragma once

#include "core/interpreter.hpp"
#include "slp/slp.hpp"
#include <functional>
#include <map>
#include <string>

namespace pkg::core::instructions {

typedef std::function<slp::slp_object_c(callable_context_if &context,
                                        slp::slp_object_c &args_list)>
    instruction_interpreter_fn_t;

extern std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols();

} // namespace pkg::core::instructions