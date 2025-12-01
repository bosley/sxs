#include "typechecking.hpp"
#include "core/interpreter.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <sstream>

namespace pkg::core::instructions::typechecking {

static void validate_parameters(compiler_context_if &context,
                                slp::slp_object_c &args_list,
                                const std::string &cmd_name) {
  const auto &symbols = context.get_callable_symbols();
  auto it = symbols.find(cmd_name);
  if (it == symbols.end()) {
    throw std::runtime_error(
        fmt::format("Command '{}' not found in callable symbols", cmd_name));
  }

  const auto &symbol = it->second;
  auto list = args_list.as_list();

  size_t arg_count = list.size() - 1;
  size_t required_count = symbol.required_parameters.size();

  if (!symbol.variadic && arg_count != required_count) {
    throw std::runtime_error(
        fmt::format("{} requires exactly {} argument(s), got {}", cmd_name,
                    required_count, arg_count));
  }

  if (symbol.variadic && arg_count < required_count) {
    throw std::runtime_error(
        fmt::format("{} requires at least {} argument(s), got {}", cmd_name,
                    required_count, arg_count));
  }

  for (size_t i = 0; i < required_count && i < arg_count; i++) {
    auto arg_obj = list.at(i + 1);
    const auto &param = symbol.required_parameters[i];

    if (param.type == slp::slp_type_e::ABERRANT) {
      continue;
    }

    if (param.type == slp::slp_type_e::SYMBOL &&
        arg_obj.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error(fmt::format(
          "{}: parameter '{}' must be a symbol", cmd_name, param.name));
    }

    if (param.type != slp::slp_type_e::ABERRANT &&
        param.type != slp::slp_type_e::SYMBOL && arg_obj.type() != param.type) {
      auto arg_type = context.eval_type(arg_obj);
      if (arg_type.base_type != param.type) {
        throw std::runtime_error(
            fmt::format("{}: parameter '{}' expects type {}, got {}", cmd_name,
                        param.name, static_cast<int>(param.type),
                        static_cast<int>(arg_type.base_type)));
      }
    }
  }
}

type_info_s typecheck_define(compiler_context_if &context,
                             slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "def");

  auto symbol_obj = list.at(1);
  std::string symbol_name = symbol_obj.as_symbol();

  if (context.has_symbol(symbol_name, true)) {
    throw std::runtime_error(fmt::format(
        "Symbol '{}' is already defined in current scope", symbol_name));
  }

  auto value_obj = list.at(2);
  auto value_type = context.eval_type(value_obj);

  context.define_symbol(symbol_name, value_type);

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_fn(compiler_context_if &context,
                         slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "fn");

  auto params_obj = list.at(1);
  auto return_type_obj = list.at(2);
  auto body_obj = list.at(3);

  std::string return_type_sym = return_type_obj.as_symbol();
  type_info_s return_type;
  if (!context.is_type_symbol(return_type_sym, return_type)) {
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

    if (!context.is_type_symbol(param_type_sym, param_type)) {
      throw std::runtime_error(
          fmt::format("fn: invalid parameter type: {}", param_type_sym));
    }

    parameters.push_back(param_type);
  }

  context.push_scope();

  for (size_t i = 0; i < params_list.size(); i += 2) {
    std::string param_name = params_list.at(i).as_symbol();
    context.define_symbol(param_name, parameters[i / 2]);
  }

  auto body_type = context.eval_type(body_obj);

  context.pop_scope();

  if (!context.types_match(return_type, body_type)) {
    throw std::runtime_error(
        fmt::format("fn: body returns type {}, but declared return type is {}",
                    static_cast<int>(body_type.base_type),
                    static_cast<int>(return_type.base_type)));
  }

  std::uint64_t lambda_id = context.allocate_lambda_id();
  function_signature_s sig;
  sig.parameters = parameters;
  sig.return_type = return_type;
  sig.variadic = false;
  context.register_lambda(lambda_id, sig);

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

type_info_s typecheck_if(compiler_context_if &context,
                         slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "if");

  auto condition_obj = list.at(1);
  auto true_branch_obj = list.at(2);
  auto false_branch_obj = list.at(3);

  auto condition_type = context.eval_type(condition_obj);
  if (condition_type.base_type != slp::slp_type_e::INTEGER) {
    throw std::runtime_error("if: condition must be an integer");
  }

  auto true_type = context.eval_type(true_branch_obj);
  auto false_type = context.eval_type(false_branch_obj);

  if (!context.types_match(true_type, false_type)) {
    throw std::runtime_error(
        fmt::format("if: both branches must return the same type, got {} and "
                    "{}",
                    static_cast<int>(true_type.base_type),
                    static_cast<int>(false_type.base_type)));
  }

  return true_type;
}

type_info_s typecheck_match(compiler_context_if &context,
                            slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "match");

  auto value_obj = list.at(1);
  auto value_type = context.eval_type(value_obj);

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
    auto pattern_type = context.eval_type(pattern_obj);

    if (pattern_type.base_type != value_type.base_type) {
      context.get_logger()->warn(
          "match: pattern type {} does not match value type {}",
          static_cast<int>(pattern_type.base_type),
          static_cast<int>(value_type.base_type));
    }

    auto result_obj = handler_list.at(1);
    context.eval_type(result_obj);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_reflect(compiler_context_if &context,
                              slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "reflect");

  auto value_obj = list.at(1);
  context.eval_type(value_obj);

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

    if (!context.is_type_symbol(type_symbol, handler_type)) {
      throw std::runtime_error(
          fmt::format("reflect: invalid type symbol: {}", type_symbol));
    }

    auto body = handler_list.at(1);
    context.eval_type(body);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_try(compiler_context_if &context,
                          slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "try");

  auto body_obj = list.at(1);
  auto handler_obj = list.at(2);

  auto body_type = context.eval_type(body_obj);

  if (handler_obj.type() == slp::slp_type_e::BRACKET_LIST) {
    context.push_scope();

    const auto &symbols = context.get_callable_symbols();
    auto it = symbols.find("try");
    if (it != symbols.end()) {
      for (const auto &[symbol_name, symbol_type] :
           it->second.injected_symbols) {
        type_info_s injected_type;
        injected_type.base_type = symbol_type;
        context.define_symbol(symbol_name, injected_type);
      }
    }

    auto handler_type = context.eval_type(handler_obj);
    context.pop_scope();

    if (!context.types_match(body_type, handler_type)) {
      throw std::runtime_error(fmt::format(
          "try: body and handler must return the same type, got {} and {}",
          static_cast<int>(body_type.base_type),
          static_cast<int>(handler_type.base_type)));
    }
    return body_type;
  } else {
    auto handler_type = context.eval_type(handler_obj);
    if (!context.types_match(body_type, handler_type)) {
      throw std::runtime_error(fmt::format(
          "try: body and handler must return the same type, got {} and {}",
          static_cast<int>(body_type.base_type),
          static_cast<int>(handler_type.base_type)));
    }
    return body_type;
  }
}

type_info_s typecheck_recover(compiler_context_if &context,
                              slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "recover");

  auto body_obj = list.at(1);
  auto handler_obj = list.at(2);

  auto body_type = context.eval_type(body_obj);

  context.push_scope();

  const auto &symbols = context.get_callable_symbols();
  auto it = symbols.find("recover");
  if (it != symbols.end()) {
    for (const auto &[symbol_name, symbol_type] : it->second.injected_symbols) {
      type_info_s injected_type;
      injected_type.base_type = symbol_type;
      context.define_symbol(symbol_name, injected_type);
    }
  }

  auto handler_type = context.eval_type(handler_obj);
  context.pop_scope();

  if (!context.types_match(body_type, handler_type)) {
    throw std::runtime_error(fmt::format(
        "recover: body and handler must return the same type, got {} and {}",
        static_cast<int>(body_type.base_type),
        static_cast<int>(handler_type.base_type)));
  }

  return body_type;
}

type_info_s typecheck_assert(compiler_context_if &context,
                             slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "assert");

  auto condition_obj = list.at(1);
  auto message_obj = list.at(2);

  auto condition_type = context.eval_type(condition_obj);
  auto message_type = context.eval_type(message_obj);

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

type_info_s typecheck_cast(compiler_context_if &context,
                           slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "cast");

  auto type_obj = list.at(1);
  auto value_obj = list.at(2);

  std::string type_symbol = type_obj.as_symbol();
  type_info_s expected_type;

  if (!context.is_type_symbol(type_symbol, expected_type)) {
    throw std::runtime_error(
        fmt::format("cast: invalid type symbol: {}", type_symbol));
  }

  context.eval_type(value_obj);

  return expected_type;
}

type_info_s typecheck_do(compiler_context_if &context,
                         slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "do");

  auto body_obj = list.at(1);

  context.push_loop_context();
  context.push_scope();

  const auto &symbols = context.get_callable_symbols();
  auto it = symbols.find("do");
  if (it != symbols.end()) {
    for (const auto &[symbol_name, symbol_type] : it->second.injected_symbols) {
      type_info_s injected_type;
      injected_type.base_type = symbol_type;
      context.define_symbol(symbol_name, injected_type);
    }
  }

  context.eval_type(body_obj);

  context.pop_scope();
  context.pop_loop_context();

  type_info_s result;
  result.base_type = slp::slp_type_e::ABERRANT;
  return result;
}

type_info_s typecheck_done(compiler_context_if &context,
                           slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "done");

  if (!context.is_in_loop()) {
    throw std::runtime_error("done called outside of do loop");
  }

  auto value_obj = list.at(1);
  context.eval_type(value_obj);

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_at(compiler_context_if &context,
                         slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "at");

  auto index_obj = list.at(1);
  auto collection_obj = list.at(2);

  auto index_type = context.eval_type(index_obj);
  if (index_type.base_type != slp::slp_type_e::INTEGER) {
    throw std::runtime_error("at: index must be an integer");
  }

  auto collection_type = context.eval_type(collection_obj);
  if (collection_type.base_type != slp::slp_type_e::PAREN_LIST &&
      collection_type.base_type != slp::slp_type_e::BRACKET_LIST &&
      collection_type.base_type != slp::slp_type_e::BRACE_LIST &&
      collection_type.base_type != slp::slp_type_e::DQ_LIST) {
    throw std::runtime_error("at: collection must be a list or string type");
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_eq(compiler_context_if &context,
                         slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "eq");

  auto lhs_obj = list.at(1);
  auto rhs_obj = list.at(2);

  context.eval_type(lhs_obj);
  context.eval_type(rhs_obj);

  type_info_s result;
  result.base_type = slp::slp_type_e::INTEGER;
  return result;
}

type_info_s typecheck_eval(compiler_context_if &context,
                           slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "eval");

  auto code_obj = list.at(1);
  auto code_type = context.eval_type(code_obj);

  if (code_type.base_type != slp::slp_type_e::DQ_LIST) {
    throw std::runtime_error("eval: argument must be a string");
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_apply(compiler_context_if &context,
                            slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "apply");

  auto lambda_obj = list.at(1);
  auto args_obj = list.at(2);

  auto lambda_type = context.eval_type(lambda_obj);
  if (lambda_type.base_type != slp::slp_type_e::ABERRANT) {
    throw std::runtime_error(
        "apply: first argument must be a lambda (aberrant type)");
  }

  auto args_type = context.eval_type(args_obj);
  if (args_type.base_type != slp::slp_type_e::BRACE_LIST) {
    throw std::runtime_error(
        "apply: second argument must be a brace list of arguments");
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_export(compiler_context_if &context,
                             slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "export");

  auto name_obj = list.at(1);
  std::string export_name = name_obj.as_symbol();

  auto value_obj = list.at(2);
  auto value_type = context.eval_type(value_obj);

  context.define_symbol(export_name, value_type);
  context.get_current_exports()[export_name] = value_type;

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_debug(compiler_context_if &context,
                            slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "debug");

  for (size_t i = 1; i < list.size(); i++) {
    auto elem = list.at(i);
    context.eval_type(elem);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::INTEGER;
  return result;
}

type_info_s typecheck_import(compiler_context_if &context,
                             slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "import");

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

    auto resolved_path = context.resolve_file_path(file_path);
    if (resolved_path.empty()) {
      throw std::runtime_error(
          fmt::format("import: could not resolve file: {}", file_path));
    }

    auto canonical_path = std::filesystem::canonical(resolved_path).string();

    if (context.get_checked_files().count(canonical_path)) {
      context.get_logger()->debug("File already checked: {}", canonical_path);
      continue;
    }

    if (context.get_currently_checking().count(canonical_path)) {
      std::string error_msg = "Circular import detected:\n";
      for (const auto &check_file : context.get_check_stack()) {
        error_msg += "  " + check_file + " imports\n";
      }
      error_msg += "  " + canonical_path + " (cycle detected)";
      context.get_logger()->error("{}", error_msg);
      throw std::runtime_error(error_msg);
    }

    context.get_currently_checking().insert(canonical_path);
    context.get_check_stack().push_back(canonical_path);

    std::ifstream file(canonical_path);
    if (!file.is_open()) {
      context.get_currently_checking().erase(canonical_path);
      context.get_check_stack().pop_back();
      throw std::runtime_error(
          fmt::format("import: failed to open file: {}", canonical_path));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    auto parse_result = slp::parse(source);
    if (parse_result.is_error()) {
      context.get_currently_checking().erase(canonical_path);
      context.get_check_stack().pop_back();
      throw std::runtime_error(fmt::format("import: parse error in {}: {}",
                                           canonical_path,
                                           parse_result.error().message));
    }

    auto symbols = context.get_callable_symbols();
    auto import_context = create_compiler_context(
        context.get_logger(), context.get_include_paths(),
        context.get_working_directory(), symbols);

    import_context->set_current_file(canonical_path);

    try {
      auto obj = parse_result.take();
      import_context->eval_type(obj);
    } catch (const std::exception &e) {
      context.get_currently_checking().erase(canonical_path);
      context.get_check_stack().pop_back();
      throw std::runtime_error(fmt::format(
          "import: type checking failed for {}: {}", canonical_path, e.what()));
    }

    std::map<std::uint64_t, std::uint64_t> lambda_id_remapping;
    for (const auto &[export_name, export_type] :
         import_context->get_current_exports()) {
      std::string prefixed_name = symbol + "/" + export_name;
      type_info_s remapped_type = export_type;

      if (export_type.lambda_id != 0) {
        std::uint64_t new_lambda_id = context.allocate_lambda_id();
        auto sig = import_context->get_lambda_signature(export_type.lambda_id);
        context.register_lambda(new_lambda_id, sig);
        remapped_type.lambda_id = new_lambda_id;
      }

      context.define_symbol(prefixed_name, remapped_type);
    }

    context.get_currently_checking().erase(canonical_path);
    context.get_check_stack().pop_back();
    context.get_checked_files().insert(canonical_path);
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

type_info_s typecheck_load(compiler_context_if &context,
                           slp::slp_object_c &args_list) {
  auto list = args_list.as_list();
  validate_parameters(context, args_list, "load");

  for (size_t i = 1; i < list.size(); i++) {
    auto kernel_name_obj = list.at(i);

    if (kernel_name_obj.type() != slp::slp_type_e::DQ_LIST) {
      throw std::runtime_error(
          "load: all arguments must be strings (kernel names)");
    }

    std::string kernel_name = kernel_name_obj.as_string().to_string();
    auto kernel_dir = context.resolve_kernel_path(kernel_name);

    if (kernel_dir.empty()) {
      throw std::runtime_error(
          fmt::format("load: could not resolve kernel: {}", kernel_name));
    }

    if (!context.load_kernel_types(kernel_name, kernel_dir)) {
      throw std::runtime_error(
          fmt::format("load: failed to load kernel types for {}", kernel_name));
    }
  }

  type_info_s result;
  result.base_type = slp::slp_type_e::NONE;
  return result;
}

} // namespace pkg::core::instructions::typechecking
