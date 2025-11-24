#include "runtime/fns/fns.hpp"
#include "runtime/fns/kv.hpp"
#include "runtime/fns/event.hpp"
#include "runtime/fns/runtime_fns.hpp"

namespace runtime::fns {

std::vector<function_group_s> get_all_function_groups(const function_dependencies_s &deps) {
    std::vector<function_group_s> groups;
    
    groups.push_back(get_kv_functions(
        deps.logger,
        deps.eval_fn,
        deps.to_string_fn
    ));
    
    groups.push_back(get_event_functions(
        deps.logger,
        deps.eval_fn,
        deps.to_string_fn,
        deps.subscription_handlers,
        deps.subscription_handlers_mutex
    ));
    
    groups.push_back(get_runtime_functions(
        deps.logger,
        deps.eval_fn,
        deps.to_string_fn,
        deps.pending_awaits,
        deps.pending_awaits_mutex,
        deps.max_await_timeout
    ));
    
    return groups;
}

}

