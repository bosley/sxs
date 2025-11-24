#pragma once

#include "runtime/events/events.hpp"
#include "runtime/runtime.hpp"
#include <any>
#include <ctime>
#include <functional>
#include <kvds/kvds.hpp>
#include <map>
#include <memory>
#include <record/record.hpp>
#include <string>

namespace runtime {

class entity_c;

enum class publish_result_e {
  OK,
  RATE_LIMIT_EXCEEDED,
  PERMISSION_DENIED,
  NO_ENTITY,
  NO_EVENT_SYSTEM,
  NO_PRODUCER,
  NO_TOPIC_WRITER
};

class scoped_kv_c : public kvds::kv_c {
public:
  scoped_kv_c(kvds::kv_c *underlying, const std::string &scope,
              entity_c *entity);
  ~scoped_kv_c() = default;

  bool is_open() const override;
  bool set(const std::string &key, const std::string &value) override;
  bool get(const std::string &key, std::string &value) override;
  bool del(const std::string &key) override;
  bool exists(const std::string &key) const override;
  bool set_batch(const std::map<std::string, std::string> &kv_pairs) override;
  bool delete_batch(const std::vector<std::string> &keys) override;
  bool set_nx(const std::string &key, const std::string &value) override;
  bool compare_and_swap(const std::string &key,
                        const std::string &expected_value,
                        const std::string &new_value) override;
  void
  iterate(const std::string &prefix,
          std::function<bool(const std::string &key, const std::string &value)>
              callback) const override;

private:
  kvds::kv_c *underlying_;
  std::string scope_;
  entity_c *entity_;

  std::string add_scope_prefix(const std::string &key) const;
  std::string remove_scope_prefix(const std::string &key) const;
  bool check_read_permission() const;
  bool check_write_permission() const;
};

class session_c {
public:
  session_c(const session_c &) = delete;
  session_c(session_c &&) = delete;
  session_c &operator=(const session_c &) = delete;
  session_c &operator=(session_c &&) = delete;

  session_c(const std::string &session_id, const std::string &entity_id,
            const std::string &scope, entity_c *entity, kvds::kv_c *datastore,
            events::event_system_c *event_system);
  ~session_c() = default;

  std::string get_id() const;
  std::string get_entity_id() const;
  std::string get_scope() const;
  bool is_active() const;
  std::time_t get_creation_time() const;

  void set_active(bool active);
  kvds::kv_c *get_store();

  publish_result_e publish_event(events::event_category_e category, 
                                 std::uint16_t topic_id,
                                 const std::any &payload);
  bool subscribe_to_topic(events::event_category_e category,
                          std::uint16_t topic_id,
                          std::function<void(const events::event_s &)> handler);
  bool unsubscribe_from_topic(events::event_category_e category,
                              std::uint16_t topic_id);

private:
  class session_event_consumer_c : public events::event_consumer_if {
  public:
    session_event_consumer_c(session_c *session);
    ~session_event_consumer_c() = default;
    void consume_event(const events::event_s &event) override;

  private:
    session_c *session_;
  };

  void consume_event(const events::event_s &event);

  std::string id_;
  std::string entity_id_;
  std::string scope_;
  bool active_;
  std::time_t creation_time_;
  entity_c *entity_;
  std::unique_ptr<scoped_kv_c> scoped_store_;
  events::event_system_c *event_system_;
  std::map<std::pair<events::event_category_e, std::uint16_t>,
           std::function<void(const events::event_s &)>>
      topic_handlers_;
  std::map<std::uint16_t, events::event_consumer_t> topic_consumers_;
};

class session_subsystem_c : public runtime_subsystem_if {
public:
  session_subsystem_c(logger_t logger, size_t max_sessions_per_entity);
  ~session_subsystem_c() = default;

  const char *get_name() const override final;
  void initialize(runtime_accessor_t accessor) override final;
  void shutdown() override final;
  bool is_running() const override final;

  std::shared_ptr<session_c> create_session(const std::string &entity_id,
                                            const std::string &scope);
  std::shared_ptr<session_c> get_session(const std::string &session_id);
  std::vector<std::shared_ptr<session_c>> list_sessions();
  std::vector<std::shared_ptr<session_c>>
  list_sessions_by_entity(const std::string &entity_id);
  bool close_session(const std::string &session_id);
  bool destroy_session(const std::string &session_id);

  void set_session_store(kvds::kv_c *store);
  void set_datastore(kvds::kv_c *store);
  void set_entity_store(kvds::kv_c *store);
  void set_event_system(events::event_system_c *event_system);

private:
  logger_t logger_;
  size_t max_sessions_per_entity_;
  bool running_;
  runtime_accessor_t accessor_;
  const char *name_{"session_subsystem_c"};

  kvds::kv_c *session_store_;
  kvds::kv_c *datastore_;
  kvds::kv_c *entity_store_;
  events::event_system_c *event_system_;
  std::unique_ptr<record::record_manager_c> entity_manager_;

  std::map<std::string, std::shared_ptr<session_c>> sessions_;
  std::map<std::string, std::shared_ptr<entity_c>> entity_cache_;
  std::map<std::string, size_t> entity_session_counts_;
  size_t session_counter_;

  std::string generate_session_id(const std::string &entity_id);
  entity_c *get_entity(const std::string &entity_id);
  bool persist_session_metadata(const session_c &session);
  bool load_session_metadata(const std::string &session_id);
};

} // namespace runtime
