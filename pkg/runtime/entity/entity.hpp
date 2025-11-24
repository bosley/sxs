#pragma once

#include "runtime/runtime.hpp"
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <record/record.hpp>
#include <string>

namespace runtime {

namespace permission {
constexpr const char *READ_ONLY = "R";
constexpr const char *WRITE_ONLY = "W";
constexpr const char *READ_WRITE = "RW";
} // namespace permission

namespace topic_permission {
constexpr const char *PUBLISH = "P";
constexpr const char *SUBSCRIBE = "S";
constexpr const char *PUBSUB = "PS";
} // namespace topic_permission

class entity_c : public record::record_base_c {
public:
  entity_c();
  ~entity_c() = default;

  std::string get_type_id() const override;
  std::string get_schema() const override;
  size_t field_count() const override;
  bool get_field(size_t index, std::string &value) override;
  bool set_field(size_t index, const std::string &value) override;
  bool load() override;

  std::string get_id() const;
  bool is_permitted(const std::string &scope, const char *permission) const;
  void grant_permission(const std::string &scope, const char *permission);
  void revoke_permission(const std::string &scope);
  std::map<std::string, std::string> get_permissions() const;
  void set_permissions(const std::map<std::string, std::string> &perms);

  bool is_permitted_topic(std::uint16_t topic_id, const char *permission) const;
  void grant_topic_permission(std::uint16_t topic_id, const char *permission);
  void revoke_topic_permission(std::uint16_t topic_id);
  std::map<std::uint16_t, std::string> get_topic_permissions() const;

  void set_max_rps(std::uint32_t max_rps);
  std::uint32_t get_max_rps() const;
  bool try_publish();

private:
  std::map<std::string, std::string> permissions_;
  std::map<std::uint16_t, std::string> topic_permissions_;
  std::uint32_t max_rps_;
  std::deque<std::chrono::steady_clock::time_point> publish_timestamps_;
  mutable std::mutex publish_mutex_;

  std::string serialize_permissions() const;
  void deserialize_permissions(const std::string &data);
  std::string serialize_topic_permissions() const;
  void deserialize_topic_permissions(const std::string &data);
  void cleanup_old_timestamps();
};

class entity_subsystem_c : public runtime_subsystem_if {
public:
  entity_subsystem_c(logger_t logger);
  ~entity_subsystem_c() = default;

  const char *get_name() const override final;
  void initialize(runtime_accessor_t accessor) override final;
  void shutdown() override final;
  bool is_running() const override final;
};

} // namespace runtime
