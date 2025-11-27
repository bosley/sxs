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

struct function_parameter_s {
  std::string name;
  slp::slp_type_e type;
  bool is_evaluated{true};
};

struct function_information_s {
  slp::slp_type_e return_type{slp::slp_type_e::NONE};

  std::vector<function_parameter_s> parameters;

  bool is_variadic{false};

  bool can_return_error{true};

  std::map<std::string, slp::slp_type_e> handler_context_vars;

  std::function<slp::slp_object_c(
      session_c &, const slp::slp_object_c &,
      const std::map<std::string, slp::slp_object_c> &)>
      function;
};

struct function_group_s {
  const char *group_name;
  std::map<std::string, function_information_s> functions;
};

std::vector<function_group_s>
get_all_function_groups(runtime_information_if &runtime_info);

} // namespace runtime::fns