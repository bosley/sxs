#pragma once
#include "runtime/processor.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace runtime::fns {

struct function_group_s {
    const char* group_name;
    std::map<std::string, std::function<slp::slp_object_c(
        session_c *, const slp::slp_object_c &, const processor_c::eval_context_s &)>> functions;
};

struct function_dependencies_s {
    logger_t logger;
    std::function<slp::slp_object_c(session_c*, const slp::slp_object_c&, const processor_c::eval_context_s&)> eval_fn;
    std::function<std::string(const slp::slp_object_c&)> to_string_fn;
    std::vector<processor_c::subscription_handler_s> *subscription_handlers;
    std::mutex *subscription_handlers_mutex;
    std::map<std::string, std::shared_ptr<processor_c::pending_await_s>> *pending_awaits;
    std::mutex *pending_awaits_mutex;
    std::chrono::seconds max_await_timeout;
};

std::vector<function_group_s> get_all_function_groups(const function_dependencies_s &deps);

}