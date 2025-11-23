#include <atomic>
#include <chrono>
#include <runtime/events/events.hpp>
#include <runtime/runtime.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <thread>
#include <vector>

using namespace runtime::events;

class test_accessor_c : public runtime::runtime_accessor_if {
public:
  void raise_warning(const char *message) override {}
  void raise_error(const char *message) override {}
};

class test_consumer_c : public event_consumer_if {
public:
  test_consumer_c() : event_count_(0) {}

  void consume_event(const event_s &event) override {
    std::unique_lock<std::mutex> lock(mutex_);
    event_count_++;
    last_event_ = event;
    consumed_events_.push_back(event);
  }

  size_t get_event_count() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return event_count_;
  }

  event_s get_last_event() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return last_event_;
  }

  std::vector<event_s> get_consumed_events() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return consumed_events_;
  }

private:
  mutable std::mutex mutex_;
  std::atomic<size_t> event_count_;
  event_s last_event_;
  std::vector<event_s> consumed_events_;
};

class throwing_consumer_c : public event_consumer_if {
public:
  void consume_event(const event_s &event) override {
    throw std::runtime_error("Test exception");
  }
};

TEST_CASE("event system basic operations", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  SECTION("create and initialize event system") {
    event_system_c event_system(logger.get());
    auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());

    event_system.initialize(accessor);
    CHECK(event_system.is_running());
    CHECK(event_system.get_name() == std::string("event_system_c"));

    event_system.shutdown();
    CHECK_FALSE(event_system.is_running());
  }

  SECTION("create event system with custom thread count") {
    event_system_c event_system(logger.get(), 8, 500);

    auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
    event_system.initialize(accessor);
    CHECK(event_system.is_running());

    event_system.shutdown();
  }
}

TEST_CASE("event producer and topic writer", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  event_system_c event_system(logger.get());
  auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
  event_system.initialize(accessor);

  SECTION("get event producer for category") {
    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);

    CHECK(producer);
    CHECK(producer.get() != nullptr);
  }

  SECTION("get topic writer from producer") {
    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);

    auto writer = producer->get_topic_writer_for_topic(42);
    CHECK(writer);
    CHECK(writer.get() != nullptr);
  }

  event_system.shutdown();
}

TEST_CASE("event consumer registration", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  event_system_c event_system(logger.get());
  auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
  event_system.initialize(accessor);

  SECTION("register single consumer") {
    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(100, consumer);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    CHECK(consumer->get_event_count() == 0);
  }

  SECTION("register multiple consumers for same topic") {
    auto consumer1 = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    auto consumer2 = std::shared_ptr<test_consumer_c>(new test_consumer_c());

    event_system.register_consumer(200, consumer1);
    event_system.register_consumer(200, consumer2);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  SECTION("register consumers for different topics") {
    auto consumer1 = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    auto consumer2 = std::shared_ptr<test_consumer_c>(new test_consumer_c());

    event_system.register_consumer(300, consumer1);
    event_system.register_consumer(301, consumer2);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  event_system.shutdown();
}

TEST_CASE("event publishing and consumption", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  event_system_c event_system(logger.get());
  auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
  event_system.initialize(accessor);

  SECTION("publish and consume single event") {
    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(400, consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer = producer->get_topic_writer_for_topic(400);

    event_s event;
    event.payload = std::string("test payload");
    writer->write_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(consumer->get_event_count() == 1);
    auto last = consumer->get_last_event();
    CHECK(last.category == event_category_e::RUNTIME_EXECUTION_REQUEST);
    CHECK(last.topic_identifier == 400);
  }

  SECTION("publish multiple events") {
    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(401, consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_BACKCHANNEL_A);
    auto writer = producer->get_topic_writer_for_topic(401);

    for (int i = 0; i < 10; ++i) {
      event_s event;
      event.payload = i;
      writer->write_event(event);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(consumer->get_event_count() == 10);
  }

  SECTION("multiple consumers receive same event") {
    auto consumer1 = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    auto consumer2 = std::shared_ptr<test_consumer_c>(new test_consumer_c());

    event_system.register_consumer(402, consumer1);
    event_system.register_consumer(402, consumer2);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_BACKCHANNEL_B);
    auto writer = producer->get_topic_writer_for_topic(402);

    event_s event;
    event.payload = std::string("shared event");
    writer->write_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(consumer1->get_event_count() == 1);
    CHECK(consumer2->get_event_count() == 1);
  }

  SECTION("events to different topics go to correct consumers") {
    auto consumer1 = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    auto consumer2 = std::shared_ptr<test_consumer_c>(new test_consumer_c());

    event_system.register_consumer(403, consumer1);
    event_system.register_consumer(404, consumer2);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer1 = producer->get_topic_writer_for_topic(403);
    auto writer2 = producer->get_topic_writer_for_topic(404);

    event_s event1;
    event1.payload = std::string("topic 403");
    writer1->write_event(event1);

    event_s event2;
    event2.payload = std::string("topic 404");
    writer2->write_event(event2);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(consumer1->get_event_count() == 1);
    CHECK(consumer2->get_event_count() == 1);

    auto last1 = consumer1->get_last_event();
    auto last2 = consumer2->get_last_event();
    CHECK(last1.topic_identifier == 403);
    CHECK(last2.topic_identifier == 404);
  }

  event_system.shutdown();
}

TEST_CASE("multi-threaded event processing", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  event_system_c event_system(logger.get(), 4, 1000);
  auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
  event_system.initialize(accessor);

  SECTION("concurrent event publishing") {
    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(500, consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer = producer->get_topic_writer_for_topic(500);

    std::vector<std::thread> threads;
    const int events_per_thread = 10;
    const int num_threads = 5;

    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&writer, events_per_thread]() {
        for (int i = 0; i < events_per_thread; ++i) {
          event_s event;
          event.payload = i;
          writer->write_event(event);
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(consumer->get_event_count() == events_per_thread * num_threads);
  }

  SECTION("parallel processing with multiple topics") {
    auto consumer1 = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    auto consumer2 = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    auto consumer3 = std::shared_ptr<test_consumer_c>(new test_consumer_c());

    event_system.register_consumer(501, consumer1);
    event_system.register_consumer(502, consumer2);
    event_system.register_consumer(503, consumer3);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer1 = producer->get_topic_writer_for_topic(501);
    auto writer2 = producer->get_topic_writer_for_topic(502);
    auto writer3 = producer->get_topic_writer_for_topic(503);

    std::vector<std::thread> threads;
    const int events_per_writer = 20;

    threads.emplace_back([&writer1, events_per_writer]() {
      for (int i = 0; i < events_per_writer; ++i) {
        event_s event;
        writer1->write_event(event);
      }
    });

    threads.emplace_back([&writer2, events_per_writer]() {
      for (int i = 0; i < events_per_writer; ++i) {
        event_s event;
        writer2->write_event(event);
      }
    });

    threads.emplace_back([&writer3, events_per_writer]() {
      for (int i = 0; i < events_per_writer; ++i) {
        event_s event;
        writer3->write_event(event);
      }
    });

    for (auto &thread : threads) {
      thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(consumer1->get_event_count() == events_per_writer);
    CHECK(consumer2->get_event_count() == events_per_writer);
    CHECK(consumer3->get_event_count() == events_per_writer);
  }

  event_system.shutdown();
}

TEST_CASE("event system error handling", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  event_system_c event_system(logger.get());
  auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
  event_system.initialize(accessor);

  SECTION("exception in consumer does not stop processing") {
    auto throwing_consumer =
        std::shared_ptr<throwing_consumer_c>(new throwing_consumer_c());
    auto normal_consumer =
        std::shared_ptr<test_consumer_c>(new test_consumer_c());

    event_system.register_consumer(600, throwing_consumer);
    event_system.register_consumer(600, normal_consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer = producer->get_topic_writer_for_topic(600);

    event_s event;
    writer->write_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(normal_consumer->get_event_count() == 1);
  }

  SECTION("multiple exceptions handled") {
    auto throwing_consumer =
        std::shared_ptr<throwing_consumer_c>(new throwing_consumer_c());

    event_system.register_consumer(601, throwing_consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer = producer->get_topic_writer_for_topic(601);

    for (int i = 0; i < 5; ++i) {
      event_s event;
      writer->write_event(event);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  event_system.shutdown();
}

TEST_CASE("event system shutdown behavior", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  SECTION("shutdown with empty queue") {
    event_system_c event_system(logger.get());
    auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());

    event_system.initialize(accessor);
    CHECK(event_system.is_running());

    event_system.shutdown();
    CHECK_FALSE(event_system.is_running());
  }

  SECTION("shutdown with pending events") {
    event_system_c event_system(logger.get(), 2, 1000);
    auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
    event_system.initialize(accessor);

    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(700, consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer = producer->get_topic_writer_for_topic(700);

    for (int i = 0; i < 100; ++i) {
      event_s event;
      writer->write_event(event);
    }

    event_system.shutdown();
    CHECK_FALSE(event_system.is_running());
  }

  SECTION("cannot publish after shutdown") {
    event_system_c event_system(logger.get());
    auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
    event_system.initialize(accessor);

    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(701, consumer);

    auto producer = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer = producer->get_topic_writer_for_topic(701);

    event_system.shutdown();

    event_s event;
    writer->write_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(consumer->get_event_count() == 0);
  }
}

TEST_CASE("event system category handling", "[unit][runtime][events]") {
  auto logger = spdlog::get("runtime");
  if (!logger) {
    try {
      logger = spdlog::stdout_color_mt("runtime");
    } catch (const spdlog::spdlog_ex &) {
      logger = spdlog::get("runtime");
    }
  }

  event_system_c event_system(logger.get());
  auto accessor = std::shared_ptr<test_accessor_c>(new test_accessor_c());
  event_system.initialize(accessor);

  SECTION("events maintain correct category") {
    auto consumer = std::shared_ptr<test_consumer_c>(new test_consumer_c());
    event_system.register_consumer(800, consumer);

    auto producer_exec = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_EXECUTION_REQUEST);
    auto writer_exec = producer_exec->get_topic_writer_for_topic(800);

    event_s event1;
    writer_exec->write_event(event1);

    auto producer_backchannel = event_system.get_event_producer_for_category(
        event_category_e::RUNTIME_BACKCHANNEL_A);
    auto writer_backchannel =
        producer_backchannel->get_topic_writer_for_topic(800);

    event_s event2;
    writer_backchannel->write_event(event2);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(consumer->get_event_count() == 2);
    auto events = consumer->get_consumed_events();
    CHECK(events.size() == 2);

    bool found_exec = false;
    bool found_backchannel = false;
    for (const auto &evt : events) {
      if (evt.category == event_category_e::RUNTIME_EXECUTION_REQUEST) {
        found_exec = true;
      }
      if (evt.category == event_category_e::RUNTIME_BACKCHANNEL_A) {
        found_backchannel = true;
      }
    }
    CHECK(found_exec);
    CHECK(found_backchannel);
  }

  event_system.shutdown();
}
