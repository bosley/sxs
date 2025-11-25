#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
#include <record/record.hpp>
#include <runtime/entity/entity.hpp>
#include <runtime/events/events.hpp>
#include <runtime/processor.hpp>
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
  auto logger = spdlog::get("processor_kv_atomic_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("processor_kv_atomic_test");
  }
  return logger;
}

class test_accessor_c : public runtime::runtime_accessor_if {
public:
  void raise_warning(const char *message) override {}
  void raise_error(const char *message) override {}
};

runtime::session_c *
create_test_session(runtime::events::event_system_c &event_system,
                    kvds::datastore_c &data_ds, runtime::entity_c *entity) {
  return new runtime::session_c("test_session", "test_entity", "test_scope",
                                *entity, &data_ds, &event_system);
}
} // namespace

TEST_CASE("core/kv/snx sets key only if not exists",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_snx");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_snx_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("snx succeeds on new key") {
    runtime::execution_request_s request{
        *session, "(core/kv/snx newkey \"newvalue\")", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string value;
    CHECK(session->get_store()->get("newkey", value));
    CHECK(value == "newvalue");
  }

  SECTION("snx fails on existing key") {
    session->get_store()->set("existingkey", "original");

    runtime::execution_request_s request{
        *session, "(core/kv/snx existingkey \"newvalue\")", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string value;
    CHECK(session->get_store()->get("existingkey", value));
    CHECK(value == "original");
  }

  SECTION("snx with evaluated value from kv/get") {
    session->get_store()->set("source", "123");

    runtime::execution_request_s request{
        *session, "(core/kv/snx calckey (core/kv/get source))", "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string value;
    CHECK(session->get_store()->get("calckey", value));
    CHECK(value == "123");
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("core/kv/cas compares and swaps atomically",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_cas");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_cas_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("cas succeeds with correct expected value") {
    session->get_store()->set("counter", "10");

    runtime::execution_request_s request{
        *session, "(core/kv/cas counter \"10\" \"11\")", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string value;
    CHECK(session->get_store()->get("counter", value));
    CHECK(value == "11");
  }

  SECTION("cas fails with incorrect expected value") {
    session->get_store()->set("counter", "10");

    runtime::execution_request_s request{
        *session, "(core/kv/cas counter \"5\" \"11\")", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string value;
    CHECK(session->get_store()->get("counter", value));
    CHECK(value == "10");
  }

  SECTION("cas fails on non-existent key") {
    runtime::execution_request_s request{
        *session, "(core/kv/cas nokey \"anything\" \"newvalue\")", "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK_FALSE(session->get_store()->exists("nokey"));
  }

  SECTION("cas with evaluated values from kv/get") {
    session->get_store()->set("value", "100");
    session->get_store()->set("newvalue", "150");

    runtime::execution_request_s request{
        *session,
        "(core/kv/cas value (core/kv/get value) (core/kv/get newvalue))",
        "req4"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string value;
    CHECK(session->get_store()->get("value", value));
    CHECK(value == "150");
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("core/kv/iterate processes keys with prefix",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_iterate");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_iterate_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("iterate executes handler with $key binding") {
    session->get_store()->set("user:1", "alice");
    session->get_store()->set("user:2", "bob");
    session->get_store()->set("user:3", "charlie");
    session->get_store()->set("other:1", "data");

    runtime::execution_request_s request{*session,
                                         R"((core/kv/iterate user: 0 10 {
      (core/kv/set processed $key)
    }))",
                                         "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string lastProcessed;
    CHECK(session->get_store()->get("processed", lastProcessed));
    CHECK((lastProcessed == "user:1" || lastProcessed == "user:2" ||
           lastProcessed == "user:3"));
    CHECK_FALSE(session->get_store()->exists("copy_other:1"));
  }

  SECTION("iterate respects offset and limit") {
    session->get_store()->set("item:01", "a");
    session->get_store()->set("item:02", "b");
    session->get_store()->set("item:03", "c");
    session->get_store()->set("item:04", "d");
    session->get_store()->set("item:05", "e");
    session->get_store()->set("visit_count", "0");

    runtime::execution_request_s request{*session,
                                         R"((core/kv/iterate item: 1 2 {
      (core/kv/set last_visited $key)
      (core/kv/set visit_count (core/expr/eval (core/kv/get visit_count)))
    }))",
                                         "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string lastVisited;
    CHECK(session->get_store()->get("last_visited", lastVisited));
    CHECK((lastVisited == "item:02" || lastVisited == "item:03"));
  }

  SECTION("iterate stops on handler error") {
    session->get_store()->set("data:1", "x");
    session->get_store()->set("data:2", "y");
    session->get_store()->set("data:3", "z");

    runtime::execution_request_s request{*session,
                                         R"((core/kv/iterate data: 0 10 {
      (core/kv/set visited $key)
      (core/unknown/function)
      (core/kv/set should_not_reach "true")
    }))",
                                         "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(session->get_store()->exists("visited"));
    CHECK_FALSE(session->get_store()->exists("should_not_reach"));
  }

  SECTION("iterate with multiple operations in handler") {
    session->get_store()->set("num:1", "10");
    session->get_store()->set("num:2", "20");
    session->get_store()->set("num:3", "30");

    runtime::execution_request_s request{*session,
                                         R"((core/kv/iterate num: 0 10 {
      (core/kv/set last_iterated_key $key)
      (core/kv/exists $key)
    }))",
                                         "req4"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string lastKey;
    CHECK(session->get_store()->get("last_iterated_key", lastKey));
    CHECK((lastKey == "num:1" || lastKey == "num:2" || lastKey == "num:3"));
  }


  SECTION("iterate with zero limit processes no items") {
    session->get_store()->set("zero:1", "a");
    session->get_store()->set("zero:2", "b");

    runtime::execution_request_s request{*session,
                                         R"((core/kv/iterate zero: 0 0 {
      (core/kv/set should_not_create "true")
    }))",
                                         "req6"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK_FALSE(session->get_store()->exists("should_not_create"));
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}
