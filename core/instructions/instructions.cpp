#include "instructions.hpp"
#include "core/interpreter.hpp"
#include "interpretation/interpretation.hpp"
#include "typechecking/typechecking.hpp"
#include <fmt/core.h>

namespace pkg::core::instructions {

std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols() {
  std::map<std::string, pkg::core::callable_symbol_s> symbols;

  symbols["def"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .instruction_generator = generation::make_define,
      .required_parameters = {{.name = "symbol",
                               .type = slp::slp_type_e::SYMBOL},
                              {.name = "value",
                               .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_define,
      .typecheck_function = typechecking::typecheck_define};

  symbols["fn"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_fn,
      .required_parameters =
          {{.name = "params", .type = slp::slp_type_e::PAREN_LIST},
           {.name = "return_type", .type = slp::slp_type_e::SYMBOL},
           {.name = "body", .type = slp::slp_type_e::BRACKET_LIST}},
      .variadic = false,
      .function = interpretation::interpret_fn,
      .typecheck_function = typechecking::typecheck_fn};

  symbols["debug"] =
      callable_symbol_s{.return_type = slp::slp_type_e::NONE,
                        .instruction_generator = generation::make_debug,
                        .required_parameters = {},
                        .variadic = true,
                        .function = interpretation::interpret_debug,
                        .typecheck_function = typechecking::typecheck_debug};

  symbols["export"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .instruction_generator = generation::make_export,
      .required_parameters = {{.name = "name", .type = slp::slp_type_e::SYMBOL},
                              {.name = "value",
                               .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_export,
      .typecheck_function = typechecking::typecheck_export};

  symbols["if"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_if,
      .required_parameters =
          {{.name = "condition", .type = slp::slp_type_e::ABERRANT},
           {.name = "true_branch", .type = slp::slp_type_e::ABERRANT},
           {.name = "false_branch", .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_if,
      .typecheck_function = typechecking::typecheck_if};

  symbols["reflect"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_reflect,
      .required_parameters = {{.name = "value",
                               .type = slp::slp_type_e::ABERRANT},
                              {.name = "handler",
                               .type = slp::slp_type_e::PAREN_LIST}},
      .variadic = true,
      .function = interpretation::interpret_reflect,
      .typecheck_function = typechecking::typecheck_reflect};

  symbols["try"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_try,
      .required_parameters = {{.name = "body",
                               .type = slp::slp_type_e::ABERRANT},
                              {.name = "handler",
                               .type = slp::slp_type_e::ABERRANT}},
      .injected_symbols = {{"$error", slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_try,
      .typecheck_function = typechecking::typecheck_try};

  symbols["assert"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .instruction_generator = generation::make_assert,
      .required_parameters = {{.name = "condition",
                               .type = slp::slp_type_e::INTEGER},
                              {.name = "message",
                               .type = slp::slp_type_e::DQ_LIST}},
      .variadic = false,
      .function = interpretation::interpret_assert,
      .typecheck_function = typechecking::typecheck_assert};

  symbols["recover"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_recover,
      .required_parameters = {{.name = "body",
                               .type = slp::slp_type_e::BRACKET_LIST},
                              {.name = "handler",
                               .type = slp::slp_type_e::BRACKET_LIST}},
      .injected_symbols = {{"$exception", slp::slp_type_e::DQ_LIST}},
      .variadic = false,
      .function = interpretation::interpret_recover,
      .typecheck_function = typechecking::typecheck_recover};

  symbols["eval"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_eval,
      .required_parameters = {{.name = "code",
                               .type = slp::slp_type_e::DQ_LIST}},
      .variadic = false,
      .function = interpretation::interpret_eval,
      .typecheck_function = typechecking::typecheck_eval};

  symbols["apply"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_apply,
      .required_parameters = {{.name = "lambda",
                               .type = slp::slp_type_e::ABERRANT},
                              {.name = "args",
                               .type = slp::slp_type_e::BRACE_LIST}},
      .variadic = false,
      .function = interpretation::interpret_apply,
      .typecheck_function = typechecking::typecheck_apply};

  symbols["match"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_match,
      .required_parameters = {{.name = "value",
                               .type = slp::slp_type_e::ABERRANT},
                              {.name = "handler",
                               .type = slp::slp_type_e::PAREN_LIST}},
      .variadic = true,
      .function = interpretation::interpret_match,
      .typecheck_function = typechecking::typecheck_match};

  symbols["cast"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_cast,
      .required_parameters = {{.name = "type", .type = slp::slp_type_e::SYMBOL},
                              {.name = "value",
                               .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_cast,
      .typecheck_function = typechecking::typecheck_cast};

  symbols["do"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_do,
      .required_parameters = {{.name = "body",
                               .type = slp::slp_type_e::BRACKET_LIST}},
      .injected_symbols = {{"$iterations", slp::slp_type_e::INTEGER}},
      .variadic = false,
      .function = interpretation::interpret_do,
      .typecheck_function = typechecking::typecheck_do};

  symbols["done"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .instruction_generator = generation::make_done,
      .required_parameters = {{.name = "value",
                               .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_done,
      .typecheck_function = typechecking::typecheck_done};

  symbols["at"] = callable_symbol_s{
      .return_type = slp::slp_type_e::ABERRANT,
      .instruction_generator = generation::make_at,
      .required_parameters = {{.name = "index",
                               .type = slp::slp_type_e::INTEGER},
                              {.name = "collection",
                               .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_at,
      .typecheck_function = typechecking::typecheck_at};

  symbols["eq"] = callable_symbol_s{
      .return_type = slp::slp_type_e::INTEGER,
      .instruction_generator = generation::make_eq,
      .required_parameters = {{.name = "lhs",
                               .type = slp::slp_type_e::ABERRANT},
                              {.name = "rhs",
                               .type = slp::slp_type_e::ABERRANT}},
      .variadic = false,
      .function = interpretation::interpret_eq,
      .typecheck_function = typechecking::typecheck_eq};

  return symbols;
}

} // namespace pkg::core::instructions
