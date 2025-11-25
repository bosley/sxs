#pragma once
#include <functional>
#include <map>
#include <slp/slp.hpp>
#include <string>
#include <vector>

namespace runtime {
class session_c;
class runtime_information_if;
} // namespace runtime

namespace runtime::fns {

struct function_group_s {
  const char *group_name;
  std::map<std::string, std::function<slp::slp_object_c(
                            session_c &, const slp::slp_object_c &,
                            const std::map<std::string, slp::slp_object_c> &)>>
      functions;
};

std::vector<function_group_s>
get_all_function_groups(runtime_information_if &runtime_info);

} // namespace runtime::fns