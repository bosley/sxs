#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace runtime {

using logger_t = spdlog::logger *;
class runtime_c;
class runtime_subsystem_if;
class runtime_accessor_if;
class processor_c;
using runtime_t = std::shared_ptr<runtime_c>;
using runtime_subsystem_t = std::unique_ptr<runtime_subsystem_if>;
using runtime_accessor_t = std::shared_ptr<runtime_accessor_if>;

struct options_s {
  bool validate_only{false};
  std::string runtime_root_path;
  std::vector<std::string> include_paths;
  size_t event_system_max_threads{4};
  size_t event_system_max_queue_size{1000};
  size_t max_sessions_per_entity{10};
  size_t num_processors{1}; // 1 lowest each processor is a consumer on a topic. 0th process processes all execution requests for topic 0, proc 1 doers topic 1 and so on. this way we can publish to any given processor
};

class runtime_accessor_if {
public:
  virtual ~runtime_accessor_if() = default;
  virtual void raise_warning(const char *message) = 0;
  virtual void raise_error(const char *message) = 0;
};

class runtime_subsystem_if {
public:
  virtual ~runtime_subsystem_if() = default;
  virtual const char *get_name() const = 0;
  virtual void initialize(runtime_accessor_t accessor) = 0;
  virtual void shutdown() = 0;
  virtual bool is_running() const = 0;
};

class runtime_c {
public:
  runtime_c(const runtime_c &) = delete;
  runtime_c(runtime_c &&) = delete;
  runtime_c &operator=(const runtime_c &) = delete;
  runtime_c &operator=(runtime_c &&) = delete;

  runtime_c(const options_s &options);
  ~runtime_c();

  bool initialize();
  bool shutdown();
  bool is_running() const;

  logger_t get_logger() const;

private:
  /*
    When we create a subsystem we hand it a specific_accessor_c so it can
    raise errors and interface with the runtime in a way that we track
    the caller
  */
  class specific_accessor_c : public runtime_accessor_if {
  public:
    specific_accessor_c(const specific_accessor_c &) = delete;
    specific_accessor_c(specific_accessor_c &&) = delete;
    specific_accessor_c &operator=(const specific_accessor_c &) = delete;
    specific_accessor_c &operator=(specific_accessor_c &&) = delete;

    specific_accessor_c(runtime_subsystem_if *subsystem);
    ~specific_accessor_c() = default;

    void raise_warning(const char *message) override final;
    void raise_error(const char *message) override final;

  private:
    runtime_c *runtime_;
    runtime_subsystem_if *subsystem_;
  };

  options_s options_;
  bool running_;
  logger_t logger_;
  std::shared_ptr<spdlog::logger> spdlog_logger_;

  std::vector<runtime_subsystem_t> subsystems_;
  std::vector<std::unique_ptr<processor_c>> processors_;
};

} // namespace runtime
