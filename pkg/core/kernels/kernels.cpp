#include "kernels.hpp"
#include <stdexcept>

namespace pkg::core::kernels {

kernel_manager_c::kernel_manager_c(logger_t logger,
                                   std::vector<std::string> include_paths,
                                   std::string working_directory)
    : logger_(logger), include_paths_(std::move(include_paths)),
      working_directory_(std::move(working_directory)), kernels_locked_(false) {
  context_ = std::make_unique<kernel_context_c>(*this);
}

kernel_context_if &kernel_manager_c::get_kernel_context() { return *context_; }

void kernel_manager_c::lock_kernels() {
  kernels_locked_ = true;
  logger_->debug("Kernels locked - no more kernel loads allowed");
}

kernel_manager_c::kernel_context_c::~kernel_context_c() = default;

bool kernel_manager_c::kernel_context_c::is_load_allowed() {
  return !manager_.kernels_locked_;
}

bool kernel_manager_c::kernel_context_c::attempt_load(
    const std::string &kernel_name) {
  if (manager_.kernels_locked_) {
    manager_.logger_->error("Kernel load attempted after kernels were locked");
    return false;
  }

  manager_.logger_->warn("Kernel loading not yet implemented: {}", kernel_name);
  throw std::runtime_error("Kernel loading not yet implemented");
}

void kernel_manager_c::kernel_context_c::lock() { manager_.lock_kernels(); }

} // namespace pkg::core::kernels
