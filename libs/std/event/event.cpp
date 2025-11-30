#include "events/events.hpp"
#include <cstring>
#include <kernel_api.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>

static const struct pkg::kernel::api_table_s *g_api = nullptr;
static std::unique_ptr<pkg::events::event_system_c> g_event_system;
static std::map<std::string, std::shared_ptr<pkg::events::publisher_if>>
    g_publishers;
static std::mutex g_publishers_mutex;

static slp::slp_object_c create_error(const std::string &message) {
  std::string error_str = "@(" + message + ")";
  auto parse_result = slp::parse(error_str);
  return parse_result.take();
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

struct subscriber_entry_s {
  std::size_t id;
  std::size_t event_system_id;
  std::string topic;
  std::string serialized_lambda;
};

static std::map<std::size_t, subscriber_entry_s> g_subscribers;
static std::mutex g_subscribers_mutex;

class lambda_subscriber_c : public pkg::events::subscriber_if {
public:
  lambda_subscriber_c(std::size_t sub_id) : sub_id_(sub_id) {}

  void on_event(const pkg::events::event_c &event) override {}

private:
  std::size_t sub_id_;
};

static std::map<std::size_t, std::unique_ptr<lambda_subscriber_c>>
    g_subscriber_impls;

static slp::slp_object_c event_subscribe(pkg::kernel::context_t ctx,
                                         const slp::slp_object_c &args) {
  if (!g_event_system) {
    return create_error("subscribe: event system not initialized");
  }

  auto list = args.as_list();
  if (list.size() < 3) {
    return create_error("subscribe requires 2 arguments");
  }

  auto topic_obj = list.at(1);
  auto lambda_obj = list.at(2);

  if (topic_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("subscribe requires symbol topic");
  }

  const char *topic = topic_obj.as_symbol();
  if (!topic) {
    return create_error("subscribe: invalid topic");
  }

  std::string serialized_lambda = serialize_slp_object(lambda_obj);

  static std::size_t next_sub_id = 1;
  std::size_t sub_id = next_sub_id++;

  auto subscriber_impl = std::make_unique<lambda_subscriber_c>(sub_id);
  std::size_t event_sub_id =
      g_event_system->subscribe(topic, subscriber_impl.get());

  if (event_sub_id == 0) {
    return create_error("subscribe: failed to subscribe");
  }

  subscriber_entry_s entry;
  entry.id = sub_id;
  entry.event_system_id = event_sub_id;
  entry.topic = topic;
  entry.serialized_lambda = serialized_lambda;

  {
    std::lock_guard<std::mutex> lock(g_subscribers_mutex);
    g_subscribers[sub_id] = entry;
    g_subscriber_impls[sub_id] = std::move(subscriber_impl);
  }

  return slp::slp_object_c::create_int(static_cast<long long>(sub_id));
}

static slp::slp_object_c event_unsubscribe(pkg::kernel::context_t ctx,
                                           const slp::slp_object_c &args) {
  if (!g_event_system) {
    return create_error("unsubscribe: event system not initialized");
  }

  auto list = args.as_list();
  if (list.size() < 2) {
    return create_error("unsubscribe requires 1 argument");
  }

  auto id_obj = list.at(1);
  auto evaled_id = g_api->eval(ctx, id_obj);

  if (evaled_id.type() != slp::slp_type_e::INTEGER) {
    return create_error("unsubscribe requires integer id");
  }

  long long id = evaled_id.as_int();

  std::size_t event_system_id;
  {
    std::lock_guard<std::mutex> lock(g_subscribers_mutex);
    auto it = g_subscribers.find(static_cast<std::size_t>(id));
    if (it == g_subscribers.end()) {
      return create_error("unsubscribe: id not found");
    }

    event_system_id = it->second.event_system_id;
    g_subscribers.erase(it);
    g_subscriber_impls.erase(static_cast<std::size_t>(id));
  }

  g_event_system->unsubscribe(event_system_id);

  return slp::slp_object_c::create_int(0);
}

static slp::slp_object_c event_publish(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  if (!g_event_system) {
    return create_error("publish: event system not initialized");
  }

  auto list = args.as_list();
  if (list.size() < 3) {
    return create_error("publish requires 2 arguments");
  }

  auto topic_obj = list.at(1);
  auto data_obj = list.at(2);

  if (topic_obj.type() != slp::slp_type_e::SYMBOL) {
    return create_error("publish requires symbol topic");
  }

  const char *topic = topic_obj.as_symbol();
  if (!topic) {
    return create_error("publish: invalid topic");
  }

  auto evaled_data = g_api->eval(ctx, data_obj);
  std::string serialized_data = serialize_slp_object(evaled_data);

  std::shared_ptr<pkg::events::publisher_if> publisher;
  {
    std::lock_guard<std::mutex> lock(g_publishers_mutex);
    auto it = g_publishers.find(topic);
    if (it == g_publishers.end()) {
      publisher = g_event_system->get_publisher(topic, 1000);
      if (!publisher) {
        return create_error("publish: failed to create publisher");
      }
      g_publishers[topic] = publisher;
    } else {
      publisher = it->second;
    }
  }

  pkg::events::event_c event;
  event.topic = topic;
  event.encoded_slp_data = serialized_data;

  bool success = publisher->publish(event);
  return success ? slp::slp_object_c::create_int(0)
                 : create_error("publish: failed to publish");
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;

  if (!g_event_system) {
    pkg::events::options_s options;
    options.logger = spdlog::default_logger();
    options.num_threads = 4;
    options.max_queue_size = 10000;

    g_event_system = std::make_unique<pkg::events::event_system_c>(options);
    g_event_system->start();
  }

  api->register_function(registry, "subscribe", event_subscribe,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "unsubscribe", event_unsubscribe,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "publish", event_publish,
                         slp::slp_type_e::INTEGER, 0);
}

extern "C" void kernel_shutdown(const struct pkg::kernel::api_table_s *api) {
  g_subscribers.clear();
  g_subscriber_impls.clear();
  g_publishers.clear();
  if (g_event_system) {
    g_event_system->stop();
    g_event_system.reset();
  }
}
