#pragma once

#include <any>
#include <types/shared_obj.hpp>
#include "runtime/runtime.hpp"

namespace runtime::events {

class event_producer_if;
class topic_writer_if;
using event_producer_t = pkg::types::shared_obj_c<event_producer_if>;
using topic_writer_t = pkg::types::shared_obj_c<topic_writer_if>;

enum class event_origin_e {
    RUNTIME_SUBSYSTEM_UNKNOWN = 0,
    RUNTIME_SUBSYSTEM_SYSTEM = 1,
    RUNTIME_SYBSYSTEM_ENTITIES = 2,
    RUNTIME_SUBSYSTEM_SESSIONS = 3,
};

struct event_s {
    event_origin_e origin{event_origin_e::RUNTIME_SUBSYSTEM_UNKNOWN};
    std::uint16_t topic_identifier{0};
    std::any payload{nullptr};
};

class topic_writer_if : public pkg::types::shared_c {
public:
  virtual ~topic_writer_if() = default;
  virtual void write_event(const event_s &event) = 0;
};

class event_producer_if : public pkg::types::shared_c {
public:
  virtual ~event_producer_if() = default;
  virtual topic_writer_t get_topic_writer_for_topic(std::uint16_t topic_identifier) = 0;
};

class event_system_c : public runtime::runtime_subsystem_if {
public:
  event_system_c(runtime::logger_t logger);
  ~event_system_c();

  const char* get_name() const override final;
  void initialize(runtime::runtime_accessor_t accessor) override final;
  void shutdown() override final;
  bool is_running() const override final;

  event_producer_t get_event_producer_for_origin(event_origin_e origin);

private:

class specific_topic_writer_c : public topic_writer_if {
public:
  specific_topic_writer_c(event_system_c* event_system, event_origin_e origin, std::uint16_t topic_identifier);
  ~specific_topic_writer_c();

  void write_event(const event_s &event) override final;

private:
  event_system_c* event_system_;
  event_origin_e origin_;
  std::uint16_t topic_identifier_;
};

class specific_event_producer_c : public event_producer_if {
public:
  specific_event_producer_c(event_system_c* event_system, event_origin_e origin);
  ~specific_event_producer_c();

  topic_writer_t get_topic_writer_for_topic(std::uint16_t topic_identifier) override final;

private:
  event_system_c* event_system_;
  event_origin_e origin_;
};

  void handle_event(const event_s &event);

  runtime::logger_t logger_;
  bool running_;
  runtime::runtime_accessor_t accessor_;
  const char* name_{"event_system_c"};
};

} // namespace runtime::events