#include "tcs.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <sstream>

/*
UPGRADE:DYNAMIC_INJECTED_SYMBOLS

in libs/std/forge im ideating how to dynamically permit the declaration of
injected symbols into the runtime. this would mean
  - update core/kernels to accept new command during kernel ingestion to map
symbol and type information along with what "function" if falls under
  - update this to have configurable function handling so in-addition to the
builtins we can handle injection in custom scenarios as well (see below where we
inject "$exception" and "$error")

  bosley - 11/30/25

  SEE ALSO pkg/core/kernels/kernels.cpp
*/

namespace pkg::core::tcs {

tcs_c::tcs_c(logger_t logger, std::vector<std::string> include_paths,
             std::string working_directory)
    : logger_(logger), include_paths_(std::move(include_paths)),
      working_directory_(std::move(working_directory)), next_lambda_id_(1) {

  std::vector<std::pair<std::string, slp::slp_type_e>> base_types = {
      {"int", slp::slp_type_e::INTEGER},
      {"real", slp::slp_type_e::REAL},
      {"symbol", slp::slp_type_e::SYMBOL},
      {"str", slp::slp_type_e::DQ_LIST},
      {"list-p", slp::slp_type_e::PAREN_LIST},
      {"list-c", slp::slp_type_e::BRACE_LIST},
      {"list-b", slp::slp_type_e::BRACKET_LIST},
      {"none", slp::slp_type_e::NONE},
      {"some", slp::slp_type_e::SOME},
      {"error", slp::slp_type_e::ERROR},
      {"datum", slp::slp_type_e::DATUM},
      {"aberrant", slp::slp_type_e::ABERRANT},
      {"any", slp::slp_type_e::NONE}};

  for (const auto &[name, type] : base_types) {
    type_info_s type_info;
    type_info.base_type = type;
    type_symbol_map_[":" + name] = type_info;

    type_info_s variadic_type_info;
    variadic_type_info.base_type = type;
    variadic_type_info.is_variadic = true;
    type_symbol_map_[":" + name + ".."] = variadic_type_info;
  }

  type_info_s list_type;
  list_type.base_type = slp::slp_type_e::PAREN_LIST;
  type_symbol_map_[":list"] = list_type;

  type_info_s list_variadic_type;
  list_variadic_type.base_type = slp::slp_type_e::PAREN_LIST;
  list_variadic_type.is_variadic = true;
  type_symbol_map_[":list.."] = list_variadic_type;

  push_scope();
}

tcs_c::~tcs_c() = default;

bool tcs_c::check(const std::string &file_path) {
  if (!std::filesystem::exists(file_path)) {
    logger_->error("File does not exist: {}", file_path);
    return false;
  }

  auto canonical_path = std::filesystem::canonical(file_path).string();

  if (checked_files_.count(canonical_path)) {
    logger_->debug("File already checked: {}", canonical_path);
    return true;
  }

  if (currently_checking_.count(canonical_path)) {
    std::string error_msg = "Circular import detected:\n";
    for (const auto &check_file : check_stack_) {
      error_msg += "  " + check_file + " imports\n";
    }
    error_msg += "  " + canonical_path + " (cycle detected)";
    logger_->error("{}", error_msg);
    throw std::runtime_error(error_msg);
  }

  currently_checking_.insert(canonical_path);
  check_stack_.push_back(canonical_path);
  current_file_ = canonical_path;

  std::ifstream file(file_path);
  if (!file.is_open()) {
    logger_->error("Failed to open file: {}", file_path);
    currently_checking_.erase(canonical_path);
    check_stack_.pop_back();
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  file.close();

  bool result = check_source(source, canonical_path);

  currently_checking_.erase(canonical_path);
  check_stack_.pop_back();
  if (result) {
    checked_files_.insert(canonical_path);
  }

  return result;
}

bool tcs_c::check_source(const std::string &source,
                         const std::string &source_name) {
  logger_->info("Type checking: {}", source_name);

  auto parse_result = slp::parse(source);
  if (parse_result.is_error()) {
    const auto &error = parse_result.error();
    logger_->error("Parse error in {}: {}", source_name, error.message);
    return false;
  }

  try {
    auto obj = parse_result.take();
    eval_type(obj);
    logger_->info("Type checking passed: {}", source_name);
    return true;
  } catch (const std::exception &e) {
    logger_->error("Type checking failed in {}: {}", source_name, e.what());
    return false;
  }
}

type_info_s tcs_c::eval_type(slp::slp_object_c &object) {
  auto type = object.type();

  type_info_s result;

  switch (type) {
  case slp::slp_type_e::INTEGER:
    result.base_type = slp::slp_type_e::INTEGER;
    return result;

  case slp::slp_type_e::REAL:
    result.base_type = slp::slp_type_e::REAL;
    return result;

  case slp::slp_type_e::DQ_LIST:
    result.base_type = slp::slp_type_e::DQ_LIST;
    return result;

  case slp::slp_type_e::RUNE:
    result.base_type = slp::slp_type_e::RUNE;
    return result;

  case slp::slp_type_e::SYMBOL: {
    std::string sym = object.as_symbol();

    if (has_symbol(sym)) {
      return get_symbol_type(sym);
    }

    auto slash_pos = sym.find('/');
    if (slash_pos != std::string::npos) {
      std::string prefix = sym.substr(0, slash_pos);
      std::string suffix = sym.substr(slash_pos + 1);

      std::string full_sym = prefix + "/" + suffix;
      if (has_symbol(full_sym)) {
        return get_symbol_type(full_sym);
      }
    }

    result.base_type = slp::slp_type_e::SYMBOL;
    return result;
  }

  case slp::slp_type_e::ABERRANT:
    result.base_type = slp::slp_type_e::ABERRANT;
    return result;

  case slp::slp_type_e::PAREN_LIST: {
    auto list = object.as_list();
    if (list.empty()) {
      result.base_type = slp::slp_type_e::PAREN_LIST;
      return result;
    }

    auto first = list.at(0);
    if (first.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error(fmt::format("Cannot call non-symbol type: {}",
                                           static_cast<int>(first.type())));
    }

    std::string cmd = first.as_symbol();

    if (cmd == "def")
      return handle_def(object);
    if (cmd == "fn")
      return handle_fn(object);
    if (cmd == "if")
      return handle_if(object);
    if (cmd == "match")
      return handle_match(object);
    if (cmd == "reflect")
      return handle_reflect(object);
    if (cmd == "try")
      return handle_try(object);
    if (cmd == "recover")
      return handle_recover(object);
    if (cmd == "assert")
      return handle_assert(object);
    if (cmd == "eval")
      return handle_eval(object);
    if (cmd == "apply")
      return handle_apply(object);
    if (cmd == "export")
      return handle_export(object);
    if (cmd == "debug")
      return handle_debug(object);
    if (cmd == "cast")
      return handle_cast(object);

    if (has_symbol(cmd)) {
      auto sym_type = get_symbol_type(cmd);
      if (sym_type.base_type == slp::slp_type_e::ABERRANT) {
        if (sym_type.lambda_id == 0 ||
            lambda_signatures_.find(sym_type.lambda_id) ==
                lambda_signatures_.end()) {
          throw std::runtime_error(
              fmt::format("Lambda {} has no signature information", cmd));
        }

        const auto &sig = lambda_signatures_[sym_type.lambda_id];

        if (!sig.variadic && list.size() - 1 != sig.parameters.size()) {
          throw std::runtime_error(
              fmt::format("Lambda {} expects {} arguments, got {}", cmd,
                          sig.parameters.size(), list.size() - 1));
        }

        size_t fixed_param_count = sig.parameters.size();
        if (sig.variadic && fixed_param_count > 0) {
          fixed_param_count--;
        }

        if (list.size() - 1 < fixed_param_count) {
          throw std::runtime_error(
              fmt::format("Lambda {} expects at least {} arguments, got {}",
                          cmd, fixed_param_count, list.size() - 1));
        }

        for (size_t i = 0; i < fixed_param_count; i++) {
          auto arg = list.at(i + 1);
          auto arg_type = eval_type(arg);
          if (!types_match(sig.parameters[i], arg_type)) {
            throw std::runtime_error(fmt::format(
                "Lambda {} argument {} type mismatch: expected {}, got {}", cmd,
                i + 1, static_cast<int>(sig.parameters[i].base_type),
                static_cast<int>(arg_type.base_type)));
          }
        }

        if (sig.variadic && sig.parameters.size() > 0) {
          const auto &variadic_param = sig.parameters.back();
          for (size_t i = fixed_param_count; i < list.size() - 1; i++) {
            auto arg = list.at(i + 1);
            auto arg_type = eval_type(arg);
            if (!types_match(variadic_param, arg_type)) {
              throw std::runtime_error(fmt::format(
                  "Lambda {} variadic argument {} type mismatch: expected {}, "
                  "got {}",
                  cmd, i + 1, static_cast<int>(variadic_param.base_type),
                  static_cast<int>(arg_type.base_type)));
            }
          }
        }

        return sig.return_type;
      }
    }

    auto slash_pos = cmd.find('/');
    if (slash_pos != std::string::npos) {
      if (function_signatures_.count(cmd)) {
        const auto &sig = function_signatures_[cmd];

        if (!sig.variadic && list.size() - 1 != sig.parameters.size()) {
          throw std::runtime_error(
              fmt::format("Function {} expects {} arguments, got {}", cmd,
                          sig.parameters.size(), list.size() - 1));
        }

        size_t fixed_param_count = sig.parameters.size();
        if (sig.variadic && fixed_param_count > 0) {
          fixed_param_count--;
        }

        if (list.size() - 1 < fixed_param_count) {
          throw std::runtime_error(
              fmt::format("Function {} expects at least {} arguments, got {}",
                          cmd, fixed_param_count, list.size() - 1));
        }

        for (size_t i = 0; i < fixed_param_count; i++) {
          auto arg = list.at(i + 1);
          auto arg_type = eval_type(arg);
          if (!types_match(sig.parameters[i], arg_type)) {
            throw std::runtime_error(fmt::format(
                "Function {} argument {} type mismatch: expected "
                "{}, got {}",
                cmd, i + 1, static_cast<int>(sig.parameters[i].base_type),
                static_cast<int>(arg_type.base_type)));
          }
        }

        if (sig.variadic && sig.parameters.size() > 0) {
          const auto &variadic_param = sig.parameters.back();
          for (size_t i = fixed_param_count; i < list.size() - 1; i++) {
            auto arg = list.at(i + 1);
            auto arg_type = eval_type(arg);
            if (!types_match(variadic_param, arg_type)) {
              throw std::runtime_error(fmt::format(
                  "Function {} variadic argument {} type mismatch: expected "
                  "{}, got {}",
                  cmd, i + 1, static_cast<int>(variadic_param.base_type),
                  static_cast<int>(arg_type.base_type)));
            }
          }
        }

        return sig.return_type;
      }
    }

    throw std::runtime_error(fmt::format("Unknown callable symbol: {}", cmd));
  }

  case slp::slp_type_e::DATUM: {
    const std::uint8_t *base_ptr = object.get_data().data();
    const std::uint8_t *unit_ptr = base_ptr + object.get_root_offset();
    const slp::slp_unit_of_store_t *unit =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
    size_t inner_offset = static_cast<size_t>(unit->data.uint64);

    auto inner_obj = slp::slp_object_c::from_data(
        object.get_data(), object.get_symbols(), inner_offset);

    if (inner_obj.type() != slp::slp_type_e::PAREN_LIST) {
      result.base_type = slp::slp_type_e::DATUM;
      return result;
    }

    auto list = inner_obj.as_list();
    if (list.empty()) {
      result.base_type = slp::slp_type_e::DATUM;
      return result;
    }

    auto first = list.at(0);
    if (first.type() != slp::slp_type_e::SYMBOL) {
      result.base_type = slp::slp_type_e::DATUM;
      return result;
    }

    std::string cmd = first.as_symbol();
    if (cmd == "import")
      return handle_import(inner_obj);
    if (cmd == "load")
      return handle_load(inner_obj);
    if (cmd == "debug")
      return handle_debug(inner_obj);

    result.base_type = slp::slp_type_e::DATUM;
    return result;
  }

  case slp::slp_type_e::BRACKET_LIST: {
    auto list = object.as_list();
    type_info_s last_result;
    last_result.base_type = slp::slp_type_e::NONE;

    for (size_t i = 0; i < list.size(); i++) {
      auto elem = list.at(i);
      last_result = eval_type(elem);
    }
    return last_result;
  }

  default:
    result.base_type = type;
    return result;
  }
}

type_info_s tcs_c::handle_def(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error("def requires exactly 2 arguments");
  }

  auto symbol_obj = list.at(1);
  if (symbol_obj.type() != slp::slp_type_e::SYMBOL) {
    throw std::runtime_error("def requires first argument to be a symbol");
  }

  std::string symbol_name = symbol_obj.as_symbol();

  if (has_symbol(symbol_name, true)) {
    throw std::runtime_error(fmt::format(
        "Symbol '{}' is already defined in current scope", symbol_name));
  }

  auto value_obj = list.at(2);
  auto value_type = eval_type(value_obj);

  define_symbol(symbol_name, value_type);

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_fn(slp::slp_object_c &args_list) {
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
  type_info_s return_type;
  if (!is_type_symbol(return_type_sym, return_type)) {
    throw std::runtime_error(
        fmt::format("fn: invalid return type: {}", return_type_sym));
  }

  auto params_list = params_obj.as_list();
  std::vector<type_info_s> parameters;

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

    std::string param_type_sym = param_type_obj.as_symbol();
    type_info_s param_type;

    if (!is_type_symbol(param_type_sym, param_type)) {
      throw std::runtime_error(
          fmt::format("fn: invalid parameter type: {}", param_type_sym));
    }

    parameters.push_back(param_type);
  }

  push_scope();

  for (size_t i = 0; i < params_list.size(); i += 2) {
    std::string param_name = params_list.at(i).as_symbol();
    define_symbol(param_name, parameters[i / 2]);
  }

  auto body_type = eval_type(body_obj);

  pop_scope();

  if (!types_match(return_type, body_type)) {
    throw std::runtime_error(
        fmt::format("fn: body returns type {}, but declared return type is {}",
                    static_cast<int>(body_type.base_type),
                    static_cast<int>(return_type.base_type)));
  }

  std::uint64_t lambda_id = next_lambda_id_++;
  function_signature_s sig;
  sig.parameters = parameters;
  sig.return_type = return_type;
  sig.variadic = false;
  lambda_signatures_[lambda_id] = sig;

  std::string signature = ":fn<";
  for (size_t i = 0; i < parameters.size(); i++) {
    if (i > 0)
      signature += ",";
    signature += std::to_string(static_cast<int>(parameters[i].base_type));
  }
  signature += ">";
  signature += std::to_string(static_cast<int>(return_type.base_type));

  type_info_s result;
  result.base_type = slp::slp_type_e::ABERRANT;
  result.lambda_signature = signature;
  result.lambda_id = lambda_id;
  return result;
}

type_info_s tcs_c::handle_if(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 4) {
    throw std::runtime_error("if requires exactly 3 arguments: condition, "
                             "true-branch, false-branch");
  }

  auto condition_obj = list.at(1);
  auto true_branch_obj = list.at(2);
  auto false_branch_obj = list.at(3);

  auto condition_type = eval_type(condition_obj);
  if (condition_type.base_type != slp::slp_type_e::INTEGER) {
    throw std::runtime_error("if: condition must be an integer");
  }

  auto true_type = eval_type(true_branch_obj);
  auto false_type = eval_type(false_branch_obj);

  if (!types_match(true_type, false_type)) {
    throw std::runtime_error(
        fmt::format("if: both branches must return the same type, got {} and "
                    "{}",
                    static_cast<int>(true_type.base_type),
                    static_cast<int>(false_type.base_type)));
  }

  return true_type;
}

type_info_s tcs_c::handle_match(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() < 3) {
    throw std::runtime_error(
        "match requires at least 2 arguments: value and one handler");
  }

  auto value_obj = list.at(1);
  auto value_type = eval_type(value_obj);

  if (value_type.base_type == slp::slp_type_e::ABERRANT) {
    throw std::runtime_error("match: cannot match on aberrant (lambda) types");
  }

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
    auto pattern_type = eval_type(pattern_obj);

    if (pattern_type.base_type != value_type.base_type) {
      logger_->warn("match: pattern type {} does not match value type {}",
                    static_cast<int>(pattern_type.base_type),
                    static_cast<int>(value_type.base_type));
    }

    auto result_obj = handler_list.at(1);
    eval_type(result_obj);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_reflect(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() < 3) {
    throw std::runtime_error(
        "reflect requires at least 2 arguments: value and one handler");
  }

  auto value_obj = list.at(1);
  eval_type(value_obj);

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
    type_info_s handler_type;

    if (!is_type_symbol(type_symbol, handler_type)) {
      throw std::runtime_error(
          fmt::format("reflect: invalid type symbol: {}", type_symbol));
    }

    auto body = handler_list.at(1);
    eval_type(body);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_try(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "try requires exactly 2 arguments: body and handler");
  }

  auto body_obj = list.at(1);
  auto handler_obj = list.at(2);

  auto body_type = eval_type(body_obj);

  if (handler_obj.type() == slp::slp_type_e::BRACKET_LIST) {
    push_scope();
    type_info_s error_type;
    error_type.base_type = slp::slp_type_e::NONE;
    define_symbol("$error", error_type);
    auto handler_type = eval_type(handler_obj);
    pop_scope();

    if (!types_match(body_type, handler_type)) {
      throw std::runtime_error(fmt::format(
          "try: body and handler must return the same type, got {} and {}",
          static_cast<int>(body_type.base_type),
          static_cast<int>(handler_type.base_type)));
    }
    return body_type;
  } else {
    auto handler_type = eval_type(handler_obj);
    if (!types_match(body_type, handler_type)) {
      throw std::runtime_error(fmt::format(
          "try: body and handler must return the same type, got {} and {}",
          static_cast<int>(body_type.base_type),
          static_cast<int>(handler_type.base_type)));
    }
    return body_type;
  }
}

type_info_s tcs_c::handle_recover(slp::slp_object_c &args_list) {
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

  auto body_type = eval_type(body_obj);

  push_scope();
  type_info_s exception_type;
  exception_type.base_type = slp::slp_type_e::DQ_LIST;
  define_symbol("$exception", exception_type);
  auto handler_type = eval_type(handler_obj);
  pop_scope();

  if (!types_match(body_type, handler_type)) {
    throw std::runtime_error(fmt::format(
        "recover: body and handler must return the same type, got {} and {}",
        static_cast<int>(body_type.base_type),
        static_cast<int>(handler_type.base_type)));
  }

  return body_type;
}

type_info_s tcs_c::handle_assert(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "assert requires exactly 2 arguments: condition and message");
  }

  auto condition_obj = list.at(1);
  auto message_obj = list.at(2);

  auto condition_type = eval_type(condition_obj);
  auto message_type = eval_type(message_obj);

  if (condition_type.base_type != slp::slp_type_e::INTEGER) {
    throw std::runtime_error("assert: condition must be an integer");
  }

  if (message_type.base_type != slp::slp_type_e::DQ_LIST) {
    throw std::runtime_error("assert: message must be a string");
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_cast(slp::slp_object_c &args_list) {
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
  type_info_s expected_type;

  if (!is_type_symbol(type_symbol, expected_type)) {
    throw std::runtime_error(
        fmt::format("cast: invalid type symbol: {}", type_symbol));
  }

  eval_type(value_obj);

  return expected_type;
}

type_info_s tcs_c::handle_eval(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 2) {
    throw std::runtime_error("eval requires exactly 1 argument: code string");
  }

  auto code_obj = list.at(1);
  auto code_type = eval_type(code_obj);

  if (code_type.base_type != slp::slp_type_e::DQ_LIST) {
    throw std::runtime_error("eval: argument must be a string");
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_apply(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() != 3) {
    throw std::runtime_error(
        "apply requires exactly 2 arguments: lambda and args-list");
  }

  auto lambda_obj = list.at(1);
  auto args_obj = list.at(2);

  auto lambda_type = eval_type(lambda_obj);
  if (lambda_type.base_type != slp::slp_type_e::ABERRANT) {
    throw std::runtime_error(
        "apply: first argument must be a lambda (aberrant type)");
  }

  auto args_type = eval_type(args_obj);
  if (args_type.base_type != slp::slp_type_e::BRACE_LIST) {
    throw std::runtime_error(
        "apply: second argument must be a brace list of arguments");
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_export(slp::slp_object_c &args_list) {
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
  auto value_type = eval_type(value_obj);

  define_symbol(export_name, value_type);
  current_exports_[export_name] = value_type;

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_debug(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  for (size_t i = 1; i < list.size(); i++) {
    auto elem = list.at(i);
    eval_type(elem);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::INTEGER;
  return result;
}

type_info_s tcs_c::handle_import(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() < 3) {
    throw std::runtime_error(
        "import requires at least 2 arguments: symbol and file_path");
  }

  if ((list.size() - 1) % 2 != 0) {
    throw std::runtime_error("import requires pairs of arguments: symbol "
                             "file_path [symbol file_path ...]");
  }

  for (size_t i = 1; i < list.size(); i += 2) {
    auto symbol_obj = list.at(i);
    auto file_path_obj = list.at(i + 1);

    if (symbol_obj.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error("import: symbol arguments must be symbols");
    }

    if (file_path_obj.type() != slp::slp_type_e::DQ_LIST) {
      throw std::runtime_error("import: file path arguments must be strings");
    }

    std::string symbol = symbol_obj.as_symbol();
    std::string file_path = file_path_obj.as_string().to_string();

    auto resolved_path = resolve_file_path(file_path);
    if (resolved_path.empty()) {
      throw std::runtime_error(
          fmt::format("import: could not resolve file: {}", file_path));
    }

    tcs_c import_checker(logger_, include_paths_, working_directory_);
    if (!import_checker.check(resolved_path)) {
      throw std::runtime_error(
          fmt::format("import: type checking failed for {}", resolved_path));
    }

    std::map<std::uint64_t, std::uint64_t> lambda_id_remapping;
    for (const auto &[old_lambda_id, sig] : import_checker.lambda_signatures_) {
      std::uint64_t new_lambda_id = next_lambda_id_++;
      lambda_signatures_[new_lambda_id] = sig;
      lambda_id_remapping[old_lambda_id] = new_lambda_id;
    }

    for (const auto &[export_name, export_type] :
         import_checker.current_exports_) {
      std::string prefixed_name = symbol + "/" + export_name;
      type_info_s remapped_type = export_type;
      if (export_type.lambda_id != 0 &&
          lambda_id_remapping.count(export_type.lambda_id)) {
        remapped_type.lambda_id = lambda_id_remapping[export_type.lambda_id];
      }
      define_symbol(prefixed_name, remapped_type);
    }
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s tcs_c::handle_load(slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  if (list.size() < 2) {
    throw std::runtime_error("load requires at least 1 argument: kernel_name");
  }

  for (size_t i = 1; i < list.size(); i++) {
    auto kernel_name_obj = list.at(i);

    if (kernel_name_obj.type() != slp::slp_type_e::DQ_LIST) {
      throw std::runtime_error(
          "load: all arguments must be strings (kernel names)");
    }

    std::string kernel_name = kernel_name_obj.as_string().to_string();
    auto kernel_dir = resolve_kernel_path(kernel_name);

    if (kernel_dir.empty()) {
      throw std::runtime_error(
          fmt::format("load: could not resolve kernel: {}", kernel_name));
    }

    if (!load_kernel_types(kernel_name, kernel_dir)) {
      throw std::runtime_error(
          fmt::format("load: failed to load kernel types for {}", kernel_name));
    }
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

void tcs_c::push_scope() { scopes_.emplace_back(); }

void tcs_c::pop_scope() {
  if (!scopes_.empty()) {
    scopes_.pop_back();
  }
}

bool tcs_c::has_symbol(const std::string &symbol, bool local_scope_only) {
  if (local_scope_only) {
    return !scopes_.empty() &&
           scopes_.back().find(symbol) != scopes_.back().end();
  }

  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    if (it->find(symbol) != it->end()) {
      return true;
    }
  }
  return false;
}

void tcs_c::define_symbol(const std::string &symbol, const type_info_s &type) {
  if (!scopes_.empty()) {
    scopes_.back()[symbol] = type;
  }
}

type_info_s tcs_c::get_symbol_type(const std::string &symbol) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(symbol);
    if (found != it->end()) {
      return found->second;
    }
  }
  throw std::runtime_error(
      fmt::format("Symbol '{}' not found in any scope", symbol));
}

bool tcs_c::is_type_symbol(const std::string &symbol, type_info_s &out_type) {
  auto it = type_symbol_map_.find(symbol);
  if (it != type_symbol_map_.end()) {
    out_type = it->second;
    return true;
  }
  return false;
}

std::string tcs_c::resolve_file_path(const std::string &file_path) {
  if (std::filesystem::path(file_path).is_absolute()) {
    if (std::filesystem::exists(file_path)) {
      return file_path;
    }
  }

  for (const auto &include_path : include_paths_) {
    auto full_path = std::filesystem::path(include_path) / file_path;
    if (std::filesystem::exists(full_path)) {
      return full_path.string();
    }
  }

  auto working_path = std::filesystem::path(working_directory_) / file_path;
  if (std::filesystem::exists(working_path)) {
    return working_path.string();
  }

  return "";
}

std::string tcs_c::resolve_kernel_path(const std::string &kernel_name) {
  if (std::filesystem::path(kernel_name).is_absolute()) {
    if (std::filesystem::exists(kernel_name)) {
      return kernel_name;
    }
  }

  for (const auto &include_path : include_paths_) {
    auto kernel_path = std::filesystem::path(include_path) / kernel_name;
    if (std::filesystem::exists(kernel_path / "kernel.sxs")) {
      return kernel_path.string();
    }
  }

  auto working_kernel_path =
      std::filesystem::path(working_directory_) / kernel_name;
  if (std::filesystem::exists(working_kernel_path / "kernel.sxs")) {
    return working_kernel_path.string();
  }

  return "";
}

bool tcs_c::load_kernel_types(const std::string &kernel_name,
                              const std::string &kernel_dir) {
  auto kernel_sxs_path = std::filesystem::path(kernel_dir) / "kernel.sxs";

  std::ifstream file(kernel_sxs_path);
  if (!file.is_open()) {
    logger_->error("Could not open kernel.sxs: {}", kernel_sxs_path.string());
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  file.close();

  auto parse_result = slp::parse(source);
  if (parse_result.is_error()) {
    logger_->error("Failed to parse kernel.sxs: {}",
                   parse_result.error().message);
    return false;
  }

  auto kernel_obj = parse_result.take();
  if (kernel_obj.type() != slp::slp_type_e::DATUM) {
    logger_->error("kernel.sxs must start with #(define-kernel ...)");
    return false;
  }

  const std::uint8_t *base_ptr = kernel_obj.get_data().data();
  const std::uint8_t *unit_ptr = base_ptr + kernel_obj.get_root_offset();
  const slp::slp_unit_of_store_t *unit =
      reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
  size_t inner_offset = static_cast<size_t>(unit->data.uint64);

  auto inner_obj = slp::slp_object_c::from_data(
      kernel_obj.get_data(), kernel_obj.get_symbols(), inner_offset);

  if (inner_obj.type() != slp::slp_type_e::PAREN_LIST) {
    logger_->error("kernel.sxs define-kernel must be a list");
    return false;
  }

  auto list = inner_obj.as_list();
  if (list.size() < 4) {
    logger_->error("kernel.sxs define-kernel requires: name dylib functions");
    return false;
  }

  auto functions_obj = list.at(3);
  if (functions_obj.type() != slp::slp_type_e::BRACKET_LIST) {
    logger_->error("kernel.sxs functions must be a bracket list");
    return false;
  }

  auto functions_list = functions_obj.as_list();
  for (size_t i = 0; i < functions_list.size(); i++) {
    auto func_def = functions_list.at(i);
    if (func_def.type() != slp::slp_type_e::PAREN_LIST) {
      logger_->warn("kernel.sxs: skipping non-list function definition");
      continue;
    }

    auto func_list = func_def.as_list();
    if (func_list.size() < 4) {
      logger_->warn("kernel.sxs: function definition requires at least 4 "
                    "elements");
      continue;
    }

    auto cmd = func_list.at(0);
    if (cmd.type() != slp::slp_type_e::SYMBOL ||
        cmd.as_symbol() != std::string("define-function")) {
      continue;
    }

    auto func_name_obj = func_list.at(1);
    if (func_name_obj.type() != slp::slp_type_e::SYMBOL) {
      logger_->warn("kernel.sxs: function name must be a symbol");
      continue;
    }
    std::string func_name = func_name_obj.as_symbol();

    auto params_obj = func_list.at(2);
    if (params_obj.type() != slp::slp_type_e::PAREN_LIST) {
      logger_->warn("kernel.sxs: function parameters must be a list");
      continue;
    }

    auto return_type_obj = func_list.at(3);
    if (return_type_obj.type() != slp::slp_type_e::SYMBOL) {
      logger_->warn("kernel.sxs: function return type must be a symbol");
      continue;
    }

    std::string return_type_sym = return_type_obj.as_symbol();
    type_info_s return_type;
    if (!is_type_symbol(return_type_sym, return_type)) {
      logger_->error("kernel.sxs: invalid return type: {}", return_type_sym);
      continue;
    }

    auto params_list = params_obj.as_list();
    std::vector<type_info_s> parameters;
    bool variadic = false;

    for (size_t j = 0; j < params_list.size(); j += 2) {
      if (j + 1 >= params_list.size()) {
        logger_->warn("kernel.sxs: parameters must be in pairs");
        break;
      }

      auto param_type_obj = params_list.at(j + 1);
      if (param_type_obj.type() != slp::slp_type_e::SYMBOL) {
        logger_->warn("kernel.sxs: parameter type must be a symbol");
        continue;
      }

      std::string param_type_sym = param_type_obj.as_symbol();
      type_info_s param_type;

      if (!is_type_symbol(param_type_sym, param_type)) {
        logger_->error("kernel.sxs: invalid parameter type: {}",
                       param_type_sym);
        continue;
      }

      if (param_type.is_variadic) {
        variadic = true;
      }

      parameters.push_back(param_type);
    }

    function_signature_s sig;
    sig.parameters = parameters;
    sig.return_type = return_type;
    sig.variadic = variadic;

    std::string full_func_name = kernel_name + "/" + func_name;
    function_signatures_[full_func_name] = sig;

    logger_->debug("Registered kernel function: {}", full_func_name);
  }

  return true;
}

bool tcs_c::types_match(const type_info_s &expected,
                        const type_info_s &actual) {
  if (expected.base_type == slp::slp_type_e::NONE &&
      expected.lambda_signature.empty()) {
    return true;
  }

  if (expected.base_type == slp::slp_type_e::PAREN_LIST &&
      actual.base_type == slp::slp_type_e::PAREN_LIST) {
    return true;
  }

  if (expected.base_type == actual.base_type) {
    return true;
  }

  return false;
}

} // namespace pkg::core::tcs
