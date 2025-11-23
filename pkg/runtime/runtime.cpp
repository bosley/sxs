#include "runtime/runtime.hpp"
#include "runtime/system/system.hpp"
#include "runtime/events/events.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace runtime {

runtime_c::runtime_c(const options_s &options) : options_(options), running_(false) {
  spdlog_logger_ = spdlog::stdout_color_mt("runtime");
  spdlog_logger_->set_level(spdlog::level::info);
  spdlog_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
  
  logger_ = spdlog_logger_.get();
  
  subsystems_.push_back(
    std::unique_ptr<runtime_subsystem_if>(
      new events::event_system_c(logger_)
    )
  );

  subsystems_.push_back(
    std::unique_ptr<runtime_subsystem_if>(
      new system_c(logger_, options_.runtime_root_path)
    )
  );
  
}

bool runtime_c::initialize() {
  if (running_) {
    return false;
  }
  
  logger_->info("Initializing runtime subsystems...");
  
  for (auto &subsystem : subsystems_) {
    logger_->info("Initializing subsystem: {}", subsystem->get_name());
    
    auto accessor = new specific_accessor_c(subsystem.get());
    subsystem->initialize(accessor);
    
    if (!subsystem->is_running()) {
      logger_->error("Failed to initialize subsystem: {}", subsystem->get_name());
      return false;
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

logger_t runtime_c::get_logger() const { 
  return logger_; 
}

runtime_c::specific_accessor_c::specific_accessor_c(runtime_subsystem_if* subsystem)
  : runtime_(nullptr), subsystem_(subsystem) {}

void runtime_c::specific_accessor_c::raise_warning(const char* message) {
  auto spdlog_logger = spdlog::get("runtime");
  if (spdlog_logger) {
    spdlog_logger->warn("[{}] {}", subsystem_->get_name(), message);
  }
}

void runtime_c::specific_accessor_c::raise_error(const char* message) {
  auto spdlog_logger = spdlog::get("runtime");
  if (spdlog_logger) {
    spdlog_logger->error("[{}] {}", subsystem_->get_name(), message);
  }
}

} // namespace runtime

