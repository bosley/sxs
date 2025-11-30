#include "events/events.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace pkg::events {

class publisher_impl_c : public publisher_if {
public:
  publisher_impl_c(const std::string &topic, std::size_t rps,
                   std::queue<event_c> &queue, std::mutex &queue_mutex,
                   std::condition_variable &queue_cv,
                   std::size_t max_queue_size, std::atomic<bool> &running)
      : topic_(topic), rps_(rps), queue_(queue), queue_mutex_(queue_mutex),
        queue_cv_(queue_cv), max_queue_size_(max_queue_size), running_(running),
        tokens_(static_cast<double>(rps)),
        last_refill_(std::chrono::steady_clock::now()) {}

  bool publish(const event_c &event) override {
    if (!running_.load()) {
      return false;
    }

    if (!check_rate_limit()) {
      return false;
    }

    event_c evt = event;
    evt.topic = topic_;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_cv_.wait(lock, [this] { return queue_.size() < max_queue_size_; });

      queue_.push(evt);
    }
    queue_cv_.notify_one();

    return true;
  }

private:
  bool check_rate_limit() {
    std::lock_guard<std::mutex> lock(rate_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - last_refill_)
                       .count();

    if (elapsed > 0) {
      double refill = (elapsed / 1000.0) * rps_;
      tokens_ = std::min(tokens_ + refill, static_cast<double>(rps_));
      last_refill_ = now;
    }

    if (tokens_ >= 1.0) {
      tokens_ -= 1.0;
      return true;
    }

    return false;
  }

  std::string topic_;
  std::size_t rps_;
  std::queue<event_c> &queue_;
  std::mutex &queue_mutex_;
  std::condition_variable &queue_cv_;
  std::size_t max_queue_size_;
  std::atomic<bool> &running_;

  std::mutex rate_mutex_;
  double tokens_;
  std::chrono::steady_clock::time_point last_refill_;
};

struct subscriber_entry_s {
  std::size_t id;
  std::string topic;
  subscriber_if *subscriber;
};

class event_system_c::impl_c {
public:
  explicit impl_c(const options_s &options)
      : options_(options), running_(false), next_subscriber_id_(1) {
    if (!options_.logger) {
      options_.logger = spdlog::default_logger();
    }
  }

  ~impl_c() { stop(); }

  void start() {
    if (running_.load()) {
      return;
    }

    running_.store(true);

    for (std::size_t i = 0; i < options_.num_threads; ++i) {
      workers_.emplace_back([this] { worker_thread(); });
    }
  }

  void stop() {
    if (!running_.load()) {
      return;
    }

    running_.store(false);
    queue_cv_.notify_all();

    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
    workers_.clear();
  }

  std::shared_ptr<publisher_if> get_publisher(const std::string &topic,
                                              std::size_t rps) {
    if (rps == 0 || rps > 4096) {
      return nullptr;
    }

    return std::make_shared<publisher_impl_c>(
        topic, rps, event_queue_, queue_mutex_, queue_cv_,
        options_.max_queue_size, running_);
  }

  std::size_t subscribe(const std::string &topic, subscriber_if *subscriber) {
    if (!subscriber) {
      return 0;
    }

    std::lock_guard<std::mutex> lock(subscribers_mutex_);

    std::size_t id = next_subscriber_id_++;
    subscriber_entry_s entry{id, topic, subscriber};
    subscribers_[id] = entry;

    return id;
  }

  void unsubscribe(std::size_t subscriber_id) {
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    subscribers_.erase(subscriber_id);
  }

private:
  void worker_thread() {
    while (running_.load()) {
      event_c evt;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(
            lock, [this] { return !event_queue_.empty() || !running_.load(); });

        if (!running_.load() && event_queue_.empty()) {
          break;
        }

        if (event_queue_.empty()) {
          continue;
        }

        evt = event_queue_.front();
        event_queue_.pop();
      }

      queue_cv_.notify_one();

      deliver_event(evt);
    }
  }

  void deliver_event(const event_c &evt) {
    std::vector<subscriber_if *> targets;

    {
      std::lock_guard<std::mutex> lock(subscribers_mutex_);
      for (const auto &[id, entry] : subscribers_) {
        if (entry.topic == evt.topic) {
          targets.push_back(entry.subscriber);
        }
      }
    }

    for (auto *subscriber : targets) {
      try {
        subscriber->on_event(evt);
      } catch (...) {
      }
    }
  }

  options_s options_;
  std::atomic<bool> running_;
  std::vector<std::thread> workers_;

  std::queue<event_c> event_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;

  std::unordered_map<std::size_t, subscriber_entry_s> subscribers_;
  std::mutex subscribers_mutex_;
  std::size_t next_subscriber_id_;
};

event_system_c::event_system_c(const options_s &options)
    : impl_(std::make_unique<impl_c>(options)) {}

event_system_c::~event_system_c() = default;

std::shared_ptr<publisher_if>
event_system_c::get_publisher(const std::string &topic, std::size_t rps) {
  return impl_->get_publisher(topic, rps);
}

std::size_t event_system_c::subscribe(const std::string &topic,
                                      subscriber_if *subscriber) {
  return impl_->subscribe(topic, subscriber);
}

void event_system_c::unsubscribe(std::size_t subscriber_id) {
  impl_->unsubscribe(subscriber_id);
}

void event_system_c::start() { impl_->start(); }

void event_system_c::stop() { impl_->stop(); }

} // namespace pkg::events
