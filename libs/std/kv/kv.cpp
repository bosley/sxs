#include "kvds/kvds.hpp"
#include <cstring>
#include <kernel_api.hpp>
#include <map>
#include <memory>
#include <string>

static const struct pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c create_error(const std::string &message) {
  std::string error_str = "@(" + message + ")";
  auto parse_result = slp::parse(error_str);
  return parse_result.take();
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

static std::string serialize_slp_object(const slp::slp_object_c &obj) {
  const auto &buffer = obj.get_data();
  const auto &symbols = obj.get_symbols();
  size_t root_offset = obj.get_root_offset();

  std::string result;

  size_t buffer_size = buffer.size();
  result.append(reinterpret_cast<const char *>(&buffer_size), sizeof(size_t));
  result.append(reinterpret_cast<const char *>(buffer.data()), buffer_size);

  size_t symbols_count = symbols.size();
  result.append(reinterpret_cast<const char *>(&symbols_count), sizeof(size_t));

  for (const auto &[key, value] : symbols) {
    result.append(reinterpret_cast<const char *>(&key), sizeof(uint64_t));
    size_t value_len = value.size();
    result.append(reinterpret_cast<const char *>(&value_len), sizeof(size_t));
    result.append(value);
  }

  result.append(reinterpret_cast<const char *>(&root_offset), sizeof(size_t));

  return result;
}

static slp::slp_object_c deserialize_slp_object(const std::string &serialized) {
  const char *data = serialized.data();
  size_t pos = 0;

  if (serialized.size() < sizeof(size_t)) {
    return create_error("deserialize: invalid data");
  }

  size_t buffer_size;
  std::memcpy(&buffer_size, data + pos, sizeof(size_t));
  pos += sizeof(size_t);

  if (pos + buffer_size > serialized.size()) {
    return create_error("deserialize: buffer overflow");
  }

  slp::slp_buffer_c buffer;
  buffer.resize(buffer_size);
  std::memcpy(buffer.data(), data + pos, buffer_size);
  pos += buffer_size;

  if (pos + sizeof(size_t) > serialized.size()) {
    return create_error("deserialize: invalid symbols");
  }

  size_t symbols_count;
  std::memcpy(&symbols_count, data + pos, sizeof(size_t));
  pos += sizeof(size_t);

  std::map<std::uint64_t, std::string> symbols;
  for (size_t i = 0; i < symbols_count; i++) {
    if (pos + sizeof(uint64_t) + sizeof(size_t) > serialized.size()) {
      return create_error("deserialize: symbol data corrupt");
    }

    uint64_t key;
    std::memcpy(&key, data + pos, sizeof(uint64_t));
    pos += sizeof(uint64_t);

    size_t value_len;
    std::memcpy(&value_len, data + pos, sizeof(size_t));
    pos += sizeof(size_t);

    if (pos + value_len > serialized.size()) {
      return create_error("deserialize: symbol value overflow");
    }

    std::string value(data + pos, value_len);
    pos += value_len;

    symbols[key] = value;
  }

  if (pos + sizeof(size_t) > serialized.size()) {
    return create_error("deserialize: missing root offset");
  }

  size_t root_offset;
  std::memcpy(&root_offset, data + pos, sizeof(size_t));

  return slp::slp_object_c::from_data(buffer, symbols, root_offset);
}

static slp::slp_object_c kv_open_memory(pkg::kernel::context_t ctx,
                                        const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return create_error("open-memory requires 1 argument");
  }

  auto name_obj = list.at(1);
  if (name_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("open-memory requires symbol argument");
  }

  const char *name = name_obj.as_symbol();
  if (!name) {
    return create_error("open-memory: invalid symbol");
  }

  std::string store_name(name);

  if (g_stores.find(store_name) != g_stores.end()) {
    return slp::slp_object_c::create_int(0);
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
  return slp::slp_object_c::create_int(0);
}

static slp::slp_object_c kv_open_disk(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return create_error("open-disk requires 2 arguments");
  }

  auto name_obj = list.at(1);
  auto path_obj = list.at(2);

  auto evaled_path = g_api->eval(ctx, path_obj);

  if (name_obj.type() != slp::slp_type_e::SYMBOL ||
      evaled_path.type() != slp::slp_type_e::DQ_LIST) {
    return create_error("open-disk requires symbol and string arguments");
  }

  const char *name = name_obj.as_symbol();
  std::string path = evaled_path.as_string().to_string();

  if (!name) {
    return create_error("open-disk: invalid arguments");
  }

  std::string store_name(name);

  if (g_stores.find(store_name) != g_stores.end()) {
    return slp::slp_object_c::create_int(0);
  }

  if (g_distributors.find(path) == g_distributors.end()) {
    g_distributors[path] = std::make_shared<kvds::kv_c_distributor_c>(path);
  }

  auto distributor = g_distributors[path];
  auto store_opt = distributor->get_or_create_kv_c(
      store_name, kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

  if (!store_opt.has_value()) {
    return create_error("open-disk: failed to create store");
  }

  g_stores[store_name] = store_opt.value();
  return slp::slp_object_c::create_int(0);
}

static slp::slp_object_c kv_set(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return create_error("set requires 2 arguments");
  }

  auto dest_obj = list.at(1);
  auto value_obj = list.at(2);

  if (dest_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("set requires symbol:key format");
  }

  const char *dest_symbol = dest_obj.as_symbol();
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

  auto evaled_value = g_api->eval(ctx, value_obj);
  std::string serialized = serialize_slp_object(evaled_value);

  bool success = store_it->second->set(key, serialized);
  return success ? slp::slp_object_c::create_int(0)
                 : create_error("set: failed to store value");
}

static slp::slp_object_c kv_get(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return create_error("get requires 1 argument");
  }

  auto source_obj = list.at(1);

  if (source_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("get requires symbol:key format");
  }

  const char *source_symbol = source_obj.as_symbol();
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

  std::string serialized;
  bool success = store_it->second->get(key, serialized);

  if (!success) {
    return create_error("get: key not found");
  }

  return deserialize_slp_object(serialized);
}

static slp::slp_object_c kv_del(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return create_error("del requires 1 argument");
  }

  auto source_obj = list.at(1);

  if (source_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("del requires symbol:key format");
  }

  const char *source_symbol = source_obj.as_symbol();
  if (!source_symbol) {
    return create_error("del: invalid symbol");
  }

  auto [store_name, key] = parse_symbol_key(source_symbol);
  if (store_name.empty() || key.empty()) {
    return create_error("del requires symbol:key format");
  }

  auto store_it = g_stores.find(store_name);
  if (store_it == g_stores.end()) {
    return create_error("del: store not found");
  }

  bool success = store_it->second->del(key);
  return success ? slp::slp_object_c::create_int(0)
                 : create_error("del: failed to delete key");
}

static slp::slp_object_c kv_snx(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return create_error("snx requires 2 arguments");
  }

  auto dest_obj = list.at(1);
  auto value_obj = list.at(2);

  if (dest_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("snx requires symbol:key format");
  }

  const char *dest_symbol = dest_obj.as_symbol();
  if (!dest_symbol) {
    return create_error("snx: invalid symbol");
  }

  auto [store_name, key] = parse_symbol_key(dest_symbol);
  if (store_name.empty() || key.empty()) {
    return create_error("snx requires symbol:key format");
  }

  auto store_it = g_stores.find(store_name);
  if (store_it == g_stores.end()) {
    return create_error("snx: store not found");
  }

  auto evaled_value = g_api->eval(ctx, value_obj);
  std::string serialized = serialize_slp_object(evaled_value);

  bool success = store_it->second->set_nx(key, serialized);
  return success ? slp::slp_object_c::create_int(0)
                 : create_error("snx: key already exists");
}

static slp::slp_object_c kv_cas(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    return create_error("cas requires 3 arguments");
  }

  auto dest_obj = list.at(1);
  auto expected_obj = list.at(2);
  auto new_obj = list.at(3);

  if (dest_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("cas requires symbol:key format");
  }

  const char *dest_symbol = dest_obj.as_symbol();
  if (!dest_symbol) {
    return create_error("cas: invalid symbol");
  }

  auto [store_name, key] = parse_symbol_key(dest_symbol);
  if (store_name.empty() || key.empty()) {
    return create_error("cas requires symbol:key format");
  }

  auto store_it = g_stores.find(store_name);
  if (store_it == g_stores.end()) {
    return create_error("cas: store not found");
  }

  auto evaled_expected = g_api->eval(ctx, expected_obj);
  auto evaled_new = g_api->eval(ctx, new_obj);

  std::string serialized_expected = serialize_slp_object(evaled_expected);
  std::string serialized_new = serialize_slp_object(evaled_new);

  bool success = store_it->second->compare_and_swap(key, serialized_expected,
                                                    serialized_new);
  return success ? slp::slp_object_c::create_int(0)
                 : create_error("cas: comparison failed");
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "open-memory", kv_open_memory,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "open-disk", kv_open_disk,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "set", kv_set, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "get", kv_get, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "del", kv_del, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "snx", kv_snx, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "cas", kv_cas, slp::slp_type_e::INTEGER, 0);
}

extern "C" void kernel_shutdown(const struct pkg::kernel::api_table_s *api) {
  g_stores.clear();
  g_distributors.clear();
}
