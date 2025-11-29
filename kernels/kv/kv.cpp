#include "core/kernels/kernel_api.h"
#include "kvds/kvds.hpp"
#include "slp/slp.hpp"
#include <cstring>
#include <map>
#include <memory>
#include <string>

static const struct sxs_api_table_t *g_api = nullptr;

static sxs_object_t create_error(const std::string &message) {
  std::string error_str = "@(" + message + ")";
  auto parse_result = slp::parse(error_str);
  auto obj = parse_result.take();
  return new slp::slp_object_c(slp::slp_object_c::from_data(
      obj.get_data(), obj.get_symbols(), obj.get_root_offset()));
}

static std::map<std::string, std::shared_ptr<kvds::kv_c_distributor_c>>
    g_distributors;
static std::map<std::string, std::shared_ptr<kvds::kv_c>> g_stores;

static std::pair<std::string, std::string>
parse_symbol_key(const char *symbol_str) {
  std::string s(symbol_str ? symbol_str : "");
  size_t colon_pos = s.find(':');

  if (colon_pos == std::string::npos) {
    return {"", ""};
  }

  return {s.substr(0, colon_pos), s.substr(colon_pos + 1)};
}

static std::string value_to_string(sxs_object_t obj) {
  sxs_type_t type = g_api->get_type(obj);

  switch (type) {
  case SXS_TYPE_INT: {
    long long val = g_api->as_int(obj);
    return std::to_string(val);
  }
  case SXS_TYPE_REAL: {
    double val = g_api->as_real(obj);
    return std::to_string(val);
  }
  case SXS_TYPE_STRING: {
    const char *str = g_api->as_string(obj);
    return str ? str : "";
  }
  default:
    return "";
  }
}

static sxs_object_t string_to_value(const std::string &str) {
  if (str.empty()) {
    return g_api->create_string("");
  }

  if (str.find('.') != std::string::npos) {
    try {
      double val = std::stod(str);
      return g_api->create_real(val);
    } catch (...) {
      return g_api->create_string(str.c_str());
    }
  }

  try {
    long long val = std::stoll(str);
    return g_api->create_int(val);
  } catch (...) {
    return g_api->create_string(str.c_str());
  }
}

static sxs_object_t kv_open_memory(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 2) {
    return create_error("open-memory requires 1 argument");
  }

  sxs_object_t name_obj = g_api->list_at(list, 1);
  sxs_type_t type = g_api->get_type(name_obj);

  if (type != SXS_TYPE_SYMBOL) {
    return create_error("open-memory requires symbol argument");
  }

  const char *name = g_api->as_symbol(name_obj);
  if (!name) {
    return create_error("open-memory: invalid symbol");
  }

  std::string store_name(name);

  if (g_stores.find(store_name) != g_stores.end()) {
    return g_api->create_int(0);
  }

  if (g_distributors.find("__memory__") == g_distributors.end()) {
    g_distributors["__memory__"] =
        std::make_shared<kvds::kv_c_distributor_c>("");
  }

  auto distributor = g_distributors["__memory__"];
  auto store_opt = distributor->get_or_create_kv_c(
      store_name, kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);

  if (!store_opt.has_value()) {
    return create_error("open-memory: failed to create store");
  }

  g_stores[store_name] = store_opt.value();
  return g_api->create_int(0);
}

static sxs_object_t kv_open_disk(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return create_error("open-disk requires 2 arguments");
  }

  sxs_object_t name_obj = g_api->list_at(list, 1);
  sxs_object_t path_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_path = g_api->eval(ctx, path_obj);

  if (g_api->get_type(name_obj) != SXS_TYPE_SYMBOL ||
      g_api->get_type(evaled_path) != SXS_TYPE_STRING) {
    return create_error("open-disk requires symbol and string arguments");
  }

  const char *name = g_api->as_symbol(name_obj);
  const char *path = g_api->as_string(evaled_path);

  if (!name || !path) {
    return create_error("open-disk: invalid arguments");
  }

  std::string store_name(name);
  std::string disk_path(path);

  if (g_stores.find(store_name) != g_stores.end()) {
    return g_api->create_int(0);
  }

  if (g_distributors.find(disk_path) == g_distributors.end()) {
    g_distributors[disk_path] =
        std::make_shared<kvds::kv_c_distributor_c>(disk_path);
  }

  auto distributor = g_distributors[disk_path];
  auto store_opt = distributor->get_or_create_kv_c(
      store_name, kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

  if (!store_opt.has_value()) {
    return create_error("open-disk: failed to create store");
  }

  g_stores[store_name] = store_opt.value();
  return g_api->create_int(0);
}

static sxs_object_t kv_set(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return create_error("set requires 2 arguments");
  }

  sxs_object_t dest_obj = g_api->list_at(list, 1);
  sxs_object_t value_obj = g_api->list_at(list, 2);

  if (g_api->get_type(dest_obj) != SXS_TYPE_SYMBOL) {
    return create_error("set requires symbol:key format");
  }

  const char *dest_symbol = g_api->as_symbol(dest_obj);
  if (!dest_symbol) {
    return create_error("set: invalid symbol");
  }

  auto [store_name, key] = parse_symbol_key(dest_symbol);
  if (store_name.empty() || key.empty()) {
    return create_error("set requires symbol:key format");
  }

  auto store_it = g_stores.find(store_name);
  if (store_it == g_stores.end()) {
    return create_error("set: store not found");
  }

  sxs_object_t evaled_value = g_api->eval(ctx, value_obj);
  std::string value_str = value_to_string(evaled_value);

  bool success = store_it->second->set(key, value_str);
  return success ? g_api->create_int(0)
                 : create_error("set: failed to store value");
}

static sxs_object_t kv_get(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 2) {
    return create_error("get requires 1 argument");
  }

  sxs_object_t source_obj = g_api->list_at(list, 1);

  if (g_api->get_type(source_obj) != SXS_TYPE_SYMBOL) {
    return create_error("get requires symbol:key format");
  }

  const char *source_symbol = g_api->as_symbol(source_obj);
  if (!source_symbol) {
    return create_error("get: invalid symbol");
  }

  auto [store_name, key] = parse_symbol_key(source_symbol);
  if (store_name.empty() || key.empty()) {
    return create_error("get requires symbol:key format");
  }

  auto store_it = g_stores.find(store_name);
  if (store_it == g_stores.end()) {
    return create_error("get: store not found");
  }

  std::string value;
  bool success = store_it->second->get(key, value);

  if (!success) {
    return create_error("get: key not found");
  }

  return string_to_value(value);
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
  g_api = api;
  api->register_function(registry, "open-memory", kv_open_memory, SXS_TYPE_INT,
                         0);
  api->register_function(registry, "open-disk", kv_open_disk, SXS_TYPE_INT, 0);
  api->register_function(registry, "set", kv_set, SXS_TYPE_INT, 0);
  api->register_function(registry, "get", kv_get, SXS_TYPE_NONE, 0);
}
