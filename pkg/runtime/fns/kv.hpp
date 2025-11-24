#pragma once
#include "runtime/fns/fns.hpp"

namespace runtime::fns {

function_group_s get_kv_functions(
    logger_t logger,
    std::function<slp::slp_object_c(session_c*, const slp::slp_object_c&, const processor_c::eval_context_s&)> eval_fn,
    std::function<std::string(const slp::slp_object_c&)> to_string_fn
);

}

