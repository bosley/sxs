#pragma once
#include "runtime/fns/fns.hpp"
#include <mutex>
#include <vector>

namespace runtime::fns {

function_group_s get_event_functions(
    logger_t logger,
    std::function<slp::slp_object_c(session_c*, const slp::slp_object_c&, const processor_c::eval_context_s&)> eval_fn,
    std::function<std::string(const slp::slp_object_c&)> to_string_fn,
    std::vector<processor_c::subscription_handler_s> *subscription_handlers,
    std::mutex *subscription_handlers_mutex
);

}

