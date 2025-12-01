#include "type_checker.hpp"
#include "core/compiler_context.hpp"
#include "core/instructions/datum.hpp"
#include "core/instructions/instructions.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace pkg::core::type_checker {

type_checker_c::type_checker_c(logger_t logger,
                               std::vector<std::string> include_paths,
                               std::string working_directory)
    : logger_(logger), include_paths_(std::move(include_paths)),
      working_directory_(std::move(working_directory)) {}

type_checker_c::~type_checker_c() = default;

bool type_checker_c::check(const std::string &file_path) {
  if (!std::filesystem::exists(file_path)) {
    logger_->error("File does not exist: {}", file_path);
    return false;
  }

  auto canonical_path = std::filesystem::canonical(file_path).string();

  std::ifstream file(file_path);
  if (!file.is_open()) {
    logger_->error("Failed to open file: {}", file_path);
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  file.close();

  return check_source(source, canonical_path);
}

bool type_checker_c::check_source(const std::string &source,
                                  const std::string &source_name) {
  logger_->info("Type checking: {}", source_name);

  auto parse_result = slp::parse(source);
  if (parse_result.is_error()) {
    const auto &error = parse_result.error();
    logger_->error("Parse error in {}: {}", source_name, error.message);
    return false;
  }

  try {
    auto symbols = instructions::get_standard_callable_symbols();
    auto datum_symbols = datum::get_standard_callable_symbols();
    symbols.insert(datum_symbols.begin(), datum_symbols.end());

    auto context = create_compiler_context(logger_, include_paths_,
                                           working_directory_, symbols);

    context->set_current_file(source_name);

    auto obj = parse_result.take();
    context->eval_type(obj);

    logger_->info("Type checking passed: {}", source_name);
    return true;
  } catch (const std::exception &e) {
    logger_->error("Type checking failed in {}: {}", source_name, e.what());
    return false;
  }
}

type_info_s type_checker_c::check_expression(const std::string &source,
                                             const std::string &source_name) {
  auto parse_result = slp::parse(source);
  if (parse_result.is_error()) {
    const auto &error = parse_result.error();
    throw std::runtime_error("Parse error: " + error.message);
  }

  auto symbols = instructions::get_standard_callable_symbols();
  auto datum_symbols = datum::get_standard_callable_symbols();
  symbols.insert(datum_symbols.begin(), datum_symbols.end());

  auto context = create_compiler_context(logger_, include_paths_,
                                         working_directory_, symbols);

  context->set_current_file(source_name);

  auto obj = parse_result.take();
  return context->eval_type(obj);
}

} // namespace pkg::core::type_checker
