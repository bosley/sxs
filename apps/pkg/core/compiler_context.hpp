#pragma once

#include <map>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include <sxs/slp/slp.hpp>
#include <vector>

namespace pkg::core {

struct callable_symbol_s;

namespace imports {
class import_context_if;
}

namespace kernels {
class kernel_context_if;
}

typedef std::shared_ptr<spdlog::logger> logger_t;

struct type_info_s {
  slp::slp_type_e base_type;
  std::string lambda_signature;
  bool is_variadic{false};
  std::uint64_t lambda_id{0};
};

struct function_signature_s {
  std::vector<type_info_s> parameters;
  type_info_s return_type;
  bool variadic{false};
};

class compiler_context_if {
public:
  virtual ~compiler_context_if() = default;

  virtual type_info_s eval_type(slp::slp_object_c &object) = 0;

  virtual bool has_symbol(const std::string &symbol,
                          bool local_scope_only = false) = 0;

  virtual void define_symbol(const std::string &symbol,
                             const type_info_s &type) = 0;

  virtual type_info_s get_symbol_type(const std::string &symbol) = 0;

  virtual bool is_type_symbol(const std::string &symbol,
                              type_info_s &out_type) = 0;

  virtual bool push_scope() = 0;
  virtual bool pop_scope() = 0;

  virtual std::uint64_t allocate_lambda_id() = 0;

  virtual bool register_lambda(std::uint64_t id,
                               const function_signature_s &sig) = 0;

  virtual function_signature_s get_lambda_signature(std::uint64_t id) = 0;

  virtual bool has_function_signature(const std::string &name) = 0;

  virtual function_signature_s
  get_function_signature(const std::string &name) = 0;

  virtual void define_function_signature(const std::string &name,
                                         const function_signature_s &sig) = 0;

  virtual void push_loop_context() = 0;
  virtual void pop_loop_context() = 0;
  virtual bool is_in_loop() = 0;

  virtual imports::import_context_if *get_import_context() = 0;
  virtual kernels::kernel_context_if *get_kernel_context() = 0;

  virtual logger_t get_logger() = 0;
  virtual std::string get_current_file() = 0;
  virtual void set_current_file(const std::string &file) = 0;

  virtual std::vector<std::string> &get_include_paths() = 0;
  virtual std::string &get_working_directory() = 0;

  virtual std::set<std::string> &get_checked_files() = 0;
  virtual std::set<std::string> &get_currently_checking() = 0;
  virtual std::vector<std::string> &get_check_stack() = 0;

  virtual std::map<std::string, type_info_s> &get_current_exports() = 0;

  virtual bool types_match(const type_info_s &expected,
                           const type_info_s &actual) = 0;

  virtual const std::map<std::string, callable_symbol_s> &
  get_callable_symbols() = 0;
};

std::unique_ptr<compiler_context_if> create_compiler_context(
    logger_t logger, std::vector<std::string> include_paths,
    std::string working_directory,
    const std::map<std::string, callable_symbol_s> &callable_symbols,
    imports::import_context_if *import_context = nullptr,
    kernels::kernel_context_if *kernel_context = nullptr);

} // namespace pkg::core
