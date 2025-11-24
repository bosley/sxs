#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
#include <record/record.hpp>
#include <runtime/entity/entity.hpp>
#include <runtime/events/events.hpp>
#include <runtime/processor.hpp>
#include <runtime/runtime.hpp>
#include <runtime/session/session.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <thread>

namespace {
void ensure_db_cleanup(const std::string &path) {
  std::filesystem::remove_all(path);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

std::string get_unique_test_path(const std::string &base) {
  static std::atomic<int> counter{0};
  return base + "_" + std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count());
}

auto create_test_logger() {
  auto logger = spdlog::get("multi_processor_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("multi_processor_test");
  }
  return logger;
}

class test_accessor_c : public runtime::runtime_accessor_if {
public:
  void raise_warning(const char *message) override {}
  void raise_error(const char *message) override {}
};

class counting_consumer_c : public runtime::events::event_consumer_if {
public:
  void consume_event(const runtime::events::event_s &event) override {
    if (event.category ==
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST) {
      count_.fetch_add(1);
    }
  }

  int get_count() const { return count_.load(); }

private:
  std::atomic<int> count_{0};
};

} // namespace

TEST_CASE("multi processor initialization with 1 processor",
          "[unit][runtime][multi_processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  std::vector<std::unique_ptr<runtime::processor_c>> processors;
  size_t num_processors = 1;

  for (size_t i = 0; i < num_processors; i++) {
    auto processor =
        std::make_unique<runtime::processor_c>(logger.get(), &event_system);
    auto processor_consumer =
        std::shared_ptr<runtime::events::event_consumer_if>(
            processor.get(), [](runtime::events::event_consumer_if *) {});
    event_system.register_consumer(static_cast<std::uint16_t>(i),
                                    processor_consumer);
    processors.push_back(std::move(processor));
  }

  CHECK(processors.size() == 1);

  event_system.shutdown();
}

TEST_CASE("multi processor initialization with 4 processors",
          "[unit][runtime][multi_processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  std::vector<std::unique_ptr<runtime::processor_c>> processors;
  size_t num_processors = 4;

  for (size_t i = 0; i < num_processors; i++) {
    auto processor =
        std::make_unique<runtime::processor_c>(logger.get(), &event_system);
    auto processor_consumer =
        std::shared_ptr<runtime::events::event_consumer_if>(
            processor.get(), [](runtime::events::event_consumer_if *) {});
    event_system.register_consumer(static_cast<std::uint16_t>(i),
                                    processor_consumer);
    processors.push_back(std::move(processor));
  }

  CHECK(processors.size() == 4);

  event_system.shutdown();
}

TEST_CASE("multi processor concurrent execution on different topics",
          "[unit][runtime][multi_processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 1000);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/multi_processor_concurrent");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/multi_processor_concurrent_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);

  std::vector<std::unique_ptr<runtime::processor_c>> processors;
  std::vector<std::unique_ptr<runtime::session_c>> sessions;
  std::vector<std::unique_ptr<runtime::entity_c>> entities;
  size_t num_processors = 4;

  for (size_t i = 0; i < num_processors; i++) {
    auto processor =
        std::make_unique<runtime::processor_c>(logger.get(), &event_system);
    auto processor_consumer =
        std::shared_ptr<runtime::events::event_consumer_if>(
            processor.get(), [](runtime::events::event_consumer_if *) {});
    event_system.register_consumer(static_cast<std::uint16_t>(i),
                                    processor_consumer);
    processors.push_back(std::move(processor));

    auto entity_opt =
        entity_manager.get_or_create<runtime::entity_c>("user" +
                                                        std::to_string(i));
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());

    auto session = std::make_unique<runtime::session_c>(
        "session_" + std::to_string(i), "user" + std::to_string(i), "scope_" + std::to_string(i),
        entity.get(), &data_ds, &event_system);

    sessions.push_back(std::move(session));
    entities.push_back(std::move(entity));
  }

  std::atomic<int> completed_count{0};
  std::vector<bool> completed(num_processors, false);

  for (size_t i = 0; i < num_processors; i++) {
    std::thread([&, i]() {
      runtime::execution_request_s request;
      request.session = sessions[i].get();
      request.script_text =
          "[" + std::to_string(i * 100) + " " + std::to_string(i * 100 + 50) + "]";
      request.request_id = "req_" + std::to_string(i);

      runtime::events::event_s event;
      event.category =
          runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
      event.topic_identifier = static_cast<std::uint16_t>(i);
      event.payload = request;

      auto producer = event_system.get_event_producer_for_category(
          runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST);
      auto writer = producer->get_topic_writer_for_topic(
          static_cast<std::uint16_t>(i));
      writer->write_event(event);

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      completed[i] = true;
      completed_count.fetch_add(1);
    }).detach();
  }

  auto start = std::chrono::steady_clock::now();
  while (completed_count.load() < static_cast<int>(num_processors)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(5)) {
      break;
    }
  }

  CHECK(completed_count.load() == static_cast<int>(num_processors));

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("multi processor topic isolation",
          "[unit][runtime][multi_processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 1000);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  std::vector<std::shared_ptr<counting_consumer_c>> counters;
  size_t num_processors = 4;

  for (size_t i = 0; i < num_processors; i++) {
    auto counter = std::make_shared<counting_consumer_c>();
    event_system.register_consumer(static_cast<std::uint16_t>(i), counter);
    counters.push_back(counter);
  }

  runtime::events::event_s event;
  event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 2;

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST);
  auto writer = producer->get_topic_writer_for_topic(2);
  writer->write_event(event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(counters[0]->get_count() == 0);
  CHECK(counters[1]->get_count() == 0);
  CHECK(counters[2]->get_count() == 1);
  CHECK(counters[3]->get_count() == 0);

  event_system.shutdown();
}

TEST_CASE("multi processor with kv operations on different scopes",
          "[unit][runtime][multi_processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 1000);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/multi_processor_kv_scopes");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/multi_processor_kv_scopes_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);

  std::vector<std::unique_ptr<runtime::processor_c>> processors;
  std::vector<std::unique_ptr<runtime::session_c>> sessions;
  std::vector<std::unique_ptr<runtime::entity_c>> entities;
  size_t num_processors = 3;

  for (size_t i = 0; i < num_processors; i++) {
    auto processor =
        std::make_unique<runtime::processor_c>(logger.get(), &event_system);
    auto processor_consumer =
        std::shared_ptr<runtime::events::event_consumer_if>(
            processor.get(), [](runtime::events::event_consumer_if *) {});
    event_system.register_consumer(static_cast<std::uint16_t>(i),
                                    processor_consumer);
    processors.push_back(std::move(processor));

    auto entity_opt =
        entity_manager.get_or_create<runtime::entity_c>("kv_user" +
                                                        std::to_string(i));
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("kv_scope_" + std::to_string(i), 
                            runtime::permission::READ_WRITE);
    entity->save();

    auto session = std::make_unique<runtime::session_c>(
        "kv_session_" + std::to_string(i), "kv_user" + std::to_string(i),
        "kv_scope_" + std::to_string(i), entity.get(), &data_ds, &event_system);

    sessions.push_back(std::move(session));
    entities.push_back(std::move(entity));
  }

  std::atomic<int> set_count{0};

  for (size_t i = 0; i < num_processors; i++) {
    std::thread([&, i]() {
      runtime::execution_request_s request;
      request.session = sessions[i].get();
      request.script_text =
          "[(kv/set \"key\" \"value_" + std::to_string(i) + "\")]";
      request.request_id = "kv_req_" + std::to_string(i);

      runtime::events::event_s event;
      event.category =
          runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
      event.topic_identifier = static_cast<std::uint16_t>(i);
      event.payload = request;

      auto producer = event_system.get_event_producer_for_category(
          runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST);
      auto writer = producer->get_topic_writer_for_topic(
          static_cast<std::uint16_t>(i));
      writer->write_event(event);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      set_count.fetch_add(1);
    }).detach();
  }

  auto start = std::chrono::steady_clock::now();
  while (set_count.load() < static_cast<int>(num_processors)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(5)) {
      break;
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  for (size_t i = 0; i < num_processors; i++) {
    std::string value;
    bool success = sessions[i]->get_store()->get("key", value);
    CHECK(success);
    CHECK(value == "value_" + std::to_string(i));
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("multi processor stress test with rapid concurrent requests",
          "[unit][runtime][multi_processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 8, 2000);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/multi_processor_stress");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/multi_processor_stress_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);

  std::vector<std::unique_ptr<runtime::processor_c>> processors;
  std::vector<std::unique_ptr<runtime::session_c>> sessions;
  std::vector<std::unique_ptr<runtime::entity_c>> entities;
  size_t num_processors = 4;
  size_t requests_per_processor = 10;

  for (size_t i = 0; i < num_processors; i++) {
    auto processor =
        std::make_unique<runtime::processor_c>(logger.get(), &event_system);
    auto processor_consumer =
        std::shared_ptr<runtime::events::event_consumer_if>(
            processor.get(), [](runtime::events::event_consumer_if *) {});
    event_system.register_consumer(static_cast<std::uint16_t>(i),
                                    processor_consumer);
    processors.push_back(std::move(processor));

    auto entity_opt =
        entity_manager.get_or_create<runtime::entity_c>("stress_user" +
                                                        std::to_string(i));
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());

    auto session = std::make_unique<runtime::session_c>(
        "stress_session_" + std::to_string(i), "stress_user" + std::to_string(i),
        "stress_scope_" + std::to_string(i), entity.get(), &data_ds,
        &event_system);

    sessions.push_back(std::move(session));
    entities.push_back(std::move(entity));
  }

  std::atomic<int> sent_count{0};

  for (size_t i = 0; i < num_processors; i++) {
    for (size_t j = 0; j < requests_per_processor; j++) {
      std::thread([&, i, j]() {
        runtime::execution_request_s request;
        request.session = sessions[i].get();
        request.script_text = "[" + std::to_string(i * 1000 + j) + "]";
        request.request_id =
            "stress_req_" + std::to_string(i) + "_" + std::to_string(j);

        runtime::events::event_s event;
        event.category =
            runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
        event.topic_identifier = static_cast<std::uint16_t>(i);
        event.payload = request;

        auto producer = event_system.get_event_producer_for_category(
            runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST);
        auto writer = producer->get_topic_writer_for_topic(
            static_cast<std::uint16_t>(i));
        writer->write_event(event);

        sent_count.fetch_add(1);
      }).detach();
    }
  }

  auto start = std::chrono::steady_clock::now();
  size_t expected_total = num_processors * requests_per_processor;
  while (sent_count.load() < static_cast<int>(expected_total)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(10)) {
      break;
    }
  }

  CHECK(sent_count.load() == static_cast<int>(expected_total));

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}


