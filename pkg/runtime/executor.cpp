#include "runtime/events/events.hpp"
#include "runtime/processor.hpp"
#include "runtime/runtime.hpp"
#include "runtime/session/session.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace runtime {

runtime_c::script_executor_c::script_executor_c(runtime_c &runtime,
                                                const std::string &entity_id,
                                                const std::string &scope)
    : runtime_(runtime), entity_id_(entity_id), has_error_(false) {
  auto logger = runtime_.get_logger();
  logger->info("Creating script executor for entity '{}' with scope '{}'",
               entity_id, scope);

  session_ = runtime_.session_subsystem_->create_session(entity_id, scope);
  if (!session_) {
    logger->error("Failed to create session for entity: {}", entity_id);
    has_error_ = true;
    last_error_ = "Failed to create session";
  }
}

runtime_c::script_executor_c::~script_executor_c() {}

bool runtime_c::script_executor_c::execute(const std::string &script_text) {
  has_error_ = false;
  last_error_.clear();
  last_result_.clear();

  auto logger = runtime_.get_logger();

  if (!session_) {
    logger->error("Cannot execute: session not initialized");
    has_error_ = true;
    last_error_ = "Session not initialized";
    return false;
  }

  try {
    logger->info("Executing script for session: {}", session_->get_id());

    execution_request_s request{*session_, script_text, "script_exec"};

    events::event_s event;
    event.category = events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    auto producer =
        runtime_.event_system_->get_event_producer_for_category(event.category);
    if (!producer) {
      logger->error(
          "Failed to get event producer for RUNTIME_EXECUTION_REQUEST");
      has_error_ = true;
      last_error_ = "Failed to get event producer";
      return false;
    }

    auto writer = producer->get_topic_writer_for_topic(event.topic_identifier);
    if (!writer) {
      logger->error("Failed to get topic writer for topic: {}",
                    event.topic_identifier);
      has_error_ = true;
      last_error_ = "Failed to get topic writer";
      return false;
    }

    writer->write_event(event);

    logger->info("Script execution event published, waiting for completion...");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    while (true) {
      bool queue_empty = runtime_.event_system_->is_queue_empty();
      bool all_idle = true;
      for (const auto &processor : runtime_.processors_) {
        if (processor->is_busy()) {
          all_idle = false;
          break;
        }
      }

      if (queue_empty && all_idle) {
        queue_empty = runtime_.event_system_->is_queue_empty();
        all_idle = true;
        for (const auto &processor : runtime_.processors_) {
          if (processor->is_busy()) {
            all_idle = false;
            break;
          }
        }
        if (queue_empty && all_idle) {
          logger->info("Script execution complete");
          break;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return true;

  } catch (const insist_failure_exception &e) {
    logger->error("Script execution failed (insist): {}", e.what());
    has_error_ = true;
    last_error_ = std::string("insist failure: ") + e.what();
    return false;
  } catch (const std::exception &e) {
    logger->error("Script execution failed: {}", e.what());
    has_error_ = true;
    last_error_ = e.what();
    return false;
  }
}

std::string runtime_c::script_executor_c::get_last_result() const {
  return last_result_;
}

bool runtime_c::script_executor_c::has_error() const { return has_error_; }

std::string runtime_c::script_executor_c::get_last_error() const {
  return last_error_;
}

bool runtime_c::script_executor_c::require_topic_range(std::uint16_t start,
                                                       std::uint16_t end) {
  auto logger = runtime_.get_logger();
  logger->info("Granting topic range {}-{} to entity '{}'", start, end,
               entity_id_);

  if (!runtime_.session_subsystem_->grant_entity_topic_range(entity_id_, start,
                                                             end)) {
    logger->error("Failed to grant topic range to entity: {}", entity_id_);
    has_error_ = true;
    last_error_ = "Failed to grant topic permissions";
    return false;
  }

  return true;
}

} // namespace runtime