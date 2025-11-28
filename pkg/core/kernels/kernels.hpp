#pragma once

#include "core/core.hpp"

namespace pkg::core::kernels {

class kernel_context_if {
public:
  virtual ~kernel_context_if() = default;

  virtual bool is_load_allowed() = 0;

  virtual bool attempt_load(const std::string &kernel_name) = 0;

  virtual void lock() = 0;
};

class kernel_manager_c {
public:
  explicit kernel_manager_c(logger_t logger,
                            std::vector<std::string> include_paths,
                            std::string working_directory);
  ~kernel_manager_c() = default;

  kernel_manager_c(const kernel_manager_c &) = delete;
  kernel_manager_c &operator=(const kernel_manager_c &) = delete;

  kernel_context_if &get_kernel_context();

  void lock_kernels();

private:
  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
  bool kernels_locked_;

  class kernel_context_c : public kernel_context_if {
  public:
    kernel_context_c(kernel_manager_c &manager) : manager_(manager) {}
    ~kernel_context_c() override;

    bool is_load_allowed() override;
    bool attempt_load(const std::string &kernel_name) override;
    void lock() override;

  private:
    kernel_manager_c &manager_;
  };

  std::unique_ptr<kernel_context_c> context_;
};

} // namespace pkg::core::kernels