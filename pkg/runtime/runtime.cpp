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

  event_system_ = std::make_unique<events::event_system_c>(
      logger_, options_.event_system_max_threads,
      options_.event_system_max_queue_size);

  system_ = std::make_unique<system_c>(logger_, options_.runtime_root_path);

  session_subsystem_ = std::make_unique<session_subsystem_c>(
      logger_, options_.max_sessions_per_entity);
}

runtime_c::~runtime_c() {}

bool runtime_c::initialize() {
  if (running_) {
    return false;
  }

  logger_->info("Initializing runtime subsystems...");

  if (!initialize_event_system()) {
    return false;
  }

  if (!initialize_system()) {
    return false;
  }

  if (!initialize_session_subsystem()) {
    return false;
  }

  wire_subsystems();
  create_processors();

  running_ = true;
  return true;
}

bool runtime_c::initialize_event_system() {
  logger_->info("Initializing subsystem: {}", event_system_->get_name());
  auto event_accessor = std::shared_ptr<runtime_accessor_if>(
      new specific_accessor_c(*event_system_));
  event_system_->initialize(event_accessor);
  if (!event_system_->is_running()) {
    logger_->error("Failed to initialize subsystem: {}",
                   event_system_->get_name());
    return false;
  }
  return true;
}

bool runtime_c::initialize_system() {
  logger_->info("Initializing subsystem: {}", system_->get_name());
  auto system_accessor =
      std::shared_ptr<runtime_accessor_if>(new specific_accessor_c(*system_));
  system_->initialize(system_accessor);
  if (!system_->is_running()) {
    logger_->error("Failed to initialize subsystem: {}", system_->get_name());
    return false;
  }
  return true;
}

bool runtime_c::initialize_session_subsystem() {
  logger_->info("Initializing subsystem: {}", session_subsystem_->get_name());
  auto session_accessor = std::shared_ptr<runtime_accessor_if>(
      new specific_accessor_c(*session_subsystem_));
  session_subsystem_->initialize(session_accessor);
  if (!session_subsystem_->is_running()) {
    logger_->error("Failed to initialize subsystem: {}",
                   session_subsystem_->get_name());
    return false;
  }
  return true;
}

void runtime_c::wire_subsystems() {
  logger_->info("Wiring session subsystem to system stores");
  session_subsystem_->set_entity_store(system_->get_entity_store());
  session_subsystem_->set_session_store(system_->get_session_store());
  session_subsystem_->set_datastore(system_->get_datastore_store());

  logger_->info("Wiring session subsystem to event system");
  session_subsystem_->set_event_system(event_system_.get());
}

void runtime_c::create_processors() {
  size_t num_processors = std::max(options_.num_processors, size_t(1));
  logger_->info("Creating and registering {} processor(s)", num_processors);

  for (size_t i = 0; i < num_processors; i++) {
    auto processor = std::make_unique<processor_c>(logger_, *event_system_);
    auto processor_consumer = std::shared_ptr<events::event_consumer_if>(
        processor.get(), [](events::event_consumer_if *) {});
    event_system_->register_consumer(static_cast<std::uint16_t>(i),
                                     processor_consumer);
    logger_->info(
        "Processor {} registered for RUNTIME_EXECUTION_REQUEST on topic {}", i,
        i);
    processors_.push_back(std::move(processor));
  }
}

bool runtime_c::shutdown() {
  if (!running_) {
    return false;
  }

  logger_->info("Shutting down runtime subsystems...");

  logger_->info("Shutting down subsystem: {}", session_subsystem_->get_name());
  session_subsystem_->shutdown();

  logger_->info("Shutting down subsystem: {}", system_->get_name());
  system_->shutdown();

  logger_->info("Shutting down subsystem: {}", event_system_->get_name());
  event_system_->shutdown();

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
