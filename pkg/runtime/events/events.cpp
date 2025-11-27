#include "runtime/events/events.hpp"

namespace runtime::events {

event_system_c::event_system_c(runtime::logger_t logger, size_t max_threads,
                               size_t max_queue_size)
    : logger_(logger), running_(false), max_threads_(max_threads),
      max_queue_size_(max_queue_size), shutdown_requested_(false) {
  logger_->info("[{}] Created with {} worker threads and queue size {}", name_,
                max_threads_, max_queue_size_);
}

event_system_c::~event_system_c() {
  if (running_) {
    shutdown();
  }
}

const char *event_system_c::get_name() const { return name_; }

void event_system_c::initialize(runtime::runtime_accessor_t accessor) {
  accessor_ = accessor;

  logger_->info("[{}] Initializing event system", name_);

  shutdown_requested_ = false;

  logger_->info("[{}] Spawning {} worker threads", name_, max_threads_);
  for (size_t i = 0; i < max_threads_; ++i) {
    worker_threads_.emplace_back(&event_system_c::worker_thread_func, this);
    logger_->debug("[{}] Spawned worker thread #{}", name_, i);
  }

  running_ = true;
  logger_->info("[{}] Event system initialized with {} threads", name_,
                worker_threads_.size());
}

void event_system_c::shutdown() {
  logger_->info("[{}] Shutting down event system", name_);

  {
    std::unique_lock<std::mutex> lock(mutex_);
    shutdown_requested_ = true;
    logger_->info("[{}] Shutdown requested, {} events remaining in queue",
                  name_, event_queue_.size());
  }

  queue_not_empty_.notify_all();
  queue_not_full_.notify_all();

  logger_->info("[{}] Waiting for {} worker threads to complete", name_,
                worker_threads_.size());
  size_t thread_idx = 0;
  for (auto &thread : worker_threads_) {
    if (thread.joinable()) {
      logger_->debug("[{}] Joining worker thread #{}", name_, thread_idx);
      thread.join();
    }
    ++thread_idx;
  }
  worker_threads_.clear();

  {
    std::unique_lock<std::mutex> lock(mutex_);
    size_t remaining = event_queue_.size();
    if (remaining > 0) {
      logger_->warn("[{}] Clearing {} unprocessed events from queue", name_,
                    remaining);
    }
    std::queue<event_s> empty_queue;
    std::swap(event_queue_, empty_queue);
  }

  running_ = false;
  logger_->info("[{}] Event system shutdown complete", name_);
}

bool event_system_c::is_running() const { return running_; }

bool event_system_c::is_queue_empty() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return event_queue_.empty();
}

event_producer_t
event_system_c::get_event_producer_for_category(event_category_e category) {
  assert(static_cast<int>(category) >= 0 &&
         static_cast<int>(category) <
             static_cast<int>(event_category_e::SENTINEL) &&
         "event category must be between 0 and 8");

  return std::shared_ptr<event_producer_if>(
      new specific_event_producer_c(*this, category));
}

void event_system_c::handle_event(const event_s &event) {
  std::unique_lock<std::mutex> lock(mutex_);

  queue_not_full_.wait(lock, [this]() {
    return event_queue_.size() < max_queue_size_ || shutdown_requested_;
  });

  if (shutdown_requested_) {
    logger_->warn("[{}] Event rejected - shutdown in progress", name_);
    return;
  }

  event_queue_.push(event);
  size_t queue_size = event_queue_.size();

  logger_->debug(
      "[{}] Enqueued event from category {} for topic {} (queue: {}/{})", name_,
      static_cast<int>(event.category), event.topic_identifier, queue_size,
      max_queue_size_);

  queue_not_empty_.notify_one();
}

void event_system_c::worker_thread_func() {
  std::hash<std::thread::id> hasher;
  size_t thread_hash = hasher(std::this_thread::get_id());

  logger_->info("[{}] Worker thread {:x} starting", name_, thread_hash);

  while (true) {
    event_s event;
    {
      std::unique_lock<std::mutex> lock(mutex_);

      queue_not_empty_.wait(lock, [this]() {
        return !event_queue_.empty() || shutdown_requested_;
      });

      if (shutdown_requested_ && event_queue_.empty()) {
        logger_->info("[{}] Worker thread {:x} shutting down", name_,
                      thread_hash);
        break;
      }

      event = event_queue_.front();
      event_queue_.pop();

      queue_not_full_.notify_one();
    }

    logger_->debug(
        "[{}] Worker {:x} processing event from category {} for topic {}",
        name_, thread_hash, static_cast<int>(event.category),
        event.topic_identifier);

    auto it = topic_consumers_.find(event.topic_identifier);
    if (it != topic_consumers_.end()) {
      for (auto &consumer : it->second) {
        try {
          consumer->consume_event(event);
        } catch (const std::exception &e) {
          logger_->error("[{}] Consumer exception for topic {}: {}", name_,
                         event.topic_identifier, e.what());
        } catch (...) {
          logger_->error("[{}] Unknown consumer exception for topic {}", name_,
                         event.topic_identifier);
        }
      }
    } else {
      logger_->debug("[{}] No consumers registered for topic {}", name_,
                     event.topic_identifier);
    }
  }
}

event_system_c::specific_topic_writer_c::specific_topic_writer_c(
    event_system_c &event_system, event_category_e category,
    std::uint16_t topic_identifier)
    : event_system_(event_system), category_(category),
      topic_identifier_(topic_identifier) {
  assert(static_cast<int>(category) >
             static_cast<int>(event_category_e::RUNTIME_SUBSYSTEM_UNKNOWN) &&
         static_cast<int>(category) <
             static_cast<int>(event_category_e::SENTINEL) &&
         "event category must be between 0 and 8");
}

event_system_c::specific_topic_writer_c::~specific_topic_writer_c() {}

void event_system_c::specific_topic_writer_c::write_event(
    const event_s &event) {
  event_s modified_event = event;
  modified_event.category = category_;
  modified_event.topic_identifier = topic_identifier_;

  event_system_.handle_event(modified_event);
}

event_system_c::specific_event_producer_c::specific_event_producer_c(
    event_system_c &event_system, event_category_e category)
    : event_system_(event_system), category_(category) {
  assert(static_cast<int>(category) >
             static_cast<int>(event_category_e::RUNTIME_SUBSYSTEM_UNKNOWN) &&
         static_cast<int>(category) <
             static_cast<int>(event_category_e::SENTINEL) &&
         "event category must be between 0 and 8");
}

event_system_c::specific_event_producer_c::~specific_event_producer_c() {}

topic_writer_t
event_system_c::specific_event_producer_c::get_topic_writer_for_topic(
    std::uint16_t topic_identifier) {
  return std::shared_ptr<topic_writer_if>(
      new specific_topic_writer_c(event_system_, category_, topic_identifier));
}

void event_system_c::register_consumer(std::uint16_t topic_identifier,
                                       event_consumer_t consumer) {
  std::unique_lock<std::mutex> lock(mutex_);

  topic_consumers_[topic_identifier].push_back(consumer);
  size_t consumer_count = topic_consumers_[topic_identifier].size();

  logger_->info("[{}] Registered consumer for topic {} (total consumers: {})",
                name_, topic_identifier, consumer_count);
}

} // namespace runtime::events
