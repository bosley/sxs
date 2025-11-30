#include "instructions.hpp"
#include "core/imports/imports.hpp"
#include "core/interpreter.hpp"
#include <fmt/core.h>

namespace pkg::core::instructions {

std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols() {
  std::map<std::string, pkg::core::callable_symbol_s> symbols;

  symbols["def"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() != 3) {
          throw std::runtime_error("def requires exactly 2 arguments");
        }

        auto symbol_obj = list.at(1);
        if (symbol_obj.type() != slp::slp_type_e::SYMBOL) {
          throw std::runtime_error(
              "def requires first argument to be a symbol");
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
      }};

  symbols["fn"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
          throw std::runtime_error(
              "fn: second argument must be return type symbol");
        }
        if (body_obj.type() != slp::slp_type_e::BRACKET_LIST) {
          throw std::runtime_error(
              "fn: third argument must be bracket list (function body)");
        }

        std::string return_type_sym = return_type_obj.as_symbol();
        slp::slp_type_e return_type;
        if (!context.is_symbol_enscribing_valid_type(return_type_sym,
                                                     return_type)) {
          throw std::runtime_error(
              fmt::format("fn: invalid return type: {}", return_type_sym));
        }

        auto params_list = params_obj.as_list();
        std::vector<callable_parameter_s> parameters;

        for (size_t i = 0; i < params_list.size(); i += 2) {
          if (i + 1 >= params_list.size()) {
            throw std::runtime_error(
                "fn: parameters must be in pairs (name :type)");
          }

          auto param_name_obj = params_list.at(i);
          auto param_type_obj = params_list.at(i + 1);

          if (param_name_obj.type() != slp::slp_type_e::SYMBOL) {
            throw std::runtime_error("fn: parameter name must be a symbol");
          }
          if (param_type_obj.type() != slp::slp_type_e::SYMBOL) {
            throw std::runtime_error(
                "fn: parameter type must be a type symbol");
          }

          std::string param_name = param_name_obj.as_symbol();
          std::string param_type_sym = param_type_obj.as_symbol();
          slp::slp_type_e param_type;

          if (!context.is_symbol_enscribing_valid_type(param_type_sym,
                                                       param_type)) {
            throw std::runtime_error(
                fmt::format("fn: invalid parameter type: {}", param_type_sym));
          }

          parameters.push_back({param_name, param_type});
        }

        std::uint64_t lambda_id = context.allocate_lambda_id();
        context.register_lambda(lambda_id, parameters, return_type, body_obj);

        slp::slp_buffer_c buffer;
        buffer.resize(sizeof(slp::slp_unit_of_store_t));
        auto *unit =
            reinterpret_cast<slp::slp_unit_of_store_t *>(buffer.data());
        unit->header = static_cast<std::uint32_t>(slp::slp_type_e::ABERRANT);
        unit->flags = 0;
        unit->data.uint64 = lambda_id;

        return slp::slp_object_c::from_data(buffer, {}, 0);
      }};

  symbols["debug"] = callable_symbol_s{
      .return_type = slp::slp_type_e::INTEGER,
      .required_parameters = {},
      .variadic = true,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
      }};

  symbols["export"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
      }};

  symbols["if"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() != 4) {
          throw std::runtime_error(
              "if requires exactly 3 arguments: condition, true-branch, "
              "false-branch");
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
      }};

  symbols["reflect"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
          slp::slp_type_e handler_type;

          if (!context.is_symbol_enscribing_valid_type(type_symbol,
                                                       handler_type)) {
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
      }};

  symbols["try"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
      }};

  symbols["assert"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
          throw std::runtime_error(
              "assert: condition must evaluate to an integer");
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
      }};

  symbols["recover"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
            throw std::runtime_error(
                "recover: failed to parse exception string");
          }
          auto exception_str_obj = exception_str_parse.take();

          context.push_scope();
          context.define_symbol("$exception", exception_str_obj);
          auto handler_result = context.eval(handler_obj);
          context.pop_scope();
          return handler_result;
        }
      }};

  symbols["eval"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() != 2) {
          throw std::runtime_error(
              "eval requires exactly 1 argument: code string");
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
      }};

  symbols["apply"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
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
      }};

  symbols["match"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() < 3) {
          throw std::runtime_error(
              "match requires at least 2 arguments: value and one handler");
        }

        auto value_obj = list.at(1);
        auto evaluated_value = context.eval(value_obj);
        auto actual_type = evaluated_value.type();

        if (actual_type == slp::slp_type_e::ABERRANT) {
          throw std::runtime_error(
              "match: cannot match on aberrant (lambda) types");
        }

        for (size_t i = 2; i < list.size(); i++) {
          auto handler = list.at(i);

          if (handler.type() != slp::slp_type_e::PAREN_LIST) {
            throw std::runtime_error(
                "match: handlers must be paren lists like (pattern result)");
          }

          auto handler_list = handler.as_list();
          if (handler_list.size() != 2) {
            throw std::runtime_error("match: handler must have exactly 2 "
                                     "elements: (pattern result)");
          }

          auto pattern_obj = handler_list.at(0);
          auto evaluated_pattern = context.eval(pattern_obj);

          if (evaluated_pattern.type() != actual_type) {
            continue;
          }

          bool values_match = false;
          switch (actual_type) {
          case slp::slp_type_e::INTEGER:
            values_match =
                evaluated_value.as_int() == evaluated_pattern.as_int();
            break;
          case slp::slp_type_e::REAL:
            values_match =
                evaluated_value.as_real() == evaluated_pattern.as_real();
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
      }};

  return symbols;
}

} // namespace pkg::core::instructions
