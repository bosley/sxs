#include "kernels.hpp"
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sxs/kernel_api.h>
#include <sxs/slp/slp.hpp>

namespace pkg::core::kernels {

namespace {

struct registration_context_s {
  kernel_manager_c *manager;
  std::string kernel_name;
  const sxs_api_table_t *api;
};

void register_function_callback(sxs_registry_t registry, const char *name,
                                sxs_kernel_fn_t function,
                                sxs_type_t return_type, int variadic) {
  auto *ctx = static_cast<registration_context_s *>(registry);
  ctx->manager->register_kernel_function(
      ctx->kernel_name, name, reinterpret_cast<void *>(function),
      static_cast<int>(return_type), variadic != 0);
}

sxs_object_t eval_callback(sxs_context_t ctx, sxs_object_t obj) {
  auto *context = static_cast<callable_context_if *>(ctx);
  auto *object = static_cast<slp::slp_object_c *>(obj);
  auto result = context->eval(*object);
  return new slp::slp_object_c(std::move(result));
}

sxs_type_t get_type_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  return static_cast<sxs_type_t>(object->type());
}

long long as_int_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  return object->as_int();
}

double as_real_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  return object->as_real();
}

const char *as_string_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  static thread_local std::string str_buffer;
  str_buffer = object->as_string().to_string();
  return str_buffer.c_str();
}

void *as_list_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  return new slp::slp_object_c::list_c(object->as_list());
}

size_t list_size_callback(void *list) {
  auto *list_obj = static_cast<slp::slp_object_c::list_c *>(list);
  return list_obj->size();
}

sxs_object_t list_at_callback(void *list, size_t index) {
  auto *list_obj = static_cast<slp::slp_object_c::list_c *>(list);
  auto elem = list_obj->at(index);
  return new slp::slp_object_c(std::move(elem));
}

/*
  Implementations note and lament:

  Here for helpers on kernel-level calls i didn't want to have to expose or
  otherwise leverage slp directly too much in kernels and i wanted to keep the
  api minimal

  to this end, when creating slp objects from kernel implementations call back
  to here to actually instantiate things. depending on the program and the
  implementation of the kernel, this might be dreadfully slow. here, we are
  casting whatever they give us to various strings and data representations and
  then using the slp parser to ensure the form is stable. yes, this ensures the
  most correct potential approach, but it will forever pain me to see data
  transform primitive types at all just for the sake of c++ understandability.
*/

sxs_object_t create_none_callback() {
  auto parse_result = slp::parse("()");
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

sxs_object_t create_int_callback(long long value) {
  auto parse_result = slp::parse(std::to_string(value));
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

sxs_object_t create_real_callback(double value) {
  auto parse_result = slp::parse(std::to_string(value));
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

sxs_object_t create_string_callback(const char *value) {
  if (!value) {
    return create_none_callback();
  }

  auto obj = slp::create_string_direct(std::string(value));
  return new slp::slp_object_c(std::move(obj));
}

const char *as_symbol_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  return object->as_symbol();
}

sxs_object_t create_symbol_callback(const char *name) {
  if (!name) {
    return create_none_callback();
  }
  auto parse_result = slp::parse(std::string(name));
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

sxs_object_t create_paren_list_callback(sxs_object_t *objects, size_t count) {
  if (!objects || count == 0) {
    auto parse_result = slp::parse("()");
    auto obj = parse_result.take();
    return new slp::slp_object_c(slp::slp_object_c::from_data(
        obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
  }

  std::string list_str = "(";
  for (size_t i = 0; i < count; i++) {
    auto *elem = static_cast<slp::slp_object_c *>(objects[i]);
    if (elem->type() == slp::slp_type_e::INTEGER) {
      list_str += std::to_string(elem->as_int());
    } else if (elem->type() == slp::slp_type_e::REAL) {
      list_str += std::to_string(elem->as_real());
    } else if (elem->type() == slp::slp_type_e::DQ_LIST) {
      list_str += "\"" + elem->as_string().to_string() + "\"";
    } else if (elem->type() == slp::slp_type_e::SYMBOL) {
      list_str += elem->as_symbol();
    } else {
      list_str += "()";
    }
    if (i < count - 1) {
      list_str += " ";
    }
  }
  list_str += ")";

  auto parse_result = slp::parse(list_str);
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

sxs_object_t create_bracket_list_callback(sxs_object_t *objects, size_t count) {
  if (!objects || count == 0) {
    auto parse_result = slp::parse("[]");
    auto obj = parse_result.take();
    return new slp::slp_object_c(slp::slp_object_c::from_data(
        obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
  }

  std::string list_str = "[";
  for (size_t i = 0; i < count; i++) {
    auto *elem = static_cast<slp::slp_object_c *>(objects[i]);
    if (elem->type() == slp::slp_type_e::INTEGER) {
      list_str += std::to_string(elem->as_int());
    } else if (elem->type() == slp::slp_type_e::REAL) {
      list_str += std::to_string(elem->as_real());
    } else if (elem->type() == slp::slp_type_e::DQ_LIST) {
      list_str += "\"" + elem->as_string().to_string() + "\"";
    } else if (elem->type() == slp::slp_type_e::SYMBOL) {
      list_str += elem->as_symbol();
    } else {
      list_str += "()";
    }
    if (i < count - 1) {
      list_str += " ";
    }
  }
  list_str += "]";

  auto parse_result = slp::parse(list_str);
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

sxs_object_t create_brace_list_callback(sxs_object_t *objects, size_t count) {
  if (!objects || count == 0) {
    auto parse_result = slp::parse("{}");
    auto obj = parse_result.take();
    return new slp::slp_object_c(slp::slp_object_c::from_data(
        obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
  }

  std::string list_str = "{";
  for (size_t i = 0; i < count; i++) {
    auto *elem = static_cast<slp::slp_object_c *>(objects[i]);
    if (elem->type() == slp::slp_type_e::INTEGER) {
      list_str += std::to_string(elem->as_int());
    } else if (elem->type() == slp::slp_type_e::REAL) {
      list_str += std::to_string(elem->as_real());
    } else if (elem->type() == slp::slp_type_e::DQ_LIST) {
      list_str += "\"" + elem->as_string().to_string() + "\"";
    } else if (elem->type() == slp::slp_type_e::SYMBOL) {
      list_str += elem->as_symbol();
    } else {
      list_str += "()";
    }
    if (i < count - 1) {
      list_str += " ";
    }
  }
  list_str += "}";

  auto parse_result = slp::parse(list_str);
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

int some_has_value_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  return object->has_data() ? 1 : 0;
}

sxs_object_t some_get_value_callback(sxs_object_t obj) {
  auto *object = static_cast<slp::slp_object_c *>(obj);
  if (!object->has_data()) {
    return create_none_callback();
  }

  const std::uint8_t *base_ptr = object->get_data().data();
  const std::uint8_t *unit_ptr = base_ptr + object->get_root_offset();
  const slp::slp_unit_of_store_t *unit =
      reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);

  size_t inner_offset = static_cast<size_t>(unit->data.uint64);
  auto inner_obj = slp::slp_object_c::from_data(
      object->get_data(), object->get_symbols(), inner_offset);
  return new slp::slp_object_c(std::move(inner_obj));
}

} // namespace

kernel_manager_c::kernel_manager_c(logger_t logger,
                                   std::vector<std::string> include_paths,
                                   std::string working_directory)
    : logger_(logger), include_paths_(std::move(include_paths)),
      working_directory_(std::move(working_directory)), kernels_locked_(false),
      parent_context_(nullptr),
      api_table_(std::make_unique<sxs_api_table_t>()) {
  context_ = std::make_unique<kernel_context_c>(*this);

  api_table_->register_function = register_function_callback;
  api_table_->eval = eval_callback;
  api_table_->get_type = get_type_callback;
  api_table_->as_int = as_int_callback;
  api_table_->as_real = as_real_callback;
  api_table_->as_string = as_string_callback;
  api_table_->as_list = as_list_callback;
  api_table_->list_size = list_size_callback;
  api_table_->list_at = list_at_callback;
  api_table_->create_int = create_int_callback;
  api_table_->create_real = create_real_callback;
  api_table_->create_string = create_string_callback;
  api_table_->create_none = create_none_callback;
  api_table_->as_symbol = as_symbol_callback;
  api_table_->create_symbol = create_symbol_callback;
  api_table_->create_paren_list = create_paren_list_callback;
  api_table_->create_bracket_list = create_bracket_list_callback;
  api_table_->create_brace_list = create_brace_list_callback;
  api_table_->some_has_value = some_has_value_callback;
  api_table_->some_get_value = some_get_value_callback;
}

kernel_manager_c::~kernel_manager_c() {
  for (auto &[name, on_exit_fn] : kernel_on_exit_fns_) {
    logger_->debug("Calling kernel on_exit for: {}", name);
    on_exit_fn(api_table_.get());
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

  auto dylib_name_obj = list.at(2);
  if (dylib_name_obj.type() != slp::slp_type_e::DQ_LIST) {
    logger_->error("kernel.sxs dylib name must be a string");
    return false;
  }

  std::string dylib_name = dylib_name_obj.as_string().to_string();
  auto dylib_path = std::filesystem::path(kernel_dir) / dylib_name;

  if (!std::filesystem::exists(dylib_path)) {
    logger_->error("Kernel dylib not found: {}", dylib_path.string());
    return false;
  }

  std::string on_init_fn_name;
  std::string on_exit_fn_name;

  auto functions_obj = list.at(3);
  if (functions_obj.type() == slp::slp_type_e::BRACKET_LIST) {
    auto functions_list = functions_obj.as_list();
    for (size_t i = 0; i < functions_list.size(); i++) {
      auto func_def = functions_list.at(i);
      if (func_def.type() != slp::slp_type_e::PAREN_LIST) {
        continue;
      }

      auto func_list = func_def.as_list();
      if (func_list.size() < 2) {
        continue;
      }

      auto cmd = func_list.at(0);
      if (cmd.type() != slp::slp_type_e::SYMBOL) {
        continue;
      }

      std::string cmd_str = cmd.as_symbol();

      if (cmd_str == "define-ctor" && func_list.size() >= 2) {
        auto fn_name_obj = func_list.at(1);
        if (fn_name_obj.type() == slp::slp_type_e::SYMBOL) {
          on_init_fn_name = fn_name_obj.as_symbol();
        }
      } else if (cmd_str == "define-dtor" && func_list.size() >= 2) {
        auto fn_name_obj = func_list.at(1);
        if (fn_name_obj.type() == slp::slp_type_e::SYMBOL) {
          on_exit_fn_name = fn_name_obj.as_symbol();
        }
      }
    }
  }

  logger_->info("Loading kernel dylib: {}", dylib_path.string());

  void *handle = dlopen(dylib_path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    logger_->error("Failed to load kernel dylib: {}", dlerror());
    return false;
  }

  typedef void (*kernel_init_fn_t)(sxs_registry_t,
                                   const struct sxs_api_table_t *);
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

  if (!on_init_fn_name.empty()) {
    typedef void (*lifecycle_fn_t)(const struct sxs_api_table_t *);
    auto on_init_fn = reinterpret_cast<lifecycle_fn_t>(
        dlsym(handle, on_init_fn_name.c_str()));
    if (on_init_fn) {
      logger_->debug("Calling kernel on_init: {}", on_init_fn_name);
      on_init_fn(api_table_.get());
    }
  }

  if (!on_exit_fn_name.empty()) {
    typedef void (*lifecycle_fn_t)(const struct sxs_api_table_t *);
    auto on_exit_fn = reinterpret_cast<lifecycle_fn_t>(
        dlsym(handle, on_exit_fn_name.c_str()));
    if (on_exit_fn) {
      logger_->debug("Registered kernel on_exit: {}", on_exit_fn_name);
      kernel_on_exit_fns_[kernel_name] = on_exit_fn;
    }
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

  auto kernel_fn = reinterpret_cast<sxs_kernel_fn_t>(function_ptr);

  callable_symbol_s symbol;
  symbol.return_type = static_cast<slp::slp_type_e>(return_type);
  symbol.variadic = variadic;
  symbol.function = [kernel_fn,
                     this](callable_context_if &context,
                           slp::slp_object_c &args_list) -> slp::slp_object_c {
    sxs_object_t result = kernel_fn(static_cast<sxs_context_t>(&context),
                                    static_cast<sxs_object_t>(&args_list));

    if (result == nullptr) {
      slp::slp_object_c none_result;
      return none_result;
    }

    auto *result_obj = static_cast<slp::slp_object_c *>(result);
    slp::slp_object_c copy = slp::slp_object_c::from_data(
        result_obj->get_data(), result_obj->get_symbols(),
        result_obj->get_root_offset());
    delete result_obj;
    return copy;
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
