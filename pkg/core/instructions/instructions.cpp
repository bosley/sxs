#include "instructions.hpp"
#include "core/interpreter.hpp"
#include <fmt/core.h>

namespace pkg::core::instructions {

std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols() {
  std::map<std::string, pkg::core::callable_symbol_s> symbols;

  symbols["set"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {},
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() != 3) {
          throw std::runtime_error("set requires exactly 2 arguments");
        }

        auto symbol_obj = list.at(1);
        if (symbol_obj.type() != slp::slp_type_e::SYMBOL) {
          throw std::runtime_error(
              "set requires first argument to be a symbol");
        }

        std::string symbol_name = symbol_obj.as_symbol();
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

  return symbols;
}

} // namespace pkg::core::instructions
