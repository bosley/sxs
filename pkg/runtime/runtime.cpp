#include "runtime/runtime.hpp"
#include "runtime/events/events.hpp"
#include "runtime/processor.hpp"
#include "runtime/session/session.hpp"
#include "runtime/system/system.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace runtime {

runtime_c::runtime_c(const options_s &options)
    : options_(options), running_(false) {
  spdlog_logger_ = spdlog::stdout_color_mt("runtime");
  spdlog_logger_->set_level(spdlog::level::info);
  spdlog_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

  logger_ = spdlog_logger_.get();

  subsystems_.push_back(std::unique_ptr<runtime_subsystem_if>(
      new events::event_system_c(logger_, options_.event_system_max_threads,
                                 options_.event_system_max_queue_size)));

  subsystems_.push_back(std::unique_ptr<runtime_subsystem_if>(
      new system_c(logger_, options_.runtime_root_path)));

  subsystems_.push_back(std::unique_ptr<runtime_subsystem_if>(
      new session_subsystem_c(logger_, options_.max_sessions_per_entity)));
}

runtime_c::~runtime_c() {}

bool runtime_c::initialize() {
  if (running_) {
    return false;
  }

  logger_->info("Initializing runtime subsystems...");

  system_c *system_subsystem = nullptr;
  session_subsystem_c *session_subsystem = nullptr;
  events::event_system_c *event_subsystem = nullptr;

  for (auto &subsystem : subsystems_) {
    logger_->info("Initializing subsystem: {}", subsystem->get_name());

    auto accessor = std::shared_ptr<runtime_accessor_if>(
        new specific_accessor_c(*subsystem));
    subsystem->initialize(accessor);

    if (!subsystem->is_running()) {
      logger_->error("Failed to initialize subsystem: {}",
                     subsystem->get_name());
      return false;
    }

    if (std::string(subsystem->get_name()) == "system_c") {
      system_subsystem = dynamic_cast<system_c *>(subsystem.get());
    } else if (std::string(subsystem->get_name()) == "session_subsystem_c") {
      session_subsystem = dynamic_cast<session_subsystem_c *>(subsystem.get());
    } else if (std::string(subsystem->get_name()) == "event_system_c") {
      event_subsystem = dynamic_cast<events::event_system_c *>(subsystem.get());
    }
  }

  if (system_subsystem && session_subsystem) {
    logger_->info("Wiring session subsystem to system stores");
    session_subsystem->set_entity_store(system_subsystem->get_entity_store());
    session_subsystem->set_session_store(system_subsystem->get_session_store());
    session_subsystem->set_datastore(system_subsystem->get_datastore_store());
  }

  if (event_subsystem && session_subsystem) {
    logger_->info("Wiring session subsystem to event system");
    session_subsystem->set_event_system(event_subsystem);
  }

  if (event_subsystem) {
    size_t num_processors = std::max(options_.num_processors, size_t(1));
    logger_->info("Creating and registering {} processor(s)", num_processors);

    for (size_t i = 0; i < num_processors; i++) {
      auto processor = std::make_unique<processor_c>(logger_, *event_subsystem);
      auto processor_consumer = std::shared_ptr<events::event_consumer_if>(
          processor.get(), [](events::event_consumer_if *) {});
      event_subsystem->register_consumer(static_cast<std::uint16_t>(i),
                                         processor_consumer);
      logger_->info(
          "Processor {} registered for RUNTIME_EXECUTION_REQUEST on topic {}",
          i, i);
      processors_.push_back(std::move(processor));
    }
  }

  running_ = true;
  return true;
}

bool runtime_c::shutdown() {
  if (!running_) {
    return false;
  }

  logger_->info("Shutting down runtime subsystems...");

  for (auto &subsystem : subsystems_) {
    logger_->info("Shutting down subsystem: {}", subsystem->get_name());
    subsystem->shutdown();
  }

  running_ = false;
  return true;
}

bool runtime_c::is_running() const { return running_; }

logger_t runtime_c::get_logger() const { return logger_; }

runtime_c::specific_accessor_c::specific_accessor_c(
    runtime_subsystem_if &subsystem)
    : subsystem_(subsystem) {}

void runtime_c::specific_accessor_c::raise_warning(const char *message) {
  auto spdlog_logger = spdlog::get("runtime");
  if (spdlog_logger) {
    spdlog_logger->warn("[{}] {}", subsystem_.get_name(), message);
  }
}

void runtime_c::specific_accessor_c::raise_error(const char *message) {
  auto spdlog_logger = spdlog::get("runtime");
  if (spdlog_logger) {
    spdlog_logger->error("[{}] {}", subsystem_.get_name(), message);
  }
}

} // namespace runtime
