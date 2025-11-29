#include "core.hpp"
#include "imports/imports.hpp"
#include "instructions/instructions.hpp"
#include "interpreter.hpp"
#include "kernels/kernels.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace pkg::core {

core_c::core_c(const option_s &options) : options_(options) {
  if (!options_.logger) {
    throw std::runtime_error("Logger must be provided");
  }

  if (options_.file_path.empty()) {
    throw std::runtime_error("File path must be provided");
  }

  if (!std::filesystem::exists(options_.file_path)) {
    throw std::runtime_error("File does not exist: " + options_.file_path);
  }

  imports_manager_ = std::make_unique<imports::imports_manager_c>(
      options_.logger->clone("imports"), options_.include_paths,
      options_.working_directory, &import_interpreters_,
      &import_interpreter_locks_);

  kernel_manager_ = std::make_unique<kernels::kernel_manager_c>(
      options_.logger->clone("kernels"), options_.include_paths,
      options_.working_directory);
}

core_c::~core_c() = default;

int core_c::run() {
  try {
    options_.logger->info("Loading SLP file: {}", options_.file_path);

    std::ifstream file(options_.file_path);
    if (!file.is_open()) {
      options_.logger->error("Failed to open file: {}", options_.file_path);
      return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    options_.logger->debug("Source size: {} bytes", source.size());

    auto parse_result = slp::parse(source);

    if (parse_result.is_error()) {
      const auto &error = parse_result.error();
      options_.logger->error("Parse error: {}", error.message);
      options_.logger->error("At byte position: {}", error.byte_position);
      return 1;
    }

    options_.logger->info("Parse successful");

    auto symbols = instructions::get_standard_callable_symbols();
    auto interpreter =
        create_interpreter(symbols, &imports_manager_->get_import_context(),
                           &kernel_manager_->get_kernel_context(),
                           &import_interpreters_, &import_interpreter_locks_);

    imports_manager_->set_parent_context(interpreter.get());
    kernel_manager_->set_parent_context(interpreter.get());

    auto obj = parse_result.take();
    auto result = interpreter->eval(obj);

    auto kernel_functions = kernel_manager_->get_registered_functions();
    for (const auto &[name, symbol] : kernel_functions) {
      options_.logger->debug("Kernel function available: {}", name);
    }

    imports_manager_->lock_imports();
    kernel_manager_->lock_kernels();

    options_.logger->info("Execution complete");

    return 0;

  } catch (const std::exception &e) {
    options_.logger->error("Exception during execution: {}", e.what());
    return 1;
  }
}

} // namespace pkg::core
