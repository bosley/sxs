#pragma once
#include "runtime/fns/fns.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>

namespace runtime::fns {

function_group_s get_runtime_functions(
    logger_t logger,
    std::function<slp::slp_object_c(session_c*, const slp::slp_object_c&, const processor_c::eval_context_s&)> eval_fn,
    std::function<std::string(const slp::slp_object_c&)> to_string_fn,
    std::map<std::string, std::shared_ptr<processor_c::pending_await_s>> *pending_awaits,
    std::mutex *pending_awaits_mutex,
    std::chrono::seconds max_await_timeout
);

}

