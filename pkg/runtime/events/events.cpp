#include "runtime/events/events.hpp"

namespace runtime::events {

event_system_c::event_system_c(runtime::logger_t logger)
  : logger_(logger), running_(false) {}

event_system_c::~event_system_c() {
  if (running_) {
    shutdown();
  }
}

const char* event_system_c::get_name() const {
  return name_;
}

void event_system_c::initialize(runtime::runtime_accessor_t accessor) {
  accessor_ = accessor;
  
  logger_->info("[{}] Initializing event system", name_);
  
  running_ = true;
}

void event_system_c::shutdown() {
  logger_->info("[{}] Shutting down", name_);
  
  running_ = false;
}

bool event_system_c::is_running() const {
  return running_;
}

event_producer_t event_system_c::get_event_producer_for_origin(event_origin_e origin) {
  return new specific_event_producer_c(this, origin);
}

void event_system_c::handle_event(const event_s &event) {
  logger_->debug("[{}] Received event from origin {} for topic {}", 
    name_, 
    static_cast<int>(event.origin), 
    event.topic_identifier);
}

event_system_c::specific_topic_writer_c::specific_topic_writer_c(
  event_system_c* event_system, 
  event_origin_e origin, 
  std::uint16_t topic_identifier)
  : event_system_(event_system), origin_(origin), topic_identifier_(topic_identifier) {}

event_system_c::specific_topic_writer_c::~specific_topic_writer_c() {}

void event_system_c::specific_topic_writer_c::write_event(const event_s &event) {
  event_s modified_event = event;
  modified_event.origin = origin_;
  modified_event.topic_identifier = topic_identifier_;
  
  event_system_->handle_event(modified_event);
}

event_system_c::specific_event_producer_c::specific_event_producer_c(
  event_system_c* event_system, 
  event_origin_e origin)
  : event_system_(event_system), origin_(origin) {}

event_system_c::specific_event_producer_c::~specific_event_producer_c() {}

topic_writer_t event_system_c::specific_event_producer_c::get_topic_writer_for_topic(
  std::uint16_t topic_identifier) {
  return new specific_topic_writer_c(event_system_, origin_, topic_identifier);
}

} // namespace runtime::events

