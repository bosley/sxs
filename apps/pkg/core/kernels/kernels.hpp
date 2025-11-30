#pragma once

#include "core/core.hpp"
#include "core/interpreter.hpp"
#include <map>
#include <memory>
#include <set>
#include <string>

struct sxs_api_table_t;

namespace pkg::core::kernels {

class kernel_context_if {
public:
  virtual ~kernel_context_if() = default;

  virtual bool is_load_allowed() = 0;

  virtual bool attempt_load(const std::string &kernel_name) = 0;

  virtual void lock() = 0;

  virtual bool has_function(const std::string &name) const = 0;

  virtual callable_symbol_s *get_function(const std::string &name) = 0;
};

class kernel_manager_c {
public:
  explicit kernel_manager_c(logger_t logger,
                            std::vector<std::string> include_paths,
                            std::string working_directory);
  ~kernel_manager_c();

  kernel_manager_c(const kernel_manager_c &) = delete;
  kernel_manager_c &operator=(const kernel_manager_c &) = delete;

  kernel_context_if &get_kernel_context();

  void lock_kernels();

  std::map<std::string, callable_symbol_s> get_registered_functions() const;

  void set_parent_context(callable_context_if *context);

  void register_kernel_function(const std::string &kernel_name,
                                const std::string &function_name,
                                void *function_ptr, int return_type,
                                bool variadic);

private:
  std::string resolve_kernel_path(const std::string &kernel_name);

  bool load_kernel_dylib(const std::string &kernel_name,
                         const std::string &kernel_dir);

  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
  bool kernels_locked_;
  std::set<std::string> loaded_kernels_;
  std::map<std::string, void *> loaded_dylibs_;
  std::map<std::string, callable_symbol_s> registered_functions_;
  callable_context_if *parent_context_;
  std::unique_ptr<sxs_api_table_t> api_table_;
  std::map<std::string, void (*)(const struct sxs_api_table_t *)>
      kernel_on_exit_fns_;

  class kernel_context_c : public kernel_context_if {
  public:
    kernel_context_c(kernel_manager_c &manager) : manager_(manager) {}
    ~kernel_context_c() override;

    bool is_load_allowed() override;
    bool attempt_load(const std::string &kernel_name) override;
    void lock() override;
    bool has_function(const std::string &name) const override;
    callable_symbol_s *get_function(const std::string &name) override;

  private:
    kernel_manager_c &manager_;
  };

  std::unique_ptr<kernel_context_c> context_;
};

} // namespace pkg::core::kernels