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
  auto logger = spdlog::get("processor_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("processor_test");
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

TEST_CASE("processor initialization", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  SECTION("processor can be created") {
    runtime::processor_c processor(logger.get(), event_system);
  }

  event_system.shutdown();
}

TEST_CASE("processor execute simple integer script",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_simple");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_simple_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("evaluate integer literal") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "42", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("evaluate real literal") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "3.14", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("evaluate string literal") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "\"hello world\"", "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/kv/set and core/kv/get operations",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path = get_unique_test_path("/tmp/processor_test_kv");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_entity");
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

  SECTION("core/kv/set stores value") {
    runtime::execution_request_s request{
        *session, "(core/kv/set mykey \"myvalue\")", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::string value;
    CHECK(session->get_store()->get("mykey", value));
    CHECK(value == "myvalue");
  }

  SECTION("core/kv/get retrieves value") {
    session->get_store()->set("testkey", "testvalue");

    runtime::execution_request_s request{*session, "(core/kv/get testkey)",
                                         "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("core/kv/set with integer value") {
    runtime::execution_request_s request{*session, "(core/kv/set counter 42)",
                                         "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::string value;
    CHECK(session->get_store()->get("counter", value));
    CHECK(value == "42");
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/kv/del and core/kv/exists operations",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_del");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_del_entity");
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

  SECTION("core/kv/del removes value") {
    session->get_store()->set("deletekey", "deleteme");
    CHECK(session->get_store()->exists("deletekey"));

    runtime::execution_request_s request{*session, "(core/kv/del deletekey)",
                                         "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    CHECK_FALSE(session->get_store()->exists("deletekey"));
  }

  SECTION("core/kv/exists checks existence") {
    session->get_store()->set("existkey", "value");

    runtime::execution_request_s request{*session, "(core/kv/exists existkey)",
                                         "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/event/pub operation", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_event_pub");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_event_pub_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("core/event/pub publishes event") {
    runtime::execution_request_s request{
        *session, "(core/event/pub $CHANNEL_A 100 \"test message\")", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("core/event/pub with integer data") {
    runtime::execution_request_s request{
        *session, "(core/event/pub $CHANNEL_A 100 42)", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/event/sub operation", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_event_sub");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_event_sub_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_topic_permission(200, runtime::topic_permission::SUBSCRIBE);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("core/event/sub subscribes to topic") {
    runtime::execution_request_s request{
        *session, "(core/event/sub $CHANNEL_A 200 :str {})", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/util/log operation", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path = get_unique_test_path("/tmp/processor_test_log");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_log_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("core/util/log with single string") {
    runtime::execution_request_s request{
        *session, "(core/util/log \"Hello from SLP\")", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("core/util/log with multiple arguments") {
    runtime::execution_request_s request{
        *session, "(core/util/log \"Count:\" 42 \"Done\")", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor error handling", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_error");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_error_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("parse error is handled") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "(unclosed paren", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("unknown function is handled") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{
        *session, "(unknown/function arg1 arg2)", "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor permission denied scenarios",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_permission");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_permission_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("core/kv/set without permission fails") {
    runtime::execution_request_s request{*session,
                                         "(core/kv/set key \"value\")", "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    CHECK_FALSE(session->get_store()->exists("key"));
  }

  SECTION("core/event/pub without permission fails") {
    runtime::execution_request_s request{
        *session, "(core/event/pub $CHANNEL_A 100 \"message\")", "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor bracket list execution", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_bracket");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_bracket_entity");
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

  SECTION("bracket list executes multiple statements") {
    runtime::execution_request_s request{
        *session,
        "[(core/kv/set key1 \"value1\") (core/kv/set key2 "
        "\"value2\") (core/kv/set key3 "
        "\"value3\")]",
        "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    CHECK(session->get_store()->exists("key1"));
    CHECK(session->get_store()->exists("key2"));
    CHECK(session->get_store()->exists("key3"));
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor complex script execution", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_complex");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_complex_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(100, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("complex script with multiple operations") {
    runtime::execution_request_s request{*session, R"([
      (core/kv/set user_name "Alice")
      (core/kv/set user_age 30)
      (core/util/log "User created:" (core/kv/get user_name))
      (core/event/sub $CHANNEL_A 100)
      (core/event/pub $CHANNEL_A 100 "User Alice logged in")
    ])",
                                         "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::string name;
    CHECK(session->get_store()->get("user_name", name));
    CHECK(name == "Alice");

    std::string age;
    CHECK(session->get_store()->get("user_age", age));
    CHECK(age == "30");
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/expr/eval operation", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_eval");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_eval_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("evaluate simple integer literal") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "(core/expr/eval \"42\")",
                                         "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("evaluate expression from core/kv/get") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, R"([
      (core/kv/set x "100")
      (core/expr/eval (core/kv/get x))
    ])",
                                         "req2"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("evaluate nested function call") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{
        *session, "(core/expr/eval \"(core/util/log hello-world)\")", "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("error handling for parse errors") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{
        *session, "(core/expr/eval \"(broken syntax\")", "req4"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor core/expr/eval with kv operations",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_eval_kv");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_eval_kv_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("store result of eval in kv") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, R"===([
      (core/kv/set script "(core/kv/set computed 999)")
      (core/expr/eval (core/kv/get script))
      (core/kv/get computed)
    ])===",
                                         "req1"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::string computed;
    CHECK(session->get_store()->get("computed", computed));
    CHECK(computed == "999");

    delete session;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}


TEST_CASE("processor ignores wrong category events",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_wrong_category");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_wrong_category_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("processor ignores BACKCHANNEL_A events") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "42", "req1"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    delete session;
  }

  SECTION("processor ignores BACKCHANNEL_B events") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "100", "req2"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_BACKCHANNEL_B;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    delete session;
  }

  SECTION("processor still processes RUNTIME_EXECUTION_REQUEST") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "42", "req3"};

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    delete session;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}
