#include "kernels.hpp"
#include "core/interpreter.hpp"
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sxs/kernel_api.hpp>
#include <sxs/slp/slp.hpp>

namespace pkg::core::kernels {

namespace {

struct registration_context_s {
  kernel_manager_c *manager;
  std::string kernel_name;
  const pkg::kernel::api_table_s *api;
};

struct kernel_definition_context_s {
  kernel_manager_c *manager;
  std::string kernel_name;
  std::string kernel_dir;
  std::set<std::string> declared_functions;
  std::string dylib_name;
};

void register_function_callback(pkg::kernel::registry_t registry,
                                const char *name,
                                pkg::kernel::kernel_fn_t function,
                                slp::slp_type_e return_type, int variadic) {
  auto *ctx = static_cast<registration_context_s *>(registry);
  ctx->manager->register_kernel_function(
      ctx->kernel_name, name, reinterpret_cast<void *>(function),
      static_cast<int>(return_type), variadic != 0);
}

slp::slp_object_c eval_callback(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &obj) {
  auto *context = static_cast<callable_context_if *>(ctx);
  return context->eval(const_cast<slp::slp_object_c &>(obj));
}

slp::slp_object_c create_none_callback() {
  auto parse_result = slp::parse("()");
  return parse_result.take();
}

slp::slp_object_c create_int_callback(long long value) {
  auto parse_result = slp::parse(std::to_string(value));
  return parse_result.take();
}

slp::slp_object_c create_real_callback(double value) {
  auto parse_result = slp::parse(std::to_string(value));
  return parse_result.take();
}

slp::slp_object_c create_string_callback(const char *value) {
  if (!value) {
    return create_none_callback();
  }
  return slp::create_string_direct(std::string(value));
}

slp::slp_object_c create_symbol_callback(const char *name) {
  if (!name) {
    return create_none_callback();
  }
  auto parse_result = slp::parse(std::string(name));
  return parse_result.take();
}

slp::slp_object_c create_paren_list_callback(const slp::slp_object_c *objects,
                                             size_t count) {
  if (!objects || count == 0) {
    auto parse_result = slp::parse("()");
    return parse_result.take();
  }

  std::string list_str = "(";
  for (size_t i = 0; i < count; i++) {
    const auto &elem = objects[i];
    if (elem.type() == slp::slp_type_e::INTEGER) {
      list_str += std::to_string(elem.as_int());
    } else if (elem.type() == slp::slp_type_e::REAL) {
      list_str += std::to_string(elem.as_real());
    } else if (elem.type() == slp::slp_type_e::DQ_LIST) {
      list_str += "\"" + elem.as_string().to_string() + "\"";
    } else if (elem.type() == slp::slp_type_e::SYMBOL) {
      list_str += elem.as_symbol();
    } else {
      list_str += "()";
    }
    if (i < count - 1) {
      list_str += " ";
    }
  }
  list_str += ")";

  auto parse_result = slp::parse(list_str);
  return parse_result.take();
}

slp::slp_object_c create_bracket_list_callback(const slp::slp_object_c *objects,
                                               size_t count) {
  if (!objects || count == 0) {
    auto parse_result = slp::parse("[]");
    return parse_result.take();
  }

  std::string list_str = "[";
  for (size_t i = 0; i < count; i++) {
    const auto &elem = objects[i];
    if (elem.type() == slp::slp_type_e::INTEGER) {
      list_str += std::to_string(elem.as_int());
    } else if (elem.type() == slp::slp_type_e::REAL) {
      list_str += std::to_string(elem.as_real());
    } else if (elem.type() == slp::slp_type_e::DQ_LIST) {
      list_str += "\"" + elem.as_string().to_string() + "\"";
    } else if (elem.type() == slp::slp_type_e::SYMBOL) {
      list_str += elem.as_symbol();
    } else {
      list_str += "()";
    }
    if (i < count - 1) {
      list_str += " ";
    }
  }
  list_str += "]";

  auto parse_result = slp::parse(list_str);
  return parse_result.take();
}

slp::slp_object_c create_brace_list_callback(const slp::slp_object_c *objects,
                                             size_t count) {
  if (!objects || count == 0) {
    auto parse_result = slp::parse("{}");
    return parse_result.take();
  }

  std::string list_str = "{";
  for (size_t i = 0; i < count; i++) {
    const auto &elem = objects[i];
    if (elem.type() == slp::slp_type_e::INTEGER) {
      list_str += std::to_string(elem.as_int());
    } else if (elem.type() == slp::slp_type_e::REAL) {
      list_str += std::to_string(elem.as_real());
    } else if (elem.type() == slp::slp_type_e::DQ_LIST) {
      list_str += "\"" + elem.as_string().to_string() + "\"";
    } else if (elem.type() == slp::slp_type_e::SYMBOL) {
      list_str += elem.as_symbol();
    } else {
      list_str += "()";
    }
    if (i < count - 1) {
      list_str += " ";
    }
  }
  list_str += "}";

  auto parse_result = slp::parse(list_str);
  return parse_result.take();
}

std::map<std::string, callable_symbol_s>
get_kernel_definition_symbols(kernel_definition_context_s *ctx) {
  std::map<std::string, callable_symbol_s> symbols;

  symbols["define-function"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {},
      .variadic = false,
      .function = [ctx](callable_context_if &context,
                        slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() < 4) {
          throw std::runtime_error(
              "define-function requires at least 3 arguments: name (params) "
              ":return-type");
        }

        auto func_name_obj = list.at(1);
        if (func_name_obj.type() != slp::slp_type_e::SYMBOL) {
          throw std::runtime_error("define-function: name must be a symbol");
        }
        std::string func_name = func_name_obj.as_symbol();

        auto params_obj = list.at(2);
        if (params_obj.type() != slp::slp_type_e::PAREN_LIST) {
          throw std::runtime_error(
              "define-function: parameters must be a paren list");
        }

        auto return_type_obj = list.at(3);
        if (return_type_obj.type() != slp::slp_type_e::SYMBOL) {
          throw std::runtime_error(
              "define-function: return type must be a symbol");
        }

        std::string return_type_sym = return_type_obj.as_symbol();
        slp::slp_type_e return_type;
        if (!context.is_symbol_enscribing_valid_type(return_type_sym,
                                                     return_type)) {
          throw std::runtime_error(fmt::format(
              "define-function: invalid return type: {}", return_type_sym));
        }

        auto params_list = params_obj.as_list();
        for (size_t j = 0; j < params_list.size(); j += 2) {
          if (j + 1 >= params_list.size()) {
            throw std::runtime_error(
                "define-function: parameters must be in pairs (name :type)");
          }

          auto param_type_obj = params_list.at(j + 1);
          if (param_type_obj.type() != slp::slp_type_e::SYMBOL) {
            throw std::runtime_error(
                "define-function: parameter type must be a symbol");
          }

          std::string param_type_sym = param_type_obj.as_symbol();
          slp::slp_type_e param_type;
          if (!context.is_symbol_enscribing_valid_type(param_type_sym,
                                                       param_type)) {
            throw std::runtime_error(fmt::format(
                "define-function: invalid parameter type: {}", param_type_sym));
          }
        }

        ctx->declared_functions.insert(func_name);

        slp::slp_object_c result;
        return result;
      }};

  symbols["define-kernel"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .required_parameters = {},
      .variadic = false,
      .function = [ctx](callable_context_if &context,
                        slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() < 4) {
          throw std::runtime_error(
              "define-kernel requires 3 arguments: name dylib [functions]");
        }

        auto dylib_name_obj = list.at(2);
        if (dylib_name_obj.type() != slp::slp_type_e::DQ_LIST) {
          throw std::runtime_error(
              "define-kernel: dylib name must be a string");
        }

        ctx->dylib_name = dylib_name_obj.as_string().to_string();

        auto functions_obj = list.at(3);
        if (functions_obj.type() != slp::slp_type_e::BRACKET_LIST) {
          throw std::runtime_error(
              "define-kernel: functions must be a bracket list");
        }

        auto result = context.eval(functions_obj);

        return result;
      }};

  return symbols;
}

} // namespace

kernel_manager_c::kernel_manager_c(logger_t logger,
                                   std::vector<std::string> include_paths,
                                   std::string working_directory)
    : logger_(logger), include_paths_(std::move(include_paths)),
      working_directory_(std::move(working_directory)), kernels_locked_(false),
      parent_context_(nullptr),
      api_table_(std::make_unique<pkg::kernel::api_table_s>()) {
  context_ = std::make_unique<kernel_context_c>(*this);

  api_table_->register_function = register_function_callback;
  api_table_->eval = eval_callback;
  api_table_->create_int = create_int_callback;
  api_table_->create_real = create_real_callback;
  api_table_->create_string = create_string_callback;
  api_table_->create_none = create_none_callback;
  api_table_->create_symbol = create_symbol_callback;
  api_table_->create_paren_list = create_paren_list_callback;
  api_table_->create_bracket_list = create_bracket_list_callback;
  api_table_->create_brace_list = create_brace_list_callback;
}

kernel_manager_c::~kernel_manager_c() {
  for (auto &[name, shutdown_fn] : kernel_on_exit_fns_) {
    logger_->debug("Calling kernel_shutdown for: {}", name);
    shutdown_fn(api_table_.get());
  }

  for (auto &[name, handle] : loaded_dylibs_) {
    if (handle) {
      dlclose(handle);
    }
  }
}

kernel_context_if &kernel_manager_c::get_kernel_context() { return *context_; }

void kernel_manager_c::lock_kernels() {
  kernels_locked_ = true;
  logger_->debug("Kernels locked - no more kernel loads allowed");
}

std::map<std::string, callable_symbol_s>
kernel_manager_c::get_registered_functions() const {
  return registered_functions_;
}

void kernel_manager_c::set_parent_context(callable_context_if *context) {
  parent_context_ = context;
}

std::string
kernel_manager_c::resolve_kernel_path(const std::string &kernel_name) {
  std::filesystem::path kernel_file = "kernel.sxs";

  if (std::filesystem::path(kernel_name).is_absolute()) {
    auto full_path = std::filesystem::path(kernel_name) / kernel_file;
    if (std::filesystem::exists(full_path)) {
      return std::filesystem::path(kernel_name).string();
    }
  }

  for (const auto &include_path : include_paths_) {
    auto full_path =
        std::filesystem::path(include_path) / kernel_name / kernel_file;
    if (std::filesystem::exists(full_path)) {
      return (std::filesystem::path(include_path) / kernel_name).string();
    }
  }

  auto working_path =
      std::filesystem::path(working_directory_) / kernel_name / kernel_file;
  if (std::filesystem::exists(working_path)) {
    return (std::filesystem::path(working_directory_) / kernel_name).string();
  }

  return "";
}

bool kernel_manager_c::load_kernel_dylib(const std::string &kernel_name,
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

  kernel_definition_context_s def_ctx;
  def_ctx.manager = this;
  def_ctx.kernel_name = kernel_name;
  def_ctx.kernel_dir = kernel_dir;

  auto def_symbols = get_kernel_definition_symbols(&def_ctx);
  auto def_interpreter = create_interpreter(def_symbols, nullptr, nullptr);

  try {
    def_interpreter->eval(kernel_obj);
  } catch (const std::exception &e) {
    logger_->error("Error processing kernel.sxs: {}", e.what());
    return false;
  }

  if (def_ctx.dylib_name.empty()) {
    logger_->error("kernel.sxs did not specify dylib name");
    return false;
  }

  auto dylib_path = std::filesystem::path(kernel_dir) / def_ctx.dylib_name;
  if (!std::filesystem::exists(dylib_path)) {
    logger_->error("Kernel dylib not found: {}", dylib_path.string());
    return false;
  }

  logger_->info("Loading kernel dylib: {}", dylib_path.string());

  void *handle = dlopen(dylib_path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    logger_->error("Failed to load kernel dylib: {}", dlerror());
    return false;
  }

  typedef void (*kernel_init_fn_t)(pkg::kernel::registry_t,
                                   const pkg::kernel::api_table_s *);
  auto kernel_init =
      reinterpret_cast<kernel_init_fn_t>(dlsym(handle, "kernel_init"));
  if (!kernel_init) {
    logger_->error("Failed to find kernel_init in dylib: {}", dlerror());
    dlclose(handle);
    return false;
  }

  registration_context_s reg_ctx = {
      .manager = this, .kernel_name = kernel_name, .api = api_table_.get()};

  kernel_init(&reg_ctx, api_table_.get());

  for (const auto &func_name : def_ctx.declared_functions) {
    std::string full_name = kernel_name + "/" + func_name;
    if (registered_functions_.find(full_name) == registered_functions_.end()) {
      logger_->error(
          "kernel.sxs declares function '{}' but dylib did not register it",
          func_name);
      dlclose(handle);
      return false;
    }
  }

  logger_->info("All declared functions successfully registered");

  typedef void (*shutdown_fn_t)(const pkg::kernel::api_table_s *);
  auto kernel_shutdown_fn =
      reinterpret_cast<shutdown_fn_t>(dlsym(handle, "kernel_shutdown"));
  if (kernel_shutdown_fn) {
    logger_->debug("Registered kernel_shutdown for: {}", kernel_name);
    kernel_on_exit_fns_[kernel_name] = kernel_shutdown_fn;
  }

  loaded_dylibs_[kernel_name] = handle;
  logger_->info("Successfully loaded kernel: {}", kernel_name);

  return true;
}

void kernel_manager_c::register_kernel_function(
    const std::string &kernel_name, const std::string &function_name,
    void *function_ptr, int return_type, bool variadic) {

  std::string full_name = kernel_name + "/" + function_name;
  logger_->debug("Registering kernel function: {}", full_name);

  auto kernel_fn = reinterpret_cast<pkg::kernel::kernel_fn_t>(function_ptr);

  callable_symbol_s symbol;
  symbol.return_type = static_cast<slp::slp_type_e>(return_type);
  symbol.variadic = variadic;
  symbol.function =
      [kernel_fn](callable_context_if &context,
                  slp::slp_object_c &args_list) -> slp::slp_object_c {
    return kernel_fn(static_cast<pkg::kernel::context_t>(&context), args_list);
  };

  registered_functions_[full_name] = std::move(symbol);
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

  if (manager_.loaded_kernels_.count(kernel_name)) {
    manager_.logger_->debug("Kernel already loaded: {}", kernel_name);
    return true;
  }

  auto kernel_dir = manager_.resolve_kernel_path(kernel_name);
  if (kernel_dir.empty()) {
    manager_.logger_->error("Could not resolve kernel: {}", kernel_name);
    return false;
  }

  manager_.logger_->info("Loading kernel: {} from {}", kernel_name, kernel_dir);

  if (!manager_.load_kernel_dylib(kernel_name, kernel_dir)) {
    return false;
  }

  manager_.loaded_kernels_.insert(kernel_name);
  return true;
}

void kernel_manager_c::kernel_context_c::lock() { manager_.lock_kernels(); }

bool kernel_manager_c::kernel_context_c::has_function(
    const std::string &name) const {
  return manager_.registered_functions_.find(name) !=
         manager_.registered_functions_.end();
}

callable_symbol_s *
kernel_manager_c::kernel_context_c::get_function(const std::string &name) {
  auto it = manager_.registered_functions_.find(name);
  if (it != manager_.registered_functions_.end()) {
    return &it->second;
  }
  return nullptr;
}

} // namespace pkg::core::kernels
