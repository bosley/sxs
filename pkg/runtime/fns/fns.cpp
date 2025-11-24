#include "runtime/fns/fns.hpp"
#include "runtime/fns/event.hpp"
#include "runtime/fns/kv.hpp"
#include "runtime/fns/runtime_fns.hpp"

namespace runtime::fns {

std::vector<function_group_s>
get_all_function_groups(runtime_information_if &runtime_info) {
  std::vector<function_group_s> groups;

  groups.push_back(get_kv_functions(runtime_info));
  groups.push_back(get_event_functions(runtime_info));
  groups.push_back(get_runtime_functions(runtime_info));

  return groups;
}

} // namespace runtime::fns
