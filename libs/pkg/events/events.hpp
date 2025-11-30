#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace pkg::events {

typedef std::shared_ptr<spdlog::logger> logger_t;

struct options_s {
  logger_t logger;
  std::size_t num_threads;
  std::size_t max_queue_size;
};

struct event_c {
  std::string topic;
  std::string encoded_slp_data;
};

class publisher_if {
public:
  virtual ~publisher_if() = default;
  virtual bool publish(const event_c &event) = 0;
};

class subscriber_if {
public:
  virtual ~subscriber_if() = default;
  virtual void on_event(const event_c &event) = 0;
};

class event_system_c {
public:
  explicit event_system_c(const options_s &options);
  ~event_system_c();

  std::shared_ptr<publisher_if> get_publisher(const std::string &topic,
                                              std::size_t rps);

  std::size_t subscribe(const std::string &topic, subscriber_if *subscriber);

  void unsubscribe(std::size_t subscriber_id);

  void start();
  void stop();

private:
  class impl_c;
  std::unique_ptr<impl_c> impl_;
};

} // namespace pkg::events