#pragma once

#include "slp/slp.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pkg::ts {

struct type_info_s {
  slp::slp_type_e type{slp::slp_type_e::NONE};
  bool is_tainted{false};

  type_info_s() = default;
  type_info_s(slp::slp_type_e t, bool tainted = false)
      : type(t), is_tainted(tainted) {}
};

struct function_parameter_info_s {
  slp::slp_type_e type;
  bool is_evaluated;
};

struct function_signature_s {
  std::vector<function_parameter_info_s> parameters;
  slp::slp_type_e return_type{slp::slp_type_e::NONE};
  bool can_return_error{true};
  bool is_variadic{false};
  bool is_detainter{false};
  bool is_setter{false};
  bool is_getter{false};
};

class type_checker_c {
public:
  type_checker_c(const std::map<std::string, function_signature_s> &signatures);

  struct check_result_s {
    bool success{false};
    std::string error_message;
  };

  check_result_s check(const slp::slp_object_c &program);

private:
  type_info_s infer_type(const slp::slp_object_c &obj,
                         std::map<std::string, type_info_s> &symbol_map);

  std::map<std::string, function_signature_s> function_signatures_;
};

} // namespace pkg::ts
