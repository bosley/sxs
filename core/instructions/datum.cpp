#include "datum.hpp"
#include "core/instructions/interpretation/interpretation.hpp"
#include "core/instructions/typechecking/typechecking.hpp"

namespace pkg::core::datum {

std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols() {
  std::map<std::string, pkg::core::callable_symbol_s> symbols;

  symbols["debug"] = callable_symbol_s{
      .return_type = slp::slp_type_e::INTEGER,
      .required_parameters = {},
      .variadic = true,
      .function = instructions::interpretation::interpret_datum_debug,
      .typecheck_function = instructions::typechecking::typecheck_debug};

  symbols["import"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {{.name = "symbol",
                               .type = slp::slp_type_e::SYMBOL},
                              {.name = "file_path",
                               .type = slp::slp_type_e::DQ_LIST}},
      .variadic = true,
      .function = instructions::interpretation::interpret_datum_import,
      .typecheck_function = instructions::typechecking::typecheck_import};

  symbols["load"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {{.name = "kernel_name",
                               .type = slp::slp_type_e::DQ_LIST}},
      .variadic = true,
      .function = instructions::interpretation::interpret_datum_load,
      .typecheck_function = instructions::typechecking::typecheck_load};

  return symbols;
}

} // namespace pkg::core::datum
