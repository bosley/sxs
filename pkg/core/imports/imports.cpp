#include "imports.hpp"
#include "core/instructions/instructions.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace pkg::core::imports {

imports_manager_c::import_guard_c::import_guard_c(
    imports_manager_c &manager, const std::string &canonical_path)
    : manager_(manager), canonical_path_(canonical_path) {
  manager_.currently_importing_.insert(canonical_path_);
  manager_.import_stack_.push_back(canonical_path_);
}

imports_manager_c::import_guard_c::~import_guard_c() {
  manager_.currently_importing_.erase(canonical_path_);
  if (!manager_.import_stack_.empty()) {
    manager_.import_stack_.pop_back();
  }
}

imports_manager_c::imports_manager_c(logger_t logger,
                                     std::vector<std::string> include_paths,
                                     std::string working_directory)
    : logger_(logger), include_paths_(std::move(include_paths)),
      working_directory_(std::move(working_directory)), imports_locked_(false),
      parent_context_(nullptr) {
  context_ = std::make_unique<import_context_c>(*this);
}

imports_manager_c::~imports_manager_c() = default;

import_context_if &imports_manager_c::get_import_context() { return *context_; }

void imports_manager_c::lock_imports() {
  imports_locked_ = true;
  logger_->debug("Imports locked - no more imports allowed");
}

void imports_manager_c::set_parent_context(callable_context_if *context) {
  parent_context_ = context;
}

std::string imports_manager_c::resolve_file_path(const std::string &file_path) {
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

imports_manager_c::import_context_c::~import_context_c() = default;

bool imports_manager_c::import_context_c::is_import_allowed() {
  return !manager_.imports_locked_;
}

bool imports_manager_c::import_context_c::attempt_import(
    const std::string &symbol, const std::string &file_path) {

  if (manager_.imports_locked_) {
    manager_.logger_->error("Import attempted after imports were locked");
    return false;
  }

  auto resolved_path = manager_.resolve_file_path(file_path);
  if (resolved_path.empty()) {
    manager_.logger_->error("Could not resolve import file: {}", file_path);
    return false;
  }

  auto canonical_path = std::filesystem::canonical(resolved_path).string();

  if (manager_.imported_files_.count(canonical_path)) {
    manager_.logger_->debug("File already imported: {}", canonical_path);
    return true;
  }

  if (manager_.currently_importing_.count(canonical_path)) {
    std::string error_msg = "Circular import detected:\n";
    for (const auto &import_file : manager_.import_stack_) {
      error_msg += "  " + import_file + " imports\n";
    }
    error_msg += "  " + canonical_path + " (cycle detected)";
    manager_.logger_->error("{}", error_msg);
    throw std::runtime_error(error_msg);
  }

  manager_.logger_->info("Importing file: {} as symbol: {}", canonical_path,
                         symbol);

  import_guard_c guard(manager_, canonical_path);

  std::ifstream file(canonical_path);
  if (!file.is_open()) {
    manager_.logger_->error("Failed to open import file: {}", canonical_path);
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  file.close();

  auto parse_result = slp::parse(source);
  if (parse_result.is_error()) {
    const auto &error = parse_result.error();
    manager_.logger_->error("Parse error in import {}: {}", canonical_path,
                            error.message);
    return false;
  }

  auto import_symbols = instructions::get_standard_callable_symbols();
  auto import_interpreter = create_interpreter(
      import_symbols, this, manager_.parent_context_->get_kernel_context());

  manager_.current_exports_.clear();

  try {
    auto obj = parse_result.take();
    import_interpreter->eval(obj);
  } catch (const std::exception &e) {
    manager_.logger_->error("Error executing import {}: {}", canonical_path,
                            e.what());
    return false;
  }

  if (!manager_.parent_context_) {
    manager_.logger_->error("No parent context set for importing symbols");
    return false;
  }

  for (const auto &[export_name, export_value] : manager_.current_exports_) {
    std::string prefixed_name = symbol + "/" + export_name;
    auto value_copy = slp::slp_object_c::from_data(
        export_value.get_data(), export_value.get_symbols(),
        export_value.get_root_offset());

    if (export_value.type() == slp::slp_type_e::ABERRANT) {
      const std::uint8_t *base_ptr = export_value.get_data().data();
      const std::uint8_t *unit_ptr = base_ptr + export_value.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      std::uint64_t lambda_id = unit->data.uint64;

      if (!manager_.parent_context_->copy_lambda_from(import_interpreter.get(),
                                                      lambda_id)) {
        manager_.logger_->error("Failed to copy lambda definition for: {}",
                                prefixed_name);
        continue;
      }
      manager_.logger_->debug("Copied lambda {} for exported symbol: {}",
                              lambda_id, prefixed_name);
    }

    if (!manager_.parent_context_->define_symbol(prefixed_name, value_copy)) {
      manager_.logger_->error("Failed to define exported symbol: {}",
                              prefixed_name);
    } else {
      manager_.logger_->debug("Exported symbol: {}", prefixed_name);
    }
  }

  manager_.imported_files_.insert(canonical_path);
  manager_.current_exports_.clear();

  return true;
}

bool imports_manager_c::import_context_c::register_export(
    const std::string &name, slp::slp_object_c &value) {

  manager_.logger_->debug("Registering export: {}", name);
  manager_.current_exports_[name] = slp::slp_object_c::from_data(
      value.get_data(), value.get_symbols(), value.get_root_offset());
  return true;
}

void imports_manager_c::import_context_c::lock() { manager_.lock_imports(); }

} // namespace pkg::core::imports
