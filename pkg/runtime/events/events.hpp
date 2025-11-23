#pragma once

#include <any>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <vector>
#include <types/shared_obj.hpp>
#include "runtime/runtime.hpp"

namespace runtime::events {

class event_producer_if;
class topic_writer_if;
class event_consumer_if;
using event_producer_t = pkg::types::shared_obj_c<event_producer_if>;
using topic_writer_t = pkg::types::shared_obj_c<topic_writer_if>;
using event_consumer_t = pkg::types::shared_obj_c<event_consumer_if>;

enum class event_category_e {
    RUNTIME_SUBSYSTEM_UNKNOWN = 0,
    RUNTIME_EXECUTION_REQUEST = 1,
    RUNTIME_BACKCHANNEL_A = 2,
    RUNTIME_BACKCHANNEL_B = 3,
    RUNTIME_BACKCHANNEL_C = 4,
    RUNTIME_BACKCHANNEL_D = 5,
    RUNTIME_BACKCHANNEL_E = 6,
    RUNTIME_BACKCHANNEL_F = 7,
};

struct event_s {
    event_category_e category{event_category_e::RUNTIME_SUBSYSTEM_UNKNOWN};
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

class event_consumer_if : public pkg::types::shared_c {
public:
  virtual ~event_consumer_if() = default;
  virtual void consume_event(const event_s &event) = 0;
};

class event_system_c : public runtime::runtime_subsystem_if {
public:
  event_system_c(runtime::logger_t logger, size_t max_threads = 4, size_t max_queue_size = 1000);
  ~event_system_c();

  const char* get_name() const override final;
  void initialize(runtime::runtime_accessor_t accessor) override final;
  void shutdown() override final;
  bool is_running() const override final;

  event_producer_t get_event_producer_for_category(event_category_e category);
  void register_consumer(std::uint16_t topic_identifier, event_consumer_t consumer);

private:

class specific_topic_writer_c : public topic_writer_if {
public:
  specific_topic_writer_c(event_system_c* event_system, event_category_e category, std::uint16_t topic_identifier);
  ~specific_topic_writer_c();

  void write_event(const event_s &event) override final;

private:
  event_system_c* event_system_;
  event_category_e category_;
  std::uint16_t topic_identifier_;
};

class specific_event_producer_c : public event_producer_if {
public:
  specific_event_producer_c(event_system_c* event_system, event_category_e category);
  ~specific_event_producer_c();

  topic_writer_t get_topic_writer_for_topic(std::uint16_t topic_identifier) override final;

private:
  event_system_c* event_system_;
  event_category_e category_;
};

  void handle_event(const event_s &event);
  void worker_thread_func();

  runtime::logger_t logger_;
  bool running_;
  runtime::runtime_accessor_t accessor_;
  const char* name_{"event_system_c"};

  size_t max_threads_;
  size_t max_queue_size_;
  std::vector<std::thread> worker_threads_;
  std::queue<event_s> event_queue_;
  std::map<std::uint16_t, std::vector<event_consumer_t>> topic_consumers_;
  std::mutex mutex_;
  std::condition_variable queue_not_empty_;
  std::condition_variable queue_not_full_;
  bool shutdown_requested_;
};

} // namespace runtime::events