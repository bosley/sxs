#include "context.hpp"
#include "imports/imports.hpp"
#include "interpreter.hpp"
#include "kernels/kernels.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <sstream>

namespace pkg::core {

class compiler_context_c : public compiler_context_if {
public:
  compiler_context_c(
      logger_t logger, std::vector<std::string> include_paths,
      std::string working_directory,
      const std::map<std::string, callable_symbol_s> &callable_symbols,
      imports::import_context_if *import_context,
      kernels::kernel_context_if *kernel_context)
      : logger_(logger), include_paths_(std::move(include_paths)),
        working_directory_(std::move(working_directory)),
        callable_symbols_(callable_symbols), import_context_(import_context),
        kernel_context_(kernel_context), next_lambda_id_(1), loop_depth_(0) {

    std::vector<std::pair<std::string, slp::slp_type_e>> base_types = {
        {"int", slp::slp_type_e::INTEGER},
        {"real", slp::slp_type_e::REAL},
        {"symbol", slp::slp_type_e::SYMBOL},
        {"str", slp::slp_type_e::DQ_LIST},
        {"list-p", slp::slp_type_e::PAREN_LIST},
        {"list-c", slp::slp_type_e::BRACE_LIST},
        {"list-b", slp::slp_type_e::BRACKET_LIST},
        {"none", slp::slp_type_e::NONE},
        {"some", slp::slp_type_e::SOME},
        {"error", slp::slp_type_e::ERROR},
        {"datum", slp::slp_type_e::DATUM},
        {"aberrant", slp::slp_type_e::ABERRANT},
        {"any", slp::slp_type_e::NONE}};

    for (const auto &[name, type] : base_types) {
      type_info_s type_info;
      type_info.base_type = type;
      type_symbol_map_[":" + name] = type_info;

      type_info_s variadic_type_info;
      variadic_type_info.base_type = type;
      variadic_type_info.is_variadic = true;
      type_symbol_map_[":" + name + ".."] = variadic_type_info;
    }

    type_info_s list_type;
    list_type.base_type = slp::slp_type_e::PAREN_LIST;
    type_symbol_map_[":list"] = list_type;

    type_info_s list_variadic_type;
    list_variadic_type.base_type = slp::slp_type_e::PAREN_LIST;
    list_variadic_type.is_variadic = true;
    type_symbol_map_[":list.."] = list_variadic_type;

    push_scope();
  }

  ~compiler_context_c() override = default;

  type_info_s eval_type(slp::slp_object_c &object) override;

  bool has_symbol(const std::string &symbol,
                  bool local_scope_only = false) override {
    if (local_scope_only) {
      return !scopes_.empty() &&
             scopes_.back().find(symbol) != scopes_.back().end();
    }

    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (it->find(symbol) != it->end()) {
        return true;
      }
    }
    return false;
  }

  void define_symbol(const std::string &symbol,
                     const type_info_s &type) override {
    if (!scopes_.empty()) {
      scopes_.back()[symbol] = type;
    }
  }

  type_info_s get_symbol_type(const std::string &symbol) override {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto found = it->find(symbol);
      if (found != it->end()) {
        return found->second;
      }
    }
    throw std::runtime_error(
        fmt::format("Symbol '{}' not found in any scope", symbol));
  }

  bool is_type_symbol(const std::string &symbol,
                      type_info_s &out_type) override {
    auto it = type_symbol_map_.find(symbol);
    if (it != type_symbol_map_.end()) {
      out_type = it->second;
      return true;
    }
    return false;
  }

  bool push_scope() override {
    scopes_.emplace_back();
    return true;
  }

  bool pop_scope() override {
    if (!scopes_.empty()) {
      scopes_.pop_back();
      return true;
    }
    return false;
  }

  std::uint64_t allocate_lambda_id() override { return next_lambda_id_++; }

  bool register_lambda(std::uint64_t id,
                       const function_signature_s &sig) override {
    lambda_signatures_[id] = sig;
    return true;
  }

  function_signature_s get_lambda_signature(std::uint64_t id) override {
    auto it = lambda_signatures_.find(id);
    if (it != lambda_signatures_.end()) {
      return it->second;
    }
    throw std::runtime_error(
        fmt::format("Lambda signature not found for id {}", id));
  }

  bool has_function_signature(const std::string &name) override {
    return function_signatures_.find(name) != function_signatures_.end();
  }

  function_signature_s
  get_function_signature(const std::string &name) override {
    auto it = function_signatures_.find(name);
    if (it != function_signatures_.end()) {
      return it->second;
    }
    throw std::runtime_error(
        fmt::format("Function signature not found for '{}'", name));
  }

  void define_function_signature(const std::string &name,
                                 const function_signature_s &sig) override {
    function_signatures_[name] = sig;
  }

  void push_loop_context() override { loop_depth_++; }

  void pop_loop_context() override {
    if (loop_depth_ > 0) {
      loop_depth_--;
    }
  }

  bool is_in_loop() override { return loop_depth_ > 0; }

  imports::import_context_if *get_import_context() override {
    return import_context_;
  }

  kernels::kernel_context_if *get_kernel_context() override {
    return kernel_context_;
  }

  logger_t get_logger() override { return logger_; }

  std::string get_current_file() override { return current_file_; }

  void set_current_file(const std::string &file) override {
    current_file_ = file;
  }

  std::vector<std::string> &get_include_paths() override {
    return include_paths_;
  }

  std::string &get_working_directory() override { return working_directory_; }

  std::set<std::string> &get_checked_files() override { return checked_files_; }

  std::set<std::string> &get_currently_checking() override {
    return currently_checking_;
  }

  std::vector<std::string> &get_check_stack() override { return check_stack_; }

  std::map<std::string, type_info_s> &get_current_exports() override {
    return current_exports_;
  }

  bool types_match(const type_info_s &expected,
                   const type_info_s &actual) override {
    if (expected.base_type == slp::slp_type_e::NONE &&
        expected.lambda_signature.empty()) {
      return true;
    }

    if (expected.base_type == slp::slp_type_e::PAREN_LIST &&
        actual.base_type == slp::slp_type_e::PAREN_LIST) {
      return true;
    }

    if (expected.base_type == actual.base_type) {
      return true;
    }

    return false;
  }

  const std::map<std::string, callable_symbol_s> &
  get_callable_symbols() override {
    return callable_symbols_;
  }

  std::string resolve_file_path(const std::string &file_path) override;
  std::string resolve_kernel_path(const std::string &kernel_name) override;
  bool load_kernel_types(const std::string &kernel_name,
                         const std::string &kernel_dir) override;

private:
  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
  const std::map<std::string, callable_symbol_s> &callable_symbols_;
  imports::import_context_if *import_context_;
  kernels::kernel_context_if *kernel_context_;

  std::vector<std::map<std::string, type_info_s>> scopes_;
  std::map<std::string, type_info_s> type_symbol_map_;
  std::map<std::uint64_t, function_signature_s> lambda_signatures_;
  std::map<std::string, function_signature_s> function_signatures_;

  std::uint64_t next_lambda_id_;
  int loop_depth_;

  std::set<std::string> checked_files_;
  std::set<std::string> currently_checking_;
  std::vector<std::string> check_stack_;
  std::map<std::string, type_info_s> current_exports_;
  std::string current_file_;
};

type_info_s compiler_context_c::eval_type(slp::slp_object_c &object) {
  auto type = object.type();

  type_info_s result;

  switch (type) {
  case slp::slp_type_e::INTEGER:
    result.base_type = slp::slp_type_e::INTEGER;
    return result;

  case slp::slp_type_e::REAL:
    result.base_type = slp::slp_type_e::REAL;
    return result;

  case slp::slp_type_e::DQ_LIST:
    result.base_type = slp::slp_type_e::DQ_LIST;
    return result;

  case slp::slp_type_e::RUNE:
    result.base_type = slp::slp_type_e::RUNE;
    return result;

  case slp::slp_type_e::SYMBOL: {
    std::string sym = object.as_symbol();

    if (has_symbol(sym)) {
      return get_symbol_type(sym);
    }

    auto slash_pos = sym.find('/');
    if (slash_pos != std::string::npos) {
      std::string prefix = sym.substr(0, slash_pos);
      std::string suffix = sym.substr(slash_pos + 1);

      std::string full_sym = prefix + "/" + suffix;
      if (has_symbol(full_sym)) {
        return get_symbol_type(full_sym);
      }
    }

    result.base_type = slp::slp_type_e::SYMBOL;
    return result;
  }

  case slp::slp_type_e::ABERRANT:
    result.base_type = slp::slp_type_e::ABERRANT;
    return result;

  case slp::slp_type_e::SOME: {
    const std::uint8_t *base_ptr = object.get_data().data();
    const std::uint8_t *unit_ptr = base_ptr + object.get_root_offset();
    const slp::slp_unit_of_store_t *unit =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
    size_t inner_offset = static_cast<size_t>(unit->data.uint64);

    auto inner_obj = slp::slp_object_c::from_data(
        object.get_data(), object.get_symbols(), inner_offset);

    result.base_type = inner_obj.type();
    return result;
  }

  case slp::slp_type_e::PAREN_LIST: {
    auto list = object.as_list();
    if (list.empty()) {
      result.base_type = slp::slp_type_e::PAREN_LIST;
      return result;
    }

    auto first = list.at(0);
    if (first.type() != slp::slp_type_e::SYMBOL) {
      throw std::runtime_error(fmt::format("Cannot call non-symbol type: {}",
                                           static_cast<int>(first.type())));
    }

    std::string cmd = first.as_symbol();

    if (callable_symbols_.count(cmd)) {
      const auto &symbol = callable_symbols_.at(cmd);
      if (symbol.typecheck_function) {
        return symbol.typecheck_function(*this, object);
      }
      throw std::runtime_error(
          fmt::format("Builtin '{}' has no typecheck function", cmd));
    }

    if (has_symbol(cmd)) {
      auto sym_type = get_symbol_type(cmd);
      if (sym_type.base_type == slp::slp_type_e::ABERRANT) {
        if (sym_type.lambda_id == 0 ||
            lambda_signatures_.find(sym_type.lambda_id) ==
                lambda_signatures_.end()) {
          throw std::runtime_error(
              fmt::format("Lambda {} has no signature information", cmd));
        }

        const auto &sig = lambda_signatures_[sym_type.lambda_id];

        if (!sig.variadic && list.size() - 1 != sig.parameters.size()) {
          throw std::runtime_error(
              fmt::format("Lambda {} expects {} arguments, got {}", cmd,
                          sig.parameters.size(), list.size() - 1));
        }

        size_t fixed_param_count = sig.parameters.size();
        if (sig.variadic && fixed_param_count > 0) {
          fixed_param_count--;
        }

        if (list.size() - 1 < fixed_param_count) {
          throw std::runtime_error(
              fmt::format("Lambda {} expects at least {} arguments, got {}",
                          cmd, fixed_param_count, list.size() - 1));
        }

        for (size_t i = 0; i < fixed_param_count; i++) {
          auto arg = list.at(i + 1);
          auto arg_type = eval_type(arg);
          if (!types_match(sig.parameters[i], arg_type)) {
            throw std::runtime_error(fmt::format(
                "Lambda {} argument {} type mismatch: expected {}, got {}", cmd,
                i + 1, static_cast<int>(sig.parameters[i].base_type),
                static_cast<int>(arg_type.base_type)));
          }
        }

        if (sig.variadic && sig.parameters.size() > 0) {
          const auto &variadic_param = sig.parameters.back();
          for (size_t i = fixed_param_count; i < list.size() - 1; i++) {
            auto arg = list.at(i + 1);
            auto arg_type = eval_type(arg);
            if (!types_match(variadic_param, arg_type)) {
              throw std::runtime_error(fmt::format(
                  "Lambda {} variadic argument {} type mismatch: expected {}, "
                  "got {}",
                  cmd, i + 1, static_cast<int>(variadic_param.base_type),
                  static_cast<int>(arg_type.base_type)));
            }
          }
        }

        return sig.return_type;
      }
    }

    auto slash_pos = cmd.find('/');
    if (slash_pos != std::string::npos) {
      if (function_signatures_.count(cmd)) {
        const auto &sig = function_signatures_[cmd];

        if (!sig.variadic && list.size() - 1 != sig.parameters.size()) {
          throw std::runtime_error(
              fmt::format("Function {} expects {} arguments, got {}", cmd,
                          sig.parameters.size(), list.size() - 1));
        }

        size_t fixed_param_count = sig.parameters.size();
        if (sig.variadic && fixed_param_count > 0) {
          fixed_param_count--;
        }

        if (list.size() - 1 < fixed_param_count) {
          throw std::runtime_error(
              fmt::format("Function {} expects at least {} arguments, got {}",
                          cmd, fixed_param_count, list.size() - 1));
        }

        for (size_t i = 0; i < fixed_param_count; i++) {
          auto arg = list.at(i + 1);
          auto arg_type = eval_type(arg);
          if (!types_match(sig.parameters[i], arg_type)) {
            throw std::runtime_error(fmt::format(
                "Function {} argument {} type mismatch: expected "
                "{}, got {}",
                cmd, i + 1, static_cast<int>(sig.parameters[i].base_type),
                static_cast<int>(arg_type.base_type)));
          }
        }

        if (sig.variadic && sig.parameters.size() > 0) {
          const auto &variadic_param = sig.parameters.back();
          for (size_t i = fixed_param_count; i < list.size() - 1; i++) {
            auto arg = list.at(i + 1);
            auto arg_type = eval_type(arg);
            if (!types_match(variadic_param, arg_type)) {
              throw std::runtime_error(fmt::format(
                  "Function {} variadic argument {} type mismatch: expected "
                  "{}, got {}",
                  cmd, i + 1, static_cast<int>(variadic_param.base_type),
                  static_cast<int>(arg_type.base_type)));
            }
          }
        }

        return sig.return_type;
      }
    }

    throw std::runtime_error(fmt::format("Unknown callable symbol: {}", cmd));
  }

  case slp::slp_type_e::DATUM: {
    const std::uint8_t *base_ptr = object.get_data().data();
    const std::uint8_t *unit_ptr = base_ptr + object.get_root_offset();
    const slp::slp_unit_of_store_t *unit =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
    size_t inner_offset = static_cast<size_t>(unit->data.uint64);

    auto inner_obj = slp::slp_object_c::from_data(
        object.get_data(), object.get_symbols(), inner_offset);

    if (inner_obj.type() != slp::slp_type_e::PAREN_LIST) {
      result.base_type = slp::slp_type_e::DATUM;
      return result;
    }

    auto list = inner_obj.as_list();
    if (list.empty()) {
      result.base_type = slp::slp_type_e::DATUM;
      return result;
    }

    auto first = list.at(0);
    if (first.type() != slp::slp_type_e::SYMBOL) {
      result.base_type = slp::slp_type_e::DATUM;
      return result;
    }

    std::string cmd = first.as_symbol();

    if (callable_symbols_.count(cmd)) {
      const auto &symbol = callable_symbols_.at(cmd);
      if (symbol.typecheck_function) {
        return symbol.typecheck_function(*this, inner_obj);
      }
    }

    result.base_type = slp::slp_type_e::DATUM;
    return result;
  }

  case slp::slp_type_e::BRACKET_LIST: {
    auto list = object.as_list();
    type_info_s last_result;
    last_result.base_type = slp::slp_type_e::NONE;

    for (size_t i = 0; i < list.size(); i++) {
      auto elem = list.at(i);
      last_result = eval_type(elem);
    }
    return last_result;
  }

  default:
    result.base_type = type;
    return result;
  }
}

std::string
compiler_context_c::resolve_file_path(const std::string &file_path) {
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

std::string
compiler_context_c::resolve_kernel_path(const std::string &kernel_name) {
  if (std::filesystem::path(kernel_name).is_absolute()) {
    if (std::filesystem::exists(kernel_name)) {
      return kernel_name;
    }
  }

  for (const auto &include_path : include_paths_) {
    auto kernel_path = std::filesystem::path(include_path) / kernel_name;
    if (std::filesystem::exists(kernel_path / "kernel.sxs")) {
      return kernel_path.string();
    }
  }

  auto working_kernel_path =
      std::filesystem::path(working_directory_) / kernel_name;
  if (std::filesystem::exists(working_kernel_path / "kernel.sxs")) {
    return working_kernel_path.string();
  }

  return "";
}

bool compiler_context_c::load_kernel_types(const std::string &kernel_name,
                                           const std::string &kernel_dir) {
  auto kernel_sxs_path = std::filesystem::path(kernel_dir) / "kernel.sxs";

  std::ifstream file(kernel_sxs_path);
  if (!file.is_open()) {
    logger_->error("Could not open kernel.sxs: {}", kernel_sxs_path.string());
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  file.close();

  auto parse_result = slp::parse(source);
  if (parse_result.is_error()) {
    logger_->error("Failed to parse kernel.sxs: {}",
                   parse_result.error().message);
    return false;
  }

  auto kernel_obj = parse_result.take();
  if (kernel_obj.type() != slp::slp_type_e::DATUM) {
    logger_->error("kernel.sxs must start with #(define-kernel ...)");
    return false;
  }

  const std::uint8_t *base_ptr = kernel_obj.get_data().data();
  const std::uint8_t *unit_ptr = base_ptr + kernel_obj.get_root_offset();
  const slp::slp_unit_of_store_t *unit =
      reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
  size_t inner_offset = static_cast<size_t>(unit->data.uint64);

  auto inner_obj = slp::slp_object_c::from_data(
      kernel_obj.get_data(), kernel_obj.get_symbols(), inner_offset);

  if (inner_obj.type() != slp::slp_type_e::PAREN_LIST) {
    logger_->error("kernel.sxs define-kernel must be a list");
    return false;
  }

  auto list = inner_obj.as_list();
  if (list.size() < 4) {
    logger_->error("kernel.sxs define-kernel requires: name dylib functions");
    return false;
  }

  auto functions_obj = list.at(3);
  if (functions_obj.type() != slp::slp_type_e::BRACKET_LIST) {
    logger_->error("kernel.sxs functions must be a bracket list");
    return false;
  }

  auto functions_list = functions_obj.as_list();
  for (size_t i = 0; i < functions_list.size(); i++) {
    auto func_def = functions_list.at(i);
    if (func_def.type() != slp::slp_type_e::PAREN_LIST) {
      logger_->warn("kernel.sxs: skipping non-list function definition");
      continue;
    }

    auto func_list = func_def.as_list();
    if (func_list.size() < 4) {
      logger_->warn("kernel.sxs: function definition requires at least 4 "
                    "elements");
      continue;
    }

    auto cmd = func_list.at(0);
    if (cmd.type() != slp::slp_type_e::SYMBOL ||
        cmd.as_symbol() != std::string("define-function")) {
      continue;
    }

    auto func_name_obj = func_list.at(1);
    if (func_name_obj.type() != slp::slp_type_e::SYMBOL) {
      logger_->warn("kernel.sxs: function name must be a symbol");
      continue;
    }
    std::string func_name = func_name_obj.as_symbol();

    auto params_obj = func_list.at(2);
    if (params_obj.type() != slp::slp_type_e::PAREN_LIST) {
      logger_->warn("kernel.sxs: function parameters must be a list");
      continue;
    }

    auto return_type_obj = func_list.at(3);
    if (return_type_obj.type() != slp::slp_type_e::SYMBOL) {
      logger_->warn("kernel.sxs: function return type must be a symbol");
      continue;
    }

    std::string return_type_sym = return_type_obj.as_symbol();
    type_info_s return_type;
    if (!is_type_symbol(return_type_sym, return_type)) {
      logger_->error("kernel.sxs: invalid return type: {}", return_type_sym);
      continue;
    }

    auto params_list = params_obj.as_list();
    std::vector<type_info_s> parameters;
    bool variadic = false;

    for (size_t j = 0; j < params_list.size(); j += 2) {
      if (j + 1 >= params_list.size()) {
        logger_->warn("kernel.sxs: parameters must be in pairs");
        break;
      }

      auto param_type_obj = params_list.at(j + 1);
      if (param_type_obj.type() != slp::slp_type_e::SYMBOL) {
        logger_->warn("kernel.sxs: parameter type must be a symbol");
        continue;
      }

      std::string param_type_sym = param_type_obj.as_symbol();
      type_info_s param_type;

      if (!is_type_symbol(param_type_sym, param_type)) {
        logger_->error("kernel.sxs: invalid parameter type: {}",
                       param_type_sym);
        continue;
      }

      if (param_type.is_variadic) {
        variadic = true;
      }

      parameters.push_back(param_type);
    }

    function_signature_s sig;
    sig.parameters = parameters;
    sig.return_type = return_type;
    sig.variadic = variadic;

    std::string full_func_name = kernel_name + "/" + func_name;
    function_signatures_[full_func_name] = sig;

    logger_->debug("Registered kernel function: {}", full_func_name);
  }

  return true;
}

std::unique_ptr<compiler_context_if> create_compiler_context(
    logger_t logger, std::vector<std::string> include_paths,
    std::string working_directory,
    const std::map<std::string, callable_symbol_s> &callable_symbols,
    imports::import_context_if *import_context,
    kernels::kernel_context_if *kernel_context) {
  return std::make_unique<compiler_context_c>(
      logger, std::move(include_paths), std::move(working_directory),
      callable_symbols, import_context, kernel_context);
}

} // namespace pkg::core
