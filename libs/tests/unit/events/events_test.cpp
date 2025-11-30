#include <atomic>
#include <chrono>
#include <events/events.hpp>
#include <map>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

namespace {
pkg::events::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

class test_subscriber_c : public pkg::events::subscriber_if {
public:
  test_subscriber_c() : event_count_(0) {}

  void on_event(const pkg::events::event_c &event) override {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
    event_count_.fetch_add(1);
  }

  std::size_t get_event_count() const { return event_count_.load(); }

  std::vector<pkg::events::event_c> get_events() {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
    event_count_.store(0);
  }

private:
  std::atomic<std::size_t> event_count_;
  std::vector<pkg::events::event_c> events_;
  std::mutex mutex_;
};
} // namespace

TEST_CASE("event system basic initialization", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();
  system.stop();
}

TEST_CASE("event system basic pub/sub", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("test-topic", &subscriber);
  CHECK(sub_id != 0);

  auto publisher = system.get_publisher("test-topic", 100);
  CHECK(publisher != nullptr);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test-data";

  CHECK(publisher->publish(evt));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(subscriber.get_event_count() == 1);

  auto events = subscriber.get_events();
  CHECK(events.size() == 1);
  CHECK(events[0].topic == "test-topic");
  CHECK(events[0].encoded_slp_data == "test-data");

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system multiple subscribers same topic", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c sub1, sub2, sub3;
  auto id1 = system.subscribe("shared-topic", &sub1);
  auto id2 = system.subscribe("shared-topic", &sub2);
  auto id3 = system.subscribe("shared-topic", &sub3);

  auto publisher = system.get_publisher("shared-topic", 100);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "broadcast-message";

  CHECK(publisher->publish(evt));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(sub1.get_event_count() == 1);
  CHECK(sub2.get_event_count() == 1);
  CHECK(sub3.get_event_count() == 1);

  system.unsubscribe(id1);
  system.unsubscribe(id2);
  system.unsubscribe(id3);
  system.stop();
}

TEST_CASE("event system topic isolation", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c sub_topic_a, sub_topic_b;
  auto id_a = system.subscribe("topic-a", &sub_topic_a);
  auto id_b = system.subscribe("topic-b", &sub_topic_b);

  auto pub_a = system.get_publisher("topic-a", 100);
  auto pub_b = system.get_publisher("topic-b", 100);

  pkg::events::event_c evt_a;
  evt_a.encoded_slp_data = "data-for-a";

  pkg::events::event_c evt_b;
  evt_b.encoded_slp_data = "data-for-b";

  CHECK(pub_a->publish(evt_a));
  CHECK(pub_b->publish(evt_b));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(sub_topic_a.get_event_count() == 1);
  CHECK(sub_topic_b.get_event_count() == 1);

  auto events_a = sub_topic_a.get_events();
  CHECK(events_a[0].topic == "topic-a");
  CHECK(events_a[0].encoded_slp_data == "data-for-a");

  auto events_b = sub_topic_b.get_events();
  CHECK(events_b[0].topic == "topic-b");
  CHECK(events_b[0].encoded_slp_data == "data-for-b");

  system.unsubscribe(id_a);
  system.unsubscribe(id_b);
  system.stop();
}

TEST_CASE("event system unsubscribe", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("test-topic", &subscriber);

  auto publisher = system.get_publisher("test-topic", 100);

  pkg::events::event_c evt1;
  evt1.encoded_slp_data = "message-1";
  CHECK(publisher->publish(evt1));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  CHECK(subscriber.get_event_count() == 1);

  system.unsubscribe(sub_id);

  pkg::events::event_c evt2;
  evt2.encoded_slp_data = "message-2";
  CHECK(publisher->publish(evt2));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(subscriber.get_event_count() == 1);

  system.stop();
}

TEST_CASE("event system multiple messages", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("test-topic", &subscriber);

  auto publisher = system.get_publisher("test-topic", 1000);

  const int message_count = 50;
  for (int i = 0; i < message_count; ++i) {
    pkg::events::event_c evt;
    evt.encoded_slp_data = "message-" + std::to_string(i);
    CHECK(publisher->publish(evt));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  CHECK(subscriber.get_event_count() == message_count);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system concurrent publishers", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 4;
  opts.max_queue_size = 500;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("concurrent-topic", &subscriber);

  const int num_publishers = 5;
  const int messages_per_publisher = 20;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_publishers; ++i) {
    threads.emplace_back([&system, i, messages_per_publisher] {
      auto pub = system.get_publisher("concurrent-topic", 1000);
      for (int j = 0; j < messages_per_publisher; ++j) {
        pkg::events::event_c evt;
        evt.encoded_slp_data =
            "pub-" + std::to_string(i) + "-msg-" + std::to_string(j);
        pub->publish(evt);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  CHECK(subscriber.get_event_count() ==
        num_publishers * messages_per_publisher);

  auto events = subscriber.get_events();
  CHECK(events.size() == num_publishers * messages_per_publisher);

  std::map<std::string, int> message_counts;
  for (const auto &evt : events) {
    CHECK(evt.topic == "concurrent-topic");
    CHECK(evt.encoded_slp_data.find("pub-") == 0);
    CHECK(evt.encoded_slp_data.find("-msg-") != std::string::npos);
    message_counts[evt.encoded_slp_data]++;
  }

  for (const auto &[msg, count] : message_counts) {
    CHECK(count == 1);
  }

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system concurrent subscribers", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 4;
  opts.max_queue_size = 200;

  pkg::events::event_system_c system(opts);
  system.start();

  const int num_subscribers = 10;
  std::vector<test_subscriber_c> subscribers(num_subscribers);
  std::vector<std::size_t> sub_ids;

  for (int i = 0; i < num_subscribers; ++i) {
    auto id = system.subscribe("multi-sub-topic", &subscribers[i]);
    sub_ids.push_back(id);
  }

  auto publisher = system.get_publisher("multi-sub-topic", 1000);

  const int message_count = 20;
  for (int i = 0; i < message_count; ++i) {
    pkg::events::event_c evt;
    evt.encoded_slp_data = "message-" + std::to_string(i);
    CHECK(publisher->publish(evt));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  for (int i = 0; i < num_subscribers; ++i) {
    CHECK(subscribers[i].get_event_count() == message_count);

    auto events = subscribers[i].get_events();
    CHECK(events.size() == message_count);

    std::map<std::string, int> message_set;
    for (const auto &evt : events) {
      CHECK(evt.topic == "multi-sub-topic");
      CHECK(evt.encoded_slp_data.find("message-") == 0);
      message_set[evt.encoded_slp_data]++;
    }

    for (int j = 0; j < message_count; ++j) {
      std::string expected = "message-" + std::to_string(j);
      CHECK(message_set[expected] == 1);
    }
  }

  for (auto id : sub_ids) {
    system.unsubscribe(id);
  }
  system.stop();
}

TEST_CASE("event system rate limiting basic", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("rate-limited", &subscriber);

  auto publisher = system.get_publisher("rate-limited", 10);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";

  int successful_publishes = 0;
  for (int i = 0; i < 50; ++i) {
    if (publisher->publish(evt)) {
      successful_publishes++;
    }
  }

  CHECK(successful_publishes >= 10);
  CHECK(successful_publishes <= 12);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system rate limiting over time", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("rate-limited", &subscriber);

  auto publisher = system.get_publisher("rate-limited", 5);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";

  auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < 5; ++i) {
    CHECK(publisher->publish(evt));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1100));

  for (int i = 0; i < 5; ++i) {
    CHECK(publisher->publish(evt));
  }

  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  CHECK(duration >= 1000);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  CHECK(subscriber.get_event_count() == 10);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system rate limiting accurate measurement", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 500;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("rate-test", &subscriber);

  SECTION("50 RPS burst") {
    auto publisher = system.get_publisher("rate-test", 50);

    int successful = 0;
    for (int i = 0; i < 100; ++i) {
      pkg::events::event_c evt;
      evt.encoded_slp_data = "msg_" + std::to_string(i);
      if (publisher->publish(evt)) {
        successful++;
      }
    }

    CHECK(successful >= 50);
    CHECK(successful <= 55);
  }

  SECTION("100 RPS burst") {
    subscriber.clear();
    auto publisher = system.get_publisher("rate-test", 100);

    int successful = 0;
    for (int i = 0; i < 200; ++i) {
      pkg::events::event_c evt;
      evt.encoded_slp_data = "msg_" + std::to_string(i);
      if (publisher->publish(evt)) {
        successful++;
      }
    }

    CHECK(successful >= 100);
    CHECK(successful <= 105);
  }

  SECTION("500 RPS burst") {
    subscriber.clear();
    auto publisher = system.get_publisher("rate-test", 500);

    int successful = 0;
    for (int i = 0; i < 1000; ++i) {
      pkg::events::event_c evt;
      evt.encoded_slp_data = "msg_" + std::to_string(i);
      if (publisher->publish(evt)) {
        successful++;
      }
    }

    CHECK(successful >= 500);
    CHECK(successful <= 510);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(subscriber.get_event_count() >= 500);
    CHECK(subscriber.get_event_count() <= 510);
  }

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system rate limiting burst then sustain", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 200;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("burst-test", &subscriber);

  auto publisher = system.get_publisher("burst-test", 20);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";

  int burst_success = 0;
  for (int i = 0; i < 30; ++i) {
    if (publisher->publish(evt)) {
      burst_success++;
    }
  }

  CHECK(burst_success >= 20);
  CHECK(burst_success <= 22);

  std::this_thread::sleep_for(std::chrono::milliseconds(1100));

  int sustained_success = 0;
  for (int i = 0; i < 30; ++i) {
    if (publisher->publish(evt)) {
      sustained_success++;
    }
  }

  CHECK(sustained_success >= 20);
  CHECK(sustained_success <= 22);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system rate limiting multiple publishers different rates",
          "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 4;
  opts.max_queue_size = 500;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c sub_fast, sub_slow;
  auto id_fast = system.subscribe("fast-topic", &sub_fast);
  auto id_slow = system.subscribe("slow-topic", &sub_slow);

  auto pub_fast = system.get_publisher("fast-topic", 100);
  auto pub_slow = system.get_publisher("slow-topic", 10);

  int fast_success = 0;
  for (int i = 0; i < 200; ++i) {
    pkg::events::event_c evt;
    evt.encoded_slp_data = "fast_" + std::to_string(i);
    if (pub_fast->publish(evt)) {
      fast_success++;
    }
  }

  int slow_success = 0;
  for (int i = 0; i < 30; ++i) {
    pkg::events::event_c evt;
    evt.encoded_slp_data = "slow_" + std::to_string(i);
    if (pub_slow->publish(evt)) {
      slow_success++;
    }
  }

  CHECK(fast_success >= 100);
  CHECK(fast_success <= 105);

  CHECK(slow_success >= 10);
  CHECK(slow_success <= 12);

  system.unsubscribe(id_fast);
  system.unsubscribe(id_slow);
  system.stop();
}

TEST_CASE("event system invalid rps values", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  SECTION("rps = 0") {
    auto publisher = system.get_publisher("test", 0);
    CHECK(publisher == nullptr);
  }

  SECTION("rps > 4096") {
    auto publisher = system.get_publisher("test", 5000);
    CHECK(publisher == nullptr);
  }

  SECTION("rps = 4096 is valid") {
    auto publisher = system.get_publisher("test", 4096);
    CHECK(publisher != nullptr);
  }

  SECTION("rps = 1 is valid") {
    auto publisher = system.get_publisher("test", 1);
    CHECK(publisher != nullptr);
  }

  system.stop();
}

TEST_CASE("event system nullptr subscriber", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  auto sub_id = system.subscribe("test-topic", nullptr);
  CHECK(sub_id == 0);

  system.stop();
}

TEST_CASE("event system stop without start", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.stop();
}

TEST_CASE("event system multiple start/stop cycles", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);

  system.start();
  system.stop();

  system.start();
  system.stop();

  system.start();
  system.stop();
}

TEST_CASE("event system publish when stopped", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("test-topic", &subscriber);

  auto publisher = system.get_publisher("test-topic", 100);

  system.stop();

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";

  CHECK_FALSE(publisher->publish(evt));

  system.unsubscribe(sub_id);
}

TEST_CASE("event system stress test", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 8;
  opts.max_queue_size = 1000;

  pkg::events::event_system_c system(opts);
  system.start();

  const int num_topics = 5;
  const int num_subscribers_per_topic = 3;
  const int num_messages = 100;

  std::vector<std::vector<std::unique_ptr<test_subscriber_c>>> all_subscribers(
      num_topics);
  std::vector<std::vector<std::size_t>> all_sub_ids(num_topics);

  for (int t = 0; t < num_topics; ++t) {
    std::string topic = "topic-" + std::to_string(t);

    for (int s = 0; s < num_subscribers_per_topic; ++s) {
      auto sub = std::make_unique<test_subscriber_c>();
      auto id = system.subscribe(topic, sub.get());
      all_sub_ids[t].push_back(id);
      all_subscribers[t].push_back(std::move(sub));
    }
  }

  std::vector<std::thread> pub_threads;
  for (int t = 0; t < num_topics; ++t) {
    pub_threads.emplace_back([&system, t, num_messages] {
      std::string topic = "topic-" + std::to_string(t);
      auto pub = system.get_publisher(topic, 2000);

      for (int m = 0; m < num_messages; ++m) {
        pkg::events::event_c evt;
        evt.encoded_slp_data = "msg-" + std::to_string(m);
        pub->publish(evt);
      }
    });
  }

  for (auto &t : pub_threads) {
    t.join();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  for (int t = 0; t < num_topics; ++t) {
    for (int s = 0; s < num_subscribers_per_topic; ++s) {
      CHECK(all_subscribers[t][s]->get_event_count() == num_messages);
    }
  }

  for (int t = 0; t < num_topics; ++t) {
    for (auto id : all_sub_ids[t]) {
      system.unsubscribe(id);
    }
  }

  system.stop();
}

TEST_CASE("event system empty topic name", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("", &subscriber);
  CHECK(sub_id != 0);

  auto publisher = system.get_publisher("", 100);
  CHECK(publisher != nullptr);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";

  CHECK(publisher->publish(evt));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(subscriber.get_event_count() == 1);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system unsubscribe invalid id", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  system.unsubscribe(0);
  system.unsubscribe(999999);

  system.stop();
}

TEST_CASE("event system double start", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("test", &subscriber);
  auto pub = system.get_publisher("test", 100);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";
  CHECK(pub->publish(evt));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(subscriber.get_event_count() == 1);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system rate limit boundary", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("boundary", &subscriber);

  auto publisher = system.get_publisher("boundary", 4096);
  CHECK(publisher != nullptr);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "test";

  int successful = 0;
  for (int i = 0; i < 100; ++i) {
    if (publisher->publish(evt)) {
      successful++;
    }
  }

  CHECK(successful <= 100);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system data integrity in concurrent scenario",
          "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 4;
  opts.max_queue_size = 500;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("data-integrity", &subscriber);

  const int num_publishers = 3;
  const int messages_per_publisher = 50;
  std::vector<std::thread> threads;

  for (int p = 0; p < num_publishers; ++p) {
    threads.emplace_back([&system, p, messages_per_publisher] {
      auto pub = system.get_publisher("data-integrity", 1000);
      for (int m = 0; m < messages_per_publisher; ++m) {
        pkg::events::event_c evt;
        evt.encoded_slp_data = "publisher_" + std::to_string(p) + "_message_" +
                               std::to_string(m) + "_data";
        pub->publish(evt);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  CHECK(subscriber.get_event_count() ==
        num_publishers * messages_per_publisher);

  auto events = subscriber.get_events();
  CHECK(events.size() == num_publishers * messages_per_publisher);

  for (const auto &evt : events) {
    CHECK(evt.topic == "data-integrity");
    CHECK(!evt.encoded_slp_data.empty());
    CHECK(evt.encoded_slp_data.find("publisher_") == 0);
    CHECK(evt.encoded_slp_data.find("_message_") != std::string::npos);
    CHECK(evt.encoded_slp_data.find("_data") != std::string::npos);
  }

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system large payload", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("large-payload", &subscriber);

  auto publisher = system.get_publisher("large-payload", 100);

  std::string large_data(1024 * 100, 'X');
  large_data += "_MARKER_END";

  pkg::events::event_c evt;
  evt.encoded_slp_data = large_data;

  CHECK(publisher->publish(evt));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(subscriber.get_event_count() == 1);

  auto events = subscriber.get_events();
  CHECK(events.size() == 1);
  CHECK(events[0].encoded_slp_data.size() == large_data.size());
  CHECK(events[0].encoded_slp_data == large_data);
  CHECK(events[0].encoded_slp_data.find("_MARKER_END") != std::string::npos);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system special characters and unicode", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("special-chars", &subscriber);

  auto publisher = system.get_publisher("special-chars", 100);

  SECTION("newlines and tabs") {
    pkg::events::event_c evt;
    evt.encoded_slp_data = "line1\nline2\ttabbed\rcarriage";
    CHECK(publisher->publish(evt));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto events = subscriber.get_events();
    CHECK(events.size() == 1);
    CHECK(events[0].encoded_slp_data == "line1\nline2\ttabbed\rcarriage");
  }

  SECTION("special symbols") {
    subscriber.clear();
    pkg::events::event_c evt;
    evt.encoded_slp_data = "!@#$%^&*()_+-=[]{}|;':\",./<>?`~";
    CHECK(publisher->publish(evt));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto events = subscriber.get_events();
    CHECK(events.size() == 1);
    CHECK(events[0].encoded_slp_data == "!@#$%^&*()_+-=[]{}|;':\",./<>?`~");
  }

  SECTION("unicode characters") {
    subscriber.clear();
    pkg::events::event_c evt;
    evt.encoded_slp_data = "Hello 世界 🚀 café";
    CHECK(publisher->publish(evt));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto events = subscriber.get_events();
    CHECK(events.size() == 1);
    CHECK(events[0].encoded_slp_data == "Hello 世界 🚀 café");
  }

  SECTION("null bytes") {
    subscriber.clear();
    pkg::events::event_c evt;
    std::string data_with_nulls = "before";
    data_with_nulls += '\0';
    data_with_nulls += "after";
    evt.encoded_slp_data = data_with_nulls;
    CHECK(publisher->publish(evt));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto events = subscriber.get_events();
    CHECK(events.size() == 1);
    CHECK(events[0].encoded_slp_data.size() == data_with_nulls.size());
  }

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system message ordering within topic", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 1;
  opts.max_queue_size = 200;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("ordered", &subscriber);

  auto publisher = system.get_publisher("ordered", 1000);

  const int num_messages = 100;
  for (int i = 0; i < num_messages; ++i) {
    pkg::events::event_c evt;
    evt.encoded_slp_data = std::to_string(i);
    CHECK(publisher->publish(evt));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  CHECK(subscriber.get_event_count() == num_messages);

  auto events = subscriber.get_events();
  CHECK(events.size() == num_messages);

  for (int i = 0; i < num_messages; ++i) {
    CHECK(events[i].encoded_slp_data == std::to_string(i));
    CHECK(events[i].topic == "ordered");
  }

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system queue blocking behavior", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 1;
  opts.max_queue_size = 10;

  pkg::events::event_system_c system(opts);
  system.start();

  std::atomic<int> received_count{0};

  class slow_subscriber_c : public pkg::events::subscriber_if {
  public:
    explicit slow_subscriber_c(std::atomic<int> *counter) : counter_(counter) {}
    void on_event(const pkg::events::event_c &event) override {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      counter_->fetch_add(1);
    }

  private:
    std::atomic<int> *counter_;
  };

  slow_subscriber_c slow_sub(&received_count);
  auto sub_id = system.subscribe("blocking", &slow_sub);

  auto publisher = system.get_publisher("blocking", 1000);

  std::thread publisher_thread([&publisher]() {
    for (int i = 0; i < 20; ++i) {
      pkg::events::event_c evt;
      evt.encoded_slp_data = "msg_" + std::to_string(i);
      publisher->publish(evt);
    }
  });

  publisher_thread.join();

  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  CHECK(received_count.load() == 20);

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system empty data is valid", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 2;
  opts.max_queue_size = 100;

  pkg::events::event_system_c system(opts);
  system.start();

  test_subscriber_c subscriber;
  auto sub_id = system.subscribe("empty-data", &subscriber);

  auto publisher = system.get_publisher("empty-data", 100);

  pkg::events::event_c evt;
  evt.encoded_slp_data = "";

  CHECK(publisher->publish(evt));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(subscriber.get_event_count() == 1);

  auto events = subscriber.get_events();
  CHECK(events.size() == 1);
  CHECK(events[0].encoded_slp_data.empty());
  CHECK(events[0].topic == "empty-data");

  system.unsubscribe(sub_id);
  system.stop();
}

TEST_CASE("event system multiple topics data isolation", "[unit][events]") {
  pkg::events::options_s opts;
  opts.logger = create_test_logger();
  opts.num_threads = 4;
  opts.max_queue_size = 200;

  pkg::events::event_system_c system(opts);
  system.start();

  std::vector<test_subscriber_c> subscribers(3);
  std::vector<std::string> topics = {"topic_A", "topic_B", "topic_C"};

  for (size_t i = 0; i < topics.size(); ++i) {
    system.subscribe(topics[i], &subscribers[i]);
  }

  std::vector<std::thread> threads;
  for (size_t t = 0; t < topics.size(); ++t) {
    threads.emplace_back([&system, &topics, t] {
      auto pub = system.get_publisher(topics[t], 500);
      for (int m = 0; m < 30; ++m) {
        pkg::events::event_c evt;
        evt.encoded_slp_data =
            topics[t] + "_message_" + std::to_string(m) + "_content";
        pub->publish(evt);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  for (size_t i = 0; i < subscribers.size(); ++i) {
    CHECK(subscribers[i].get_event_count() == 30);

    auto events = subscribers[i].get_events();
    for (const auto &evt : events) {
      CHECK(evt.topic == topics[i]);
      CHECK(evt.encoded_slp_data.find(topics[i]) == 0);
      CHECK(evt.encoded_slp_data.find("_message_") != std::string::npos);
      CHECK(evt.encoded_slp_data.find("_content") != std::string::npos);
    }
  }

  system.stop();
}
