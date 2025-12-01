#include "interpretation.hpp"
#include "core/imports/imports.hpp"
#include "core/interpreter.hpp"
#include "sxs/slp/slp.hpp"
#include <fmt/core.h>

namespace pkg::core::instructions::interpretation {

slp::slp_object_c interpret_define(callable_context_if &context,
                                   slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error("def requires exactly 2 arguments");
  }

  auto symbol_obj = list.at(1);
  if (symbol_obj.type() != slp::slp_type_e::SYMBOL) {
    throw std::runtime_error("def requires first argument to be a symbol");
  }

  std::string symbol_name = symbol_obj.as_symbol();

  if (context.has_symbol(symbol_name, true)) {
    throw std::runtime_error(fmt::format(
        "Symbol '{}' is already defined in current scope", symbol_name));
  }

  auto value_obj = list.at(2);
  auto evaluated_value = context.eval(value_obj);

  context.define_symbol(symbol_name, evaluated_value);

  slp::slp_object_c result;
  return result;
}

slp::slp_object_c interpret_fn(callable_context_if &context,
                               slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 4) {
    throw std::runtime_error(
        "fn requires exactly 3 arguments: (params) :return-type [body]");
  }

  auto params_obj = list.at(1);
  auto return_type_obj = list.at(2);
  auto body_obj = list.at(3);

  if (params_obj.type() != slp::slp_type_e::PAREN_LIST) {
    throw std::runtime_error("fn: first argument must be parameter list");
  }
  if (return_type_obj.type() != slp::slp_type_e::SYMBOL) {
    throw std::runtime_error("fn: second argument must be return type symbol");
  }
  if (body_obj.type() != slp::slp_type_e::BRACKET_LIST) {
    throw std::runtime_error(
        "fn: third argument must be bracket list (function body)");
  }

  std::string return_type_sym = return_type_obj.as_symbol();
  slp::slp_type_e return_type;
  if (!context.is_symbol_enscribing_valid_type(return_type_sym, return_type)) {
    throw std::runtime_error(
        fmt::format("fn: invalid return type: {}", return_type_sym));
  }

  auto params_list = params_obj.as_list();
  std::vector<callable_parameter_s> parameters;

  for (size_t i = 0; i < params_list.size(); i += 2) {
    if (i + 1 >= params_list.size()) {
      throw std::runtime_error("fn: parameters must be in pairs (name :type)");
    }

    auto param_name_obj = params_list.at(i);
    auto param_type_obj = params_list.at(i + 1);

    if (param_name_obj.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error("fn: parameter name must be a symbol");
    }
    if (param_type_obj.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error("fn: parameter type must be a type symbol");
    }

    std::string param_name = param_name_obj.as_symbol();
    std::string param_type_sym = param_type_obj.as_symbol();
    slp::slp_type_e param_type;

    if (!context.is_symbol_enscribing_valid_type(param_type_sym, param_type)) {
      throw std::runtime_error(
          fmt::format("fn: invalid parameter type: {}", param_type_sym));
    }

    parameters.push_back({param_name, param_type});
  }

  std::uint64_t lambda_id = context.allocate_lambda_id();
  context.register_lambda(lambda_id, parameters, return_type, body_obj);

  slp::slp_buffer_c buffer;
  buffer.resize(sizeof(slp::slp_unit_of_store_t));
  auto *unit = reinterpret_cast<slp::slp_unit_of_store_t *>(buffer.data());
  unit->header = static_cast<std::uint32_t>(slp::slp_type_e::ABERRANT);
  unit->flags = 0;
  unit->data.uint64 = lambda_id;

  return slp::slp_object_c::from_data(buffer, {}, 0);
}

slp::slp_object_c interpret_debug(callable_context_if &context,
                                  slp::slp_object_c &args_list) {
  fmt::print("[DEBUG]");

  auto list = args_list.as_list();
  for (size_t i = 1; i < list.size(); i++) {
    auto elem = list.at(i);
    auto evaled = context.eval(elem);

    fmt::print(" ");

    auto type = evaled.type();
    switch (type) {
    case slp::slp_type_e::INTEGER:
      fmt::print("{}", evaled.as_int());
      break;
    case slp::slp_type_e::REAL:
      fmt::print("{}", evaled.as_real());
      break;
    case slp::slp_type_e::SYMBOL:
      fmt::print("{}", evaled.as_symbol());
      break;
    case slp::slp_type_e::DQ_LIST:
      fmt::print("\"{}\"", evaled.as_string().to_string());
      break;
    default:
      fmt::print("[{}]", static_cast<int>(type));
      break;
    }
  }
  fmt::print("\n");

  slp::slp_object_c result;
  return result;
}

slp::slp_object_c interpret_export(callable_context_if &context,
                                   slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "export requires exactly 2 arguments: name and value");
  }

  auto name_obj = list.at(1);
  if (name_obj.type() != slp::slp_type_e::SYMBOL) {
    throw std::runtime_error(
        "export: first argument must be a symbol (export name)");
  }

  std::string export_name = name_obj.as_symbol();
  auto value_obj = list.at(2);
  auto evaluated_value = context.eval(value_obj);

  context.define_symbol(export_name, evaluated_value);

  auto import_context = context.get_import_context();
  if (!import_context) {
    throw std::runtime_error("export: no import context available");
  }

  if (!import_context->register_export(export_name, evaluated_value)) {
    throw std::runtime_error(
        fmt::format("export: failed to register export {}", export_name));
  }

  slp::slp_object_c result;
  return result;
}

slp::slp_object_c interpret_if(callable_context_if &context,
                               slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 4) {
    throw std::runtime_error("if requires exactly 3 arguments: condition, "
                             "true-branch, false-branch");
  }

  auto condition_obj = list.at(1);
  auto true_branch_obj = list.at(2);
  auto false_branch_obj = list.at(3);

  auto evaluated_condition = context.eval(condition_obj);

  bool execute_true_branch = true;

  if (evaluated_condition.type() == slp::slp_type_e::INTEGER) {
    std::int64_t condition_value = evaluated_condition.as_int();
    execute_true_branch = (condition_value != 0);
  }

  if (execute_true_branch) {
    return context.eval(true_branch_obj);
  } else {
    return context.eval(false_branch_obj);
  }
}

slp::slp_object_c interpret_reflect(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() < 3) {
    throw std::runtime_error(
        "reflect requires at least 2 arguments: value and one handler");
  }

  auto value_obj = list.at(1);
  auto evaluated_value = context.eval(value_obj);
  auto actual_type = evaluated_value.type();

  for (size_t i = 2; i < list.size(); i++) {
    auto handler = list.at(i);

    if (handler.type() != slp::slp_type_e::PAREN_LIST) {
      throw std::runtime_error(
          "reflect: handlers must be paren lists like (:type body)");
    }

    auto handler_list = handler.as_list();
    if (handler_list.size() != 2) {
      throw std::runtime_error(
          "reflect: handler must have exactly 2 elements: (:type body)");
    }

    auto type_symbol_obj = handler_list.at(0);
    if (type_symbol_obj.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error(
          "reflect: handler type must be a symbol like :int");
    }

    std::string type_symbol = type_symbol_obj.as_symbol();

    if (actual_type == slp::slp_type_e::ABERRANT &&
        type_symbol.find(":fn<") == 0) {
      const std::uint8_t *base_ptr = evaluated_value.get_data().data();
      const std::uint8_t *unit_ptr =
          base_ptr + evaluated_value.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      std::uint64_t lambda_id = unit->data.uint64;

      std::string lambda_sig = context.get_lambda_signature(lambda_id);
      if (lambda_sig == type_symbol) {
        auto body = handler_list.at(1);
        return context.eval(body);
      }
      continue;
    }

    slp::slp_type_e handler_type;

    if (!context.is_symbol_enscribing_valid_type(type_symbol, handler_type)) {
      throw std::runtime_error(
          fmt::format("reflect: invalid type symbol: {}", type_symbol));
    }

    if (handler_type == actual_type) {
      auto body = handler_list.at(1);
      return context.eval(body);
    }
  }

  std::string error_msg = "@(handler not supplied for given type)";
  auto error_parse = slp::parse(error_msg);
  return error_parse.take();
}

slp::slp_object_c interpret_try(callable_context_if &context,
                                slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "try requires exactly 2 arguments: body and handler");
  }

  auto body_obj = list.at(1);
  auto handler_obj = list.at(2);

  auto result = context.eval(body_obj);

  if (result.type() == slp::slp_type_e::ERROR) {
    const std::uint8_t *base_ptr = result.get_data().data();
    const std::uint8_t *unit_ptr = base_ptr + result.get_root_offset();
    const slp::slp_unit_of_store_t *unit =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
    size_t inner_offset = static_cast<size_t>(unit->data.uint64);

    auto inner_obj = slp::slp_object_c::from_data(
        result.get_data(), result.get_symbols(), inner_offset);

    if (handler_obj.type() == slp::slp_type_e::BRACKET_LIST) {
      context.push_scope();
      context.define_symbol("$error", inner_obj);
      auto handler_result = context.eval(handler_obj);
      context.pop_scope();
      return handler_result;
    } else {
      return context.eval(handler_obj);
    }
  }

  return result;
}

slp::slp_object_c interpret_assert(callable_context_if &context,
                                   slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "assert requires exactly 2 arguments: condition and message");
  }

  auto condition_obj = list.at(1);
  auto message_obj = list.at(2);

  auto evaluated_condition = context.eval(condition_obj);
  auto evaluated_message = context.eval(message_obj);

  if (evaluated_condition.type() != slp::slp_type_e::INTEGER) {
    throw std::runtime_error("assert: condition must evaluate to an integer");
  }

  if (evaluated_message.type() != slp::slp_type_e::DQ_LIST) {
    throw std::runtime_error("assert: message must be a string");
  }

  std::int64_t condition_value = evaluated_condition.as_int();
  if (condition_value == 0) {
    std::string message = evaluated_message.as_string().to_string();
    throw std::runtime_error(message);
  }

  slp::slp_object_c result;
  return result;
}

slp::slp_object_c interpret_recover(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "recover requires exactly 2 arguments: body and handler");
  }

  auto body_obj = list.at(1);
  auto handler_obj = list.at(2);

  if (body_obj.type() != slp::slp_type_e::BRACKET_LIST) {
    throw std::runtime_error("recover: body must be a bracket list");
  }
  if (handler_obj.type() != slp::slp_type_e::BRACKET_LIST) {
    throw std::runtime_error("recover: handler must be a bracket list");
  }

  try {
    return context.eval(body_obj);
  } catch (const std::exception &e) {
    std::string exception_message = e.what();
    std::string exception_str_literal = "\"" + exception_message + "\"";
    auto exception_str_parse = slp::parse(exception_str_literal);
    if (exception_str_parse.is_error()) {
      throw std::runtime_error("recover: failed to parse exception string");
    }
    auto exception_str_obj = exception_str_parse.take();

    context.push_scope();
    context.define_symbol("$exception", exception_str_obj);
    auto handler_result = context.eval(handler_obj);
    context.pop_scope();
    return handler_result;
  }
}

slp::slp_object_c interpret_eval(callable_context_if &context,
                                 slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 2) {
    throw std::runtime_error("eval requires exactly 1 argument: code string");
  }

  auto code_obj = list.at(1);
  auto evaluated_code = context.eval(code_obj);

  if (evaluated_code.type() != slp::slp_type_e::DQ_LIST) {
    throw std::runtime_error("eval: argument must be a string");
  }

  std::string code_string = evaluated_code.as_string().to_string();

  auto parse_result = slp::parse(code_string);
  if (parse_result.is_error()) {
    const auto &error = parse_result.error();
    throw std::runtime_error(
        fmt::format("eval: parse error: {}", error.message));
  }

  auto parsed_obj = parse_result.take();

  context.push_scope();
  auto result = context.eval(parsed_obj);
  context.pop_scope();

  return result;
}

slp::slp_object_c interpret_apply(callable_context_if &context,
                                  slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "apply requires exactly 2 arguments: lambda and args-list");
  }

  auto lambda_obj = list.at(1);
  auto args_obj = list.at(2);

  auto evaluated_lambda = context.eval(lambda_obj);
  if (evaluated_lambda.type() != slp::slp_type_e::ABERRANT) {
    throw std::runtime_error(
        "apply: first argument must be a lambda (aberrant type)");
  }

  auto evaluated_args = context.eval(args_obj);
  if (evaluated_args.type() != slp::slp_type_e::BRACE_LIST) {
    throw std::runtime_error(
        "apply: second argument must be a brace list of arguments");
  }

  auto args_to_apply = evaluated_args.as_list();

  context.push_scope();
  context.define_symbol("apply-temp-lambda", evaluated_lambda);

  std::string call_str = "(apply-temp-lambda";
  for (size_t i = 0; i < args_to_apply.size(); i++) {
    call_str += " ";
    auto arg = args_to_apply.at(i);
    std::string arg_sym = "apply-temp-arg-" + std::to_string(i);
    context.define_symbol(arg_sym, arg);
    call_str += arg_sym;
  }
  call_str += ")";

  auto parse_result = slp::parse(call_str);
  if (parse_result.is_error()) {
    context.pop_scope();
    throw std::runtime_error("apply: failed to construct call");
  }

  auto call_obj = parse_result.take();
  auto result = context.eval(call_obj);
  context.pop_scope();

  return result;
}

slp::slp_object_c interpret_match(callable_context_if &context,
                                  slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() < 3) {
    throw std::runtime_error(
        "match requires at least 2 arguments: value and one handler");
  }

  auto value_obj = list.at(1);
  auto evaluated_value = context.eval(value_obj);
  auto actual_type = evaluated_value.type();

  for (size_t i = 2; i < list.size(); i++) {
    auto handler = list.at(i);

    if (handler.type() != slp::slp_type_e::PAREN_LIST) {
      throw std::runtime_error(
          "match: handlers must be paren lists like (pattern result)");
    }

    auto handler_list = handler.as_list();
    if (handler_list.size() != 2) {
      throw std::runtime_error(
          "match: handler must have exactly 2 elements: (pattern result)");
    }

    auto pattern_obj = handler_list.at(0);
    auto evaluated_pattern = context.eval(pattern_obj);

    if (evaluated_pattern.type() != actual_type) {
      continue;
    }

    bool values_match = false;
    switch (actual_type) {
    case slp::slp_type_e::INTEGER:
      values_match = evaluated_value.as_int() == evaluated_pattern.as_int();
      break;
    case slp::slp_type_e::REAL:
      values_match = evaluated_value.as_real() == evaluated_pattern.as_real();
      break;
    case slp::slp_type_e::SYMBOL: {
      std::string val_sym = evaluated_value.as_symbol();
      std::string pat_sym = evaluated_pattern.as_symbol();
      values_match = (val_sym == pat_sym);
      break;
    }
    case slp::slp_type_e::DQ_LIST: {
      std::string val_str = evaluated_value.as_string().to_string();
      std::string pat_str = evaluated_pattern.as_string().to_string();
      values_match = (val_str == pat_str);
      break;
    }
    case slp::slp_type_e::ABERRANT: {
      const std::uint8_t *val_base = evaluated_value.get_data().data();
      const std::uint8_t *val_unit =
          val_base + evaluated_value.get_root_offset();
      const slp::slp_unit_of_store_t *val_u =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(val_unit);
      std::uint64_t val_id = val_u->data.uint64;

      const std::uint8_t *pat_base = evaluated_pattern.get_data().data();
      const std::uint8_t *pat_unit =
          pat_base + evaluated_pattern.get_root_offset();
      const slp::slp_unit_of_store_t *pat_u =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(pat_unit);
      std::uint64_t pat_id = pat_u->data.uint64;

      values_match = (val_id == pat_id);
      break;
    }
    default:
      values_match = false;
      break;
    }

    if (values_match) {
      auto result_obj = handler_list.at(1);
      return context.eval(result_obj);
    }
  }

  std::string error_msg = "@(no matching handler found)";
  auto error_parse = slp::parse(error_msg);
  return error_parse.take();
}

slp::slp_object_c interpret_cast(callable_context_if &context,
                                 slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "cast requires exactly 2 arguments: type and value");
  }

  auto type_obj = list.at(1);
  auto value_obj = list.at(2);

  if (type_obj.type() != slp::slp_type_e::SYMBOL) {
    throw std::runtime_error("cast: first argument must be a type symbol");
  }

  std::string type_symbol = type_obj.as_symbol();
  slp::slp_type_e expected_type;

  if (!context.is_symbol_enscribing_valid_type(type_symbol, expected_type)) {
    throw std::runtime_error(
        fmt::format("cast: invalid type symbol: {}", type_symbol));
  }

  auto evaluated_value = context.eval(value_obj);
  auto actual_type = evaluated_value.type();

  if (expected_type == actual_type) {
    return evaluated_value;
  }

  if (expected_type == slp::slp_type_e::INTEGER &&
      actual_type == slp::slp_type_e::REAL) {
    double real_val = evaluated_value.as_real();
    std::int64_t int_val = static_cast<std::int64_t>(real_val);
    std::string int_str = std::to_string(int_val);
    auto parse_result = slp::parse(int_str);
    return parse_result.take();
  }

  if (expected_type == slp::slp_type_e::REAL &&
      actual_type == slp::slp_type_e::INTEGER) {
    std::int64_t int_val = evaluated_value.as_int();
    double real_val = static_cast<double>(int_val);
    std::string real_str = std::to_string(real_val);
    auto parse_result = slp::parse(real_str);
    return parse_result.take();
  }

  bool is_actual_list_type = (actual_type == slp::slp_type_e::DQ_LIST ||
                              actual_type == slp::slp_type_e::PAREN_LIST ||
                              actual_type == slp::slp_type_e::BRACE_LIST ||
                              actual_type == slp::slp_type_e::BRACKET_LIST ||
                              actual_type == slp::slp_type_e::SOME);

  bool is_expected_list_type = (expected_type == slp::slp_type_e::DQ_LIST ||
                                expected_type == slp::slp_type_e::PAREN_LIST ||
                                expected_type == slp::slp_type_e::BRACE_LIST ||
                                expected_type == slp::slp_type_e::BRACKET_LIST);

  if (is_actual_list_type && is_expected_list_type) {
    if (actual_type == slp::slp_type_e::SOME) {
      const std::uint8_t *base_ptr = evaluated_value.get_data().data();
      const std::uint8_t *unit_ptr =
          base_ptr + evaluated_value.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      size_t inner_offset = static_cast<size_t>(unit->data.uint64);

      evaluated_value = slp::slp_object_c::from_data(
          evaluated_value.get_data(), evaluated_value.get_symbols(),
          inner_offset);
      actual_type = evaluated_value.type();
    }
    if (expected_type == slp::slp_type_e::DQ_LIST) {
      std::string bytes;
      if (actual_type == slp::slp_type_e::DQ_LIST) {
        bytes = evaluated_value.as_string().to_string();
      } else {
        auto list_items = evaluated_value.as_list();
        for (size_t i = 0; i < list_items.size(); i++) {
          auto item = list_items.at(i);
          if (item.type() == slp::slp_type_e::INTEGER) {
            std::int64_t val = item.as_int();
            unsigned char byte = static_cast<unsigned char>(val % 256);
            bytes += static_cast<char>(byte);
          } else if (item.type() == slp::slp_type_e::DQ_LIST) {
            bytes += item.as_string().to_string();
          }
        }
      }
      return slp::create_string_direct(bytes);
    }

    if (actual_type == slp::slp_type_e::DQ_LIST) {
      auto str_data = evaluated_value.as_string();
      size_t len = str_data.size();
      std::vector<slp::slp_object_c> int_objects;
      int_objects.reserve(len);
      for (size_t i = 0; i < len; i++) {
        unsigned char byte = static_cast<unsigned char>(str_data.at(i));
        int_objects.push_back(
            slp::slp_object_c::create_int(static_cast<std::int64_t>(byte)));
      }

      if (expected_type == slp::slp_type_e::PAREN_LIST) {
        return slp::slp_object_c::create_paren_list(int_objects.data(),
                                                    int_objects.size());
      } else if (expected_type == slp::slp_type_e::BRACE_LIST) {
        return slp::slp_object_c::create_brace_list(int_objects.data(),
                                                    int_objects.size());
      } else if (expected_type == slp::slp_type_e::BRACKET_LIST) {
        return slp::slp_object_c::create_bracket_list(int_objects.data(),
                                                      int_objects.size());
      }
    }

    std::string repr;
    auto list_items = evaluated_value.as_list();
    std::string list_str;
    for (size_t i = 0; i < list_items.size(); i++) {
      if (i > 0)
        list_str += " ";
      auto item = list_items.at(i);
      if (item.type() == slp::slp_type_e::SYMBOL) {
        list_str += item.as_symbol();
      } else if (item.type() == slp::slp_type_e::INTEGER) {
        list_str += std::to_string(item.as_int());
      } else if (item.type() == slp::slp_type_e::REAL) {
        list_str += std::to_string(item.as_real());
      } else if (item.type() == slp::slp_type_e::DQ_LIST) {
        list_str += "\"" + item.as_string().to_string() + "\"";
      } else {
        throw std::runtime_error(
            "cast: cannot convert complex list structures");
      }
    }
    repr = list_str;

    std::string cast_str;
    if (expected_type == slp::slp_type_e::PAREN_LIST) {
      cast_str = "(" + repr + ")";
    } else if (expected_type == slp::slp_type_e::BRACE_LIST) {
      cast_str = "{" + repr + "}";
    } else if (expected_type == slp::slp_type_e::BRACKET_LIST) {
      cast_str = "[" + repr + "]";
    }

    auto parse_result = slp::parse(cast_str);
    if (parse_result.is_error()) {
      throw std::runtime_error("cast: failed to parse converted value");
    }
    return parse_result.take();
  }

  if (expected_type == slp::slp_type_e::DQ_LIST) {
    std::string result_str;
    switch (actual_type) {
    case slp::slp_type_e::INTEGER:
      result_str = std::to_string(evaluated_value.as_int());
      break;
    case slp::slp_type_e::REAL:
      result_str = std::to_string(evaluated_value.as_real());
      break;
    case slp::slp_type_e::SYMBOL:
      result_str = evaluated_value.as_symbol();
      break;
    case slp::slp_type_e::NONE:
      result_str = "()";
      break;
    case slp::slp_type_e::ABERRANT:
      result_str = "?lambda";
      break;
    case slp::slp_type_e::ERROR: {
      const std::uint8_t *base_ptr = evaluated_value.get_data().data();
      const std::uint8_t *unit_ptr =
          base_ptr + evaluated_value.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      size_t inner_offset = static_cast<size_t>(unit->data.uint64);
      auto inner_obj = slp::slp_object_c::from_data(
          evaluated_value.get_data(), evaluated_value.get_symbols(),
          inner_offset);
      context.push_scope();
      context.define_symbol("cast_temp_inner", inner_obj);
      auto inner_cast = slp::parse("(cast :str cast_temp_inner)");
      if (!inner_cast.is_error()) {
        auto inner_cast_obj = inner_cast.take();
        auto inner_result = context.eval(inner_cast_obj);
        context.pop_scope();
        if (inner_result.type() == slp::slp_type_e::DQ_LIST) {
          result_str = "@(" + inner_result.as_string().to_string() + ")";
          return slp::create_string_direct(result_str);
        }
      }
      context.pop_scope();
      result_str = "@(error)";
      break;
    }
    case slp::slp_type_e::SOME: {
      const std::uint8_t *base_ptr = evaluated_value.get_data().data();
      const std::uint8_t *unit_ptr =
          base_ptr + evaluated_value.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      size_t inner_offset = static_cast<size_t>(unit->data.uint64);
      auto inner_obj = slp::slp_object_c::from_data(
          evaluated_value.get_data(), evaluated_value.get_symbols(),
          inner_offset);
      context.push_scope();
      context.define_symbol("cast_temp_inner", inner_obj);
      auto inner_cast = slp::parse("(cast :str cast_temp_inner)");
      if (!inner_cast.is_error()) {
        auto inner_cast_obj = inner_cast.take();
        auto inner_result = context.eval(inner_cast_obj);
        context.pop_scope();
        if (inner_result.type() == slp::slp_type_e::DQ_LIST) {
          result_str = "'" + inner_result.as_string().to_string();
          return slp::create_string_direct(result_str);
        }
      }
      context.pop_scope();
      result_str = "'()";
      break;
    }
    case slp::slp_type_e::DATUM: {
      const std::uint8_t *base_ptr = evaluated_value.get_data().data();
      const std::uint8_t *unit_ptr =
          base_ptr + evaluated_value.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      size_t inner_offset = static_cast<size_t>(unit->data.uint64);
      auto inner_obj = slp::slp_object_c::from_data(
          evaluated_value.get_data(), evaluated_value.get_symbols(),
          inner_offset);
      context.push_scope();
      context.define_symbol("cast_temp_inner", inner_obj);
      auto inner_cast = slp::parse("(cast :str cast_temp_inner)");
      if (!inner_cast.is_error()) {
        auto inner_cast_obj = inner_cast.take();
        auto inner_result = context.eval(inner_cast_obj);
        context.pop_scope();
        if (inner_result.type() == slp::slp_type_e::DQ_LIST) {
          result_str = "#" + inner_result.as_string().to_string();
          return slp::create_string_direct(result_str);
        }
      }
      context.pop_scope();
      result_str = "#()";
      break;
    }
    default:
      result_str = fmt::format("[type:{}]", static_cast<int>(actual_type));
      break;
    }
    return slp::create_string_direct(result_str);
  }

  throw std::runtime_error(fmt::format(
      "cast: type mismatch: expected {}, got {}",
      static_cast<int>(expected_type), static_cast<int>(actual_type)));
}

slp::slp_object_c interpret_do(callable_context_if &context,
                               slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 2) {
    throw std::runtime_error("do requires exactly 1 argument: body");
  }

  auto body_obj = list.at(1);
  if (body_obj.type() != slp::slp_type_e::BRACKET_LIST) {
    throw std::runtime_error("do: argument must be a bracket list");
  }

  context.push_loop_context();

  while (true) {
    context.push_scope();

    std::int64_t current_iteration = context.get_current_iteration();
    auto iteration_obj = slp::slp_object_c::create_int(current_iteration);
    context.define_symbol("$iterations", iteration_obj);

    auto body_copy = slp::slp_object_c::from_data(body_obj.get_data(),
                                                  body_obj.get_symbols(),
                                                  body_obj.get_root_offset());
    context.eval(body_copy);

    context.pop_scope();

    if (context.should_exit_loop()) {
      break;
    }

    context.increment_iteration();
  }

  auto return_value = context.get_loop_return_value();
  context.pop_loop_context();

  return return_value;
}

slp::slp_object_c interpret_done(callable_context_if &context,
                                 slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 2) {
    throw std::runtime_error("done requires exactly 1 argument: return value");
  }

  if (!context.is_in_loop()) {
    throw std::runtime_error("done called outside of do loop");
  }

  auto value_obj = list.at(1);
  auto evaluated_value = context.eval(value_obj);

  context.signal_loop_done(evaluated_value);

  slp::slp_object_c result;
  return result;
}

slp::slp_object_c interpret_at(callable_context_if &context,
                               slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "at requires exactly 2 arguments: index and collection");
  }

  auto index_obj = list.at(1);
  auto collection_obj = list.at(2);

  auto evaluated_index = context.eval(index_obj);
  if (evaluated_index.type() != slp::slp_type_e::INTEGER) {
    throw std::runtime_error("at: index must be an integer");
  }

  std::int64_t index = evaluated_index.as_int();
  if (index < 0) {
    std::string error_msg = "@(index out of bounds)";
    auto error_parse = slp::parse(error_msg);
    return error_parse.take();
  }

  auto evaluated_collection = context.eval(collection_obj);
  auto collection_type = evaluated_collection.type();

  if (collection_type == slp::slp_type_e::DQ_LIST) {
    auto str_data = evaluated_collection.as_string();
    if (static_cast<size_t>(index) >= str_data.size()) {
      std::string error_msg = "@(index out of bounds)";
      auto error_parse = slp::parse(error_msg);
      return error_parse.take();
    }
    unsigned char byte = static_cast<unsigned char>(str_data.at(index));
    return slp::slp_object_c::create_int(static_cast<std::int64_t>(byte));
  }

  if (collection_type == slp::slp_type_e::PAREN_LIST ||
      collection_type == slp::slp_type_e::BRACKET_LIST ||
      collection_type == slp::slp_type_e::BRACE_LIST) {
    auto collection_list = evaluated_collection.as_list();
    if (static_cast<size_t>(index) >= collection_list.size()) {
      std::string error_msg = "@(index out of bounds)";
      auto error_parse = slp::parse(error_msg);
      return error_parse.take();
    }
    return collection_list.at(static_cast<size_t>(index));
  }

  throw std::runtime_error("at: collection must be a list or string type");
}

slp::slp_object_c interpret_eq(callable_context_if &context,
                               slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error("eq requires exactly 2 arguments: lhs and rhs");
  }

  auto lhs_obj = list.at(1);
  auto rhs_obj = list.at(2);

  auto evaluated_lhs = context.eval(lhs_obj);
  auto evaluated_rhs = context.eval(rhs_obj);

  auto lhs_type = evaluated_lhs.type();
  auto rhs_type = evaluated_rhs.type();

  if (lhs_type != rhs_type) {
    return slp::slp_object_c::create_int(0);
  }

  if (lhs_type == slp::slp_type_e::ABERRANT) {
    const std::uint8_t *lhs_base = evaluated_lhs.get_data().data();
    const std::uint8_t *lhs_unit = lhs_base + evaluated_lhs.get_root_offset();
    const slp::slp_unit_of_store_t *lhs_u =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(lhs_unit);
    std::uint64_t lhs_id = lhs_u->data.uint64;

    const std::uint8_t *rhs_base = evaluated_rhs.get_data().data();
    const std::uint8_t *rhs_unit = rhs_base + evaluated_rhs.get_root_offset();
    const slp::slp_unit_of_store_t *rhs_u =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(rhs_unit);
    std::uint64_t rhs_id = rhs_u->data.uint64;

    return slp::slp_object_c::create_int(lhs_id == rhs_id ? 1 : 0);
  }

  if (lhs_type == slp::slp_type_e::ERROR || lhs_type == slp::slp_type_e::SOME ||
      lhs_type == slp::slp_type_e::DATUM) {
    const std::uint8_t *lhs_base = evaluated_lhs.get_data().data();
    const std::uint8_t *lhs_unit = lhs_base + evaluated_lhs.get_root_offset();
    const slp::slp_unit_of_store_t *lhs_u =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(lhs_unit);
    size_t lhs_inner_offset = static_cast<size_t>(lhs_u->data.uint64);
    auto lhs_inner = slp::slp_object_c::from_data(evaluated_lhs.get_data(),
                                                  evaluated_lhs.get_symbols(),
                                                  lhs_inner_offset);

    const std::uint8_t *rhs_base = evaluated_rhs.get_data().data();
    const std::uint8_t *rhs_unit = rhs_base + evaluated_rhs.get_root_offset();
    const slp::slp_unit_of_store_t *rhs_u =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(rhs_unit);
    size_t rhs_inner_offset = static_cast<size_t>(rhs_u->data.uint64);
    auto rhs_inner = slp::slp_object_c::from_data(evaluated_rhs.get_data(),
                                                  evaluated_rhs.get_symbols(),
                                                  rhs_inner_offset);

    context.push_scope();
    context.define_symbol("eq_lhs_inner", lhs_inner);
    context.define_symbol("eq_rhs_inner", rhs_inner);
    auto parse_result = slp::parse("(eq eq_lhs_inner eq_rhs_inner)");
    if (!parse_result.is_error()) {
      auto call_obj = parse_result.take();
      auto result = context.eval(call_obj);
      context.pop_scope();
      return result;
    }
    context.pop_scope();
    return slp::slp_object_c::create_int(0);
  }

  if (lhs_type == slp::slp_type_e::PAREN_LIST ||
      lhs_type == slp::slp_type_e::BRACKET_LIST ||
      lhs_type == slp::slp_type_e::BRACE_LIST) {
    auto lhs_list = evaluated_lhs.as_list();
    auto rhs_list = evaluated_rhs.as_list();

    if (lhs_list.size() != rhs_list.size()) {
      return slp::slp_object_c::create_int(0);
    }

    for (size_t i = 0; i < lhs_list.size(); i++) {
      auto lhs_elem = lhs_list.at(i);
      auto rhs_elem = rhs_list.at(i);

      context.push_scope();
      context.define_symbol("eq_lhs_elem", lhs_elem);
      context.define_symbol("eq_rhs_elem", rhs_elem);
      auto parse_result = slp::parse("(eq eq_lhs_elem eq_rhs_elem)");
      if (parse_result.is_error()) {
        context.pop_scope();
        return slp::slp_object_c::create_int(0);
      }
      auto call_obj = parse_result.take();
      auto result = context.eval(call_obj);
      context.pop_scope();

      if (result.type() != slp::slp_type_e::INTEGER || result.as_int() == 0) {
        return slp::slp_object_c::create_int(0);
      }
    }
    return slp::slp_object_c::create_int(1);
  }

  context.push_scope();
  context.define_symbol("eq_temp_lhs", evaluated_lhs);
  context.define_symbol("eq_temp_rhs", evaluated_rhs);

  auto lhs_cast_parse = slp::parse("(cast :str eq_temp_lhs)");
  auto rhs_cast_parse = slp::parse("(cast :str eq_temp_rhs)");

  if (lhs_cast_parse.is_error() || rhs_cast_parse.is_error()) {
    context.pop_scope();
    return slp::slp_object_c::create_int(0);
  }

  auto lhs_cast_obj = lhs_cast_parse.take();
  auto rhs_cast_obj = rhs_cast_parse.take();

  auto lhs_cast_result = context.eval(lhs_cast_obj);
  auto rhs_cast_result = context.eval(rhs_cast_obj);

  context.pop_scope();

  if (lhs_cast_result.type() != slp::slp_type_e::DQ_LIST ||
      rhs_cast_result.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(0);
  }

  std::string lhs_string = lhs_cast_result.as_string().to_string();
  std::string rhs_string = rhs_cast_result.as_string().to_string();

  bool equal = (lhs_string == rhs_string);
  return slp::slp_object_c::create_int(equal ? 1 : 0);
}

} // namespace pkg::core::instructions::interpretation
