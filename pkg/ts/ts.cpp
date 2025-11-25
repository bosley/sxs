#include "ts/ts.hpp"

namespace pkg::ts {

type_checker_c::type_checker_c(
    const std::map<std::string, function_signature_s> &signatures)
    : function_signatures_(signatures) {}

type_checker_c::check_result_s
type_checker_c::check(const slp::slp_object_c &program) {
  check_result_s result;

  if (program.type() != slp::slp_type_e::PAREN_LIST) {
    result.error_message = "program must be a paren list";
    return result;
  }

  std::map<std::string, type_info_s> symbol_map;

  auto list = program.as_list();
  for (size_t i = 0; i < list.size(); i++) {
    auto expr = list.at(i);
    auto inferred = infer_type(expr, symbol_map);

    if (inferred.type == slp::slp_type_e::ERROR) {
      result.error_message =
          "type check failed at expression " + std::to_string(i);
      return result;
    }
  }

  result.success = true;
  return result;
}

type_info_s
type_checker_c::infer_type(const slp::slp_object_c &obj,
                           std::map<std::string, type_info_s> &symbol_map) {

  auto obj_type = obj.type();

  switch (obj_type) {
  case slp::slp_type_e::INTEGER:
    return type_info_s(slp::slp_type_e::INTEGER, false);

  case slp::slp_type_e::REAL:
    return type_info_s(slp::slp_type_e::REAL, false);

  case slp::slp_type_e::DQ_LIST:
    return type_info_s(slp::slp_type_e::DQ_LIST, false);

  case slp::slp_type_e::SYMBOL:
    return type_info_s(slp::slp_type_e::SYMBOL, false);

  case slp::slp_type_e::BRACE_LIST:
    return type_info_s(slp::slp_type_e::BRACE_LIST, false);

  case slp::slp_type_e::BRACKET_LIST:
    return type_info_s(slp::slp_type_e::BRACKET_LIST, false);

  case slp::slp_type_e::SOME:
    return type_info_s(slp::slp_type_e::SOME, false);

  case slp::slp_type_e::ERROR:
    return type_info_s(slp::slp_type_e::ERROR, false);

  case slp::slp_type_e::PAREN_LIST: {
    auto list = obj.as_list();
    if (list.size() == 0) {
      return type_info_s(slp::slp_type_e::NONE, false);
    }

    auto first = list.at(0);
    if (first.type() != slp::slp_type_e::SYMBOL) {
      return type_info_s(slp::slp_type_e::ERROR, false);
    }

    std::string function_name = first.as_symbol();

    auto sig_it = function_signatures_.find(function_name);
    if (sig_it == function_signatures_.end()) {
      return type_info_s(slp::slp_type_e::ERROR, false);
    }

    const auto &sig = sig_it->second;

    size_t expected_params = sig.parameters.size();
    size_t actual_params = list.size() - 1;

    if (!sig.is_variadic && actual_params != expected_params) {
      return type_info_s(slp::slp_type_e::ERROR, false);
    }

    if (sig.is_variadic && actual_params < expected_params) {
      return type_info_s(slp::slp_type_e::ERROR, false);
    }

    if (sig.is_setter && list.size() >= 3) {
      auto key_obj = list.at(1);
      auto value_obj = list.at(2);

      if (key_obj.type() != slp::slp_type_e::SYMBOL) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      std::string key_name = key_obj.as_symbol();

      auto value_type = infer_type(value_obj, symbol_map);
      if (value_type.type == slp::slp_type_e::ERROR) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      if (value_type.is_tainted) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      auto it = symbol_map.find(key_name);
      if (it == symbol_map.end()) {
        symbol_map[key_name] = value_type;
      }

      return type_info_s(sig.return_type, sig.can_return_error);
    }

    if (sig.is_getter && list.size() >= 2) {
      auto key_obj = list.at(1);

      if (key_obj.type() != slp::slp_type_e::SYMBOL) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      std::string key_name = key_obj.as_symbol();

      auto it = symbol_map.find(key_name);
      if (it == symbol_map.end()) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      return type_info_s(it->second.type, true);
    }

    if (sig.is_detainter && list.size() >= 2) {
      auto arg_type = infer_type(list.at(1), symbol_map);
      if (arg_type.type == slp::slp_type_e::ERROR) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      if (!arg_type.is_tainted) {
        return type_info_s(slp::slp_type_e::ERROR, false);
      }

      return type_info_s(arg_type.type, false);
    }

    for (size_t i = 0; i < sig.parameters.size() && i < actual_params; i++) {
      const auto &param = sig.parameters[i];
      auto arg_obj = list.at(i + 1);

      if (param.is_evaluated) {
        auto arg_type = infer_type(arg_obj, symbol_map);
        if (arg_type.type == slp::slp_type_e::ERROR) {
          return type_info_s(slp::slp_type_e::ERROR, false);
        }

        if (param.type != slp::slp_type_e::NONE) {
          if (arg_type.type != param.type) {
            return type_info_s(slp::slp_type_e::ERROR, false);
          }
        }
      } else {
        if (param.type != slp::slp_type_e::NONE) {
          if (arg_obj.type() != param.type) {
            return type_info_s(slp::slp_type_e::ERROR, false);
          }
        }
      }
    }

    return type_info_s(sig.return_type, sig.can_return_error);
  }

  case slp::slp_type_e::NONE:
  case slp::slp_type_e::RUNE:
    return type_info_s(obj_type, false);
  }

  return type_info_s(slp::slp_type_e::ERROR, false);
}

} // namespace pkg::ts

