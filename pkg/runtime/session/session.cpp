#include "runtime/session/session.hpp"
#include "runtime/entity/entity.hpp"
#include <spdlog/spdlog.h>
#include <sstream>

namespace runtime {

scoped_kv_c::scoped_kv_c(kvds::kv_c* underlying, const std::string& scope, entity_c* entity)
  : underlying_(underlying), scope_(scope), entity_(entity) {}

bool scoped_kv_c::is_open() const {
  return underlying_ && underlying_->is_open();
}

std::string scoped_kv_c::add_scope_prefix(const std::string& key) const {
  return scope_ + "/" + key;
}

std::string scoped_kv_c::remove_scope_prefix(const std::string& key) const {
  std::string prefix = scope_ + "/";
  if (key.find(prefix) == 0) {
    return key.substr(prefix.length());
  }
  return key;
}

bool scoped_kv_c::check_read_permission() const {
  if (!entity_) {
    return false;
  }
  return entity_->is_permitted(scope_, permission::READ_ONLY) ||
         entity_->is_permitted(scope_, permission::READ_WRITE);
}

bool scoped_kv_c::check_write_permission() const {
  if (!entity_) {
    return false;
  }
  return entity_->is_permitted(scope_, permission::WRITE_ONLY) ||
         entity_->is_permitted(scope_, permission::READ_WRITE);
}

bool scoped_kv_c::set(const std::string& key, const std::string& value) {
  if (!check_write_permission()) {
    return false;
  }
  return underlying_->set(add_scope_prefix(key), value);
}

bool scoped_kv_c::get(const std::string& key, std::string& value) {
  if (!check_read_permission()) {
    return false;
  }
  return underlying_->get(add_scope_prefix(key), value);
}

bool scoped_kv_c::del(const std::string& key) {
  if (!check_write_permission()) {
    return false;
  }
  return underlying_->del(add_scope_prefix(key));
}

bool scoped_kv_c::exists(const std::string& key) const {
  if (!check_read_permission()) {
    return false;
  }
  return underlying_->exists(add_scope_prefix(key));
}

bool scoped_kv_c::set_batch(const std::map<std::string, std::string>& kv_pairs) {
  if (!check_write_permission()) {
    return false;
  }
  
  std::map<std::string, std::string> scoped_pairs;
  for (const auto& pair : kv_pairs) {
    scoped_pairs[add_scope_prefix(pair.first)] = pair.second;
  }
  
  return underlying_->set_batch(scoped_pairs);
}

bool scoped_kv_c::set_nx(const std::string& key, const std::string& value) {
  if (!check_write_permission()) {
    return false;
  }
  return underlying_->set_nx(add_scope_prefix(key), value);
}

bool scoped_kv_c::compare_and_swap(const std::string& key,
                                    const std::string& expected_value,
                                    const std::string& new_value) {
  if (!check_write_permission()) {
    return false;
  }
  return underlying_->compare_and_swap(add_scope_prefix(key), expected_value, new_value);
}

void scoped_kv_c::iterate(const std::string& prefix,
                          std::function<bool(const std::string& key, const std::string& value)> callback) const {
  if (!check_read_permission()) {
    return;
  }
  
  std::string scoped_prefix = add_scope_prefix(prefix);
  
  underlying_->iterate(scoped_prefix, [this, callback](const std::string& key, const std::string& value) {
    std::string unscoped_key = remove_scope_prefix(key);
    return callback(unscoped_key, value);
  });
}

session_c::session_c(const std::string& session_id,
                     const std::string& entity_id,
                     const std::string& scope,
                     entity_c* entity,
                     kvds::kv_c* datastore,
                     events::event_system_c* event_system)
  : id_(session_id),
    entity_id_(entity_id),
    scope_(scope),
    active_(true),
    creation_time_(std::time(nullptr)),
    entity_(entity),
    scoped_store_(std::make_unique<scoped_kv_c>(datastore, scope, entity)),
    event_system_(event_system) {}

std::string session_c::get_id() const {
  return id_;
}

std::string session_c::get_entity_id() const {
  return entity_id_;
}

std::string session_c::get_scope() const {
  return scope_;
}

bool session_c::is_active() const {
  return active_;
}

std::time_t session_c::get_creation_time() const {
  return creation_time_;
}

void session_c::set_active(bool active) {
  active_ = active;
}

kvds::kv_c* session_c::get_store() {
  return scoped_store_.get();
}

bool session_c::publish_event(events::event_category_e category, std::uint16_t topic_id, const std::any& payload) {
  if (!entity_) {
    return false;
  }

  if (!entity_->is_permitted_topic(topic_id, topic_permission::PUBLISH) &&
      !entity_->is_permitted_topic(topic_id, topic_permission::PUBSUB)) {
    return false;
  }

  if (!event_system_) {
    return false;
  }

  auto producer = event_system_->get_event_producer_for_category(category);
  if (!producer) {
    return false;
  }

  auto topic_writer = producer->get_topic_writer_for_topic(topic_id);
  if (!topic_writer) {
    return false;
  }

  events::event_s event;
  event.category = category;
  event.topic_identifier = topic_id;
  event.payload = payload;

  topic_writer->write_event(event);
  return true;
}

bool session_c::subscribe_to_topic(std::uint16_t topic_id, std::function<void(const events::event_s&)> handler) {
  if (!entity_) {
    return false;
  }

  if (!entity_->is_permitted_topic(topic_id, topic_permission::SUBSCRIBE) &&
      !entity_->is_permitted_topic(topic_id, topic_permission::PUBSUB)) {
    return false;
  }

  if (!event_system_) {
    return false;
  }

  topic_handlers_[topic_id] = handler;
  
  auto consumer = new session_event_consumer_c(this);
  topic_consumers_[topic_id] = consumer;
  event_system_->register_consumer(topic_id, consumer);
  return true;
}

bool session_c::unsubscribe_from_topic(std::uint16_t topic_id) {
  auto it = topic_handlers_.find(topic_id);
  if (it == topic_handlers_.end()) {
    return false;
  }

  topic_handlers_.erase(it);
  
  auto consumer_it = topic_consumers_.find(topic_id);
  if (consumer_it != topic_consumers_.end()) {
    topic_consumers_.erase(consumer_it);
  }
  
  return true;
}

void session_c::consume_event(const events::event_s& event) {
  auto it = topic_handlers_.find(event.topic_identifier);
  if (it != topic_handlers_.end()) {
    it->second(event);
  }
}

session_c::session_event_consumer_c::session_event_consumer_c(session_c* session)
  : session_(session) {}

void session_c::session_event_consumer_c::consume_event(const events::event_s& event) {
  if (session_) {
    session_->consume_event(event);
  }
}

session_subsystem_c::session_subsystem_c(logger_t logger, size_t max_sessions_per_entity)
  : logger_(logger),
    max_sessions_per_entity_(max_sessions_per_entity),
    running_(false),
    session_store_(nullptr),
    datastore_(nullptr),
    entity_store_(nullptr),
    event_system_(nullptr),
    session_counter_(0) {}

const char* session_subsystem_c::get_name() const {
  return name_;
}

void session_subsystem_c::initialize(runtime_accessor_t accessor) {
  accessor_ = accessor;
  
  logger_->info("[{}] Initializing session subsystem", name_);
  
  running_ = true;
}

void session_subsystem_c::shutdown() {
  logger_->info("[{}] Shutting down session subsystem", name_);
  
  sessions_.clear();
  entity_session_counts_.clear();
  
  running_ = false;
}

bool session_subsystem_c::is_running() const {
  return running_;
}

std::string session_subsystem_c::generate_session_id(const std::string& entity_id) {
  std::ostringstream oss;
  oss << entity_id << "_session_" << session_counter_++;
  return oss.str();
}

entity_c* session_subsystem_c::get_entity(const std::string& entity_id) {
  auto it = entity_cache_.find(entity_id);
  if (it != entity_cache_.end()) {
    return it->second.get();
  }
  
  if (!entity_manager_) {
    logger_->error("[{}] Entity manager not initialized", name_);
    return nullptr;
  }
  
  auto entity_opt = entity_manager_->get_or_create<entity_c>(entity_id);
  if (!entity_opt.has_value()) {
    logger_->error("[{}] Failed to get or create entity: {}", name_, entity_id);
    return nullptr;
  }
  
  auto entity = std::shared_ptr<entity_c>(std::move(entity_opt.value()));
  entity_cache_[entity_id] = entity;
  
  return entity.get();
}

bool session_subsystem_c::persist_session_metadata(const session_c& session) {
  if (!session_store_) {
    return false;
  }
  
  std::ostringstream oss;
  oss << session.get_entity_id() << "|"
      << session.get_scope() << "|"
      << (session.is_active() ? "1" : "0") << "|"
      << session.get_creation_time();
  
  return session_store_->set(session.get_id(), oss.str());
}

bool session_subsystem_c::load_session_metadata(const std::string& session_id) {
  if (!session_store_) {
    return false;
  }
  
  std::string metadata;
  if (!session_store_->get(session_id, metadata)) {
    return false;
  }
  
  return true;
}

std::shared_ptr<session_c> session_subsystem_c::create_session(const std::string& entity_id, const std::string& scope) {
  if (!running_) {
    logger_->error("[{}] Cannot create session: subsystem not running", name_);
    return nullptr;
  }
  
  auto it = entity_session_counts_.find(entity_id);
  size_t current_count = (it != entity_session_counts_.end()) ? it->second : 0;
  
  if (current_count >= max_sessions_per_entity_) {
    logger_->error("[{}] Cannot create session for entity {}: max sessions ({}) reached", 
                   name_, entity_id, max_sessions_per_entity_);
    return nullptr;
  }
  
  entity_c* entity = get_entity(entity_id);
  if (!entity) {
    logger_->error("[{}] Cannot create session: entity {} not found", name_, entity_id);
    return nullptr;
  }
  
  std::string session_id = generate_session_id(entity_id);
  
  auto session = std::make_shared<session_c>(session_id, entity_id, scope, entity, datastore_, event_system_);
  
  if (!persist_session_metadata(*session)) {
    logger_->error("[{}] Failed to persist session metadata for {}", name_, session_id);
    return nullptr;
  }
  
  sessions_[session_id] = session;
  entity_session_counts_[entity_id] = current_count + 1;
  
  logger_->info("[{}] Created session {} for entity {} with scope {}", 
                name_, session_id, entity_id, scope);
  
  return session;
}

std::shared_ptr<session_c> session_subsystem_c::get_session(const std::string& session_id) {
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    return it->second;
  }
  
  return nullptr;
}

std::vector<std::shared_ptr<session_c>> session_subsystem_c::list_sessions() {
  std::vector<std::shared_ptr<session_c>> result;
  for (const auto& pair : sessions_) {
    result.push_back(pair.second);
  }
  return result;
}

std::vector<std::shared_ptr<session_c>> session_subsystem_c::list_sessions_by_entity(const std::string& entity_id) {
  std::vector<std::shared_ptr<session_c>> result;
  for (const auto& pair : sessions_) {
    if (pair.second->get_entity_id() == entity_id) {
      result.push_back(pair.second);
    }
  }
  return result;
}

bool session_subsystem_c::close_session(const std::string& session_id) {
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) {
    logger_->error("[{}] Cannot close session {}: not found", name_, session_id);
    return false;
  }
  
  auto session = it->second;
  session->set_active(false);
  
  if (!persist_session_metadata(*session)) {
    logger_->error("[{}] Failed to persist session metadata for {}", name_, session_id);
    return false;
  }
  
  std::string entity_id = session->get_entity_id();
  auto count_it = entity_session_counts_.find(entity_id);
  if (count_it != entity_session_counts_.end() && count_it->second > 0) {
    count_it->second--;
  }
  
  logger_->info("[{}] Closed session {}", name_, session_id);
  return true;
}

bool session_subsystem_c::destroy_session(const std::string& session_id) {
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) {
    logger_->error("[{}] Cannot destroy session {}: not found", name_, session_id);
    return false;
  }
  
  std::string entity_id = it->second->get_entity_id();
  
  if (session_store_) {
    session_store_->del(session_id);
  }
  
  sessions_.erase(it);
  
  auto count_it = entity_session_counts_.find(entity_id);
  if (count_it != entity_session_counts_.end() && count_it->second > 0) {
    count_it->second--;
  }
  
  logger_->info("[{}] Destroyed session {}", name_, session_id);
  return true;
}

void session_subsystem_c::set_session_store(kvds::kv_c* store) {
  session_store_ = store;
}

void session_subsystem_c::set_datastore(kvds::kv_c* store) {
  datastore_ = store;
}

void session_subsystem_c::set_entity_store(kvds::kv_c* store) {
  entity_store_ = store;
  if (entity_store_) {
    auto spdlog_logger = spdlog::get("runtime");
    if (!spdlog_logger) {
      spdlog_logger = spdlog::default_logger();
    }
    entity_manager_ = std::make_unique<record::record_manager_c>(*entity_store_, spdlog_logger);
    logger_->info("[{}] Entity manager initialized", name_);
  }
}

void session_subsystem_c::set_event_system(events::event_system_c* event_system) {
  event_system_ = event_system;
  logger_->info("[{}] Event system wired", name_);
}

} // namespace runtime
