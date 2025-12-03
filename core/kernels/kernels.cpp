#include "kernels.hpp"
#include "core/interpreter.hpp"
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <kernel_api.hpp>
#include <slp/slp.hpp>
#include <sstream>

/*
UPGRADE:DYNAMIC_INJECTED_SYMBOLS

in libs/std/forge im ideating how to dynamically permit the declaration of
injected symbols into the runtime. this would mean
  - update core/kernels (this) to accept new command during kernel ingestion to
map symbol and type information along with what "function" if falls under
  - update tcs to have configurable function handling so in-addition to the
builtins we can handle injection in custom scenarios as well (see tcs sections
where we inject "$exception" and "$error")

  bosley - 11/30/25

  See also pkg/tcs/tcs.cpp
*/

namespace pkg::core::kernels {

namespace {

struct registration_context_s {
  kernel_manager_c *manager;
  std::string kernel_name;
  const pkg::kernel::api_table_s *api;
};

struct system_context_s {
  pkg::kernel::system_info_s info;
  std::string working_directory_storage;
};

static system_context_s g_system_context;

struct kernel_definition_context_s {
  kernel_manager_c *manager;
  std::string kernel_name;
  std::string kernel_dir;
  std::set<std::string> declared_functions;
  std::map<std::string, std::vector<slp::slp_type_e>> declared_forms;
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

const pkg::kernel::system_info_s *
get_system_info_callback(pkg::kernel::system_t sys) {
  auto *system_ctx = static_cast<system_context_s *>(sys);
  return &system_ctx->info;
}

std::map<std::string, callable_symbol_s>
get_kernel_definition_symbols(kernel_definition_context_s *ctx) {
  std::map<std::string, callable_symbol_s> symbols;

  symbols["define-function"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
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

  symbols["define-form"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
      .variadic = false,
      .function = [ctx](callable_context_if &context,
                        slp::slp_object_c &args_list) -> slp::slp_object_c {
        auto list = args_list.as_list();
        if (list.size() != 3) {
          throw std::runtime_error(
              "define-form requires exactly 2 arguments: name and elements");
        }

        auto form_name_obj = list.at(1);
        if (form_name_obj.type() != slp::slp_type_e::SYMBOL) {
          throw std::runtime_error(
              "define-form: first argument must be a symbol (form name)");
        }
        std::string form_name = form_name_obj.as_symbol();

        auto elements_obj = list.at(2);
        if (elements_obj.type() != slp::slp_type_e::BRACE_LIST) {
          throw std::runtime_error("define-form: second argument must be a "
                                   "brace list of type symbols");
        }

        auto elements_list = elements_obj.as_list();
        std::vector<slp::slp_type_e> element_types;

        for (size_t i = 0; i < elements_list.size(); i++) {
          auto elem = elements_list.at(i);
          if (elem.type() != slp::slp_type_e::SYMBOL) {
            throw std::runtime_error(
                "define-form: all elements must be type symbols");
          }

          std::string type_symbol = elem.as_symbol();
          slp::slp_type_e elem_type;

          if (!context.is_symbol_enscribing_valid_type(type_symbol,
                                                       elem_type)) {
            throw std::runtime_error(fmt::format(
                "define-form: invalid type symbol: {}", type_symbol));
          }

          element_types.push_back(elem_type);
        }

        ctx->declared_forms[form_name] = element_types;

        slp::slp_object_c result;
        return result;
      }};

  symbols["define-kernel"] = callable_symbol_s{
      .return_type = slp::slp_type_e::NONE,
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

  g_system_context.working_directory_storage = working_directory_;
  g_system_context.info.root_working_path =
      g_system_context.working_directory_storage.c_str();

  api_table_->register_function = register_function_callback;
  api_table_->eval = eval_callback;
  api_table_->get_system_info = get_system_info_callback;
  api_table_->system = &g_system_context;
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

  kernel_definition_context_s def_ctx;
  def_ctx.manager = this;
  def_ctx.kernel_name = kernel_name;
  def_ctx.kernel_dir = kernel_dir;

  auto def_symbols = get_kernel_definition_symbols(&def_ctx);
  auto def_interpreter = create_interpreter(def_symbols, nullptr, nullptr);

  std::vector<slp::slp_object_c> datums;

  if (kernel_obj.type() == slp::slp_type_e::BRACKET_LIST) {
    auto list = kernel_obj.as_list();
    for (size_t i = 0; i < list.size(); i++) {
      datums.push_back(list.at(i));
    }
  } else if (kernel_obj.type() == slp::slp_type_e::DATUM) {
    datums.push_back(std::move(kernel_obj));
  } else {
    logger_->error("kernel.sxs must contain datum declarations");
    return false;
  }

  try {
    for (auto &datum : datums) {
      def_interpreter->eval(datum);
    }
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

  for (const auto &[form_name, form_elements] : def_ctx.declared_forms) {
    if (parent_context_) {
      parent_context_->define_form(form_name, form_elements);
      logger_->debug("Registered kernel form: {}", form_name);
    }
  }

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
