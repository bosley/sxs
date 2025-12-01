#include "datum.hpp"
#include "core/imports/imports.hpp"
#include "core/kernels/kernels.hpp"
#include <fmt/core.h>

namespace pkg::core::datum {

std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols() {
  std::map<std::string, pkg::core::callable_symbol_s> symbols;

  symbols["debug"] = callable_symbol_s{
      .return_type = slp::slp_type_e::INTEGER,
      .variadic = true,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        fmt::print("[DEBUG DATUM]");

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

  symbols["import"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() < 3) {
          throw std::runtime_error(
              "import requires at least 2 arguments: symbol and file_path");
        }

        if ((list.size() - 1) % 2 != 0) {
          throw std::runtime_error("import requires pairs of arguments: symbol "
                                   "file_path [symbol file_path ...]");
        }

        auto import_context = context.get_import_context();
        if (!import_context) {
          throw std::runtime_error("import: no import context available");
        }

        if (!import_context->is_import_allowed()) {
          throw std::runtime_error(
              "import: imports are locked (must occur at start of program)");
        }

        for (size_t i = 1; i < list.size(); i += 2) {
          auto symbol_obj = list.at(i);
          auto file_path_obj = list.at(i + 1);

          if (symbol_obj.type() != slp::slp_type_e::SYMBOL) {
            throw std::runtime_error(
                "import: symbol arguments must be symbols");
          }

          if (file_path_obj.type() != slp::slp_type_e::DQ_LIST) {
            throw std::runtime_error(
                "import: file path arguments must be strings");
          }

          std::string symbol = symbol_obj.as_symbol();
          std::string file_path = file_path_obj.as_string().to_string();

          if (!import_context->attempt_import(symbol, file_path)) {
            throw std::runtime_error(fmt::format(
                "import: failed to import {} from {}", symbol, file_path));
          }
        }

        slp::slp_object_c result;
        return result;
      }};

  symbols["load"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .variadic = false,
      .function = [](callable_context_if &context,
                     slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() < 2) {
          throw std::runtime_error(
              "load requires at least 1 argument: kernel_name");
        }

        auto kernel_context = context.get_kernel_context();
        if (!kernel_context) {
          throw std::runtime_error("load: no kernel context available");
        }

        if (!kernel_context->is_load_allowed()) {
          throw std::runtime_error("load: kernel loading is locked (must occur "
                                   "at start of program)");
        }

        for (size_t i = 1; i < list.size(); i++) {
          auto kernel_name_obj = list.at(i);

          if (kernel_name_obj.type() != slp::slp_type_e::DQ_LIST) {
            throw std::runtime_error(
                "load: all arguments must be strings (kernel names)");
          }

          std::string kernel_name = kernel_name_obj.as_string().to_string();

          if (!kernel_context->attempt_load(kernel_name)) {
            throw std::runtime_error(
                fmt::format("load: failed to load kernel {}", kernel_name));
          }
        }

        slp::slp_object_c result;
        return result;
      }};

  return symbols;
}

} // namespace pkg::core::datum
