#include "datum.hpp"
#include "core/instructions/interpretation/interpretation.hpp"
#include "core/instructions/typechecking/typechecking.hpp"

namespace pkg::core::datum {

std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols() {
  std::map<std::string, pkg::core::callable_symbol_s> symbols;

  symbols["load"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {{.name = "kernel_name",
                               .type = slp::slp_type_e::DQ_LIST}},
      .variadic = true,
      .function = instructions::interpretation::interpret_datum_load,
      .typecheck_function = instructions::typechecking::typecheck_load};

  symbols["define-form"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {{.name = "name", .type = slp::slp_type_e::SYMBOL},
                              {.name = "elements",
                               .type = slp::slp_type_e::BRACE_LIST}},
      .variadic = false,
      .function = instructions::interpretation::interpret_datum_define_form,
      .typecheck_function = instructions::typechecking::typecheck_define_form};

  return symbols;
}

} // namespace pkg::core::datum
