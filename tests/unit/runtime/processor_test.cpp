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
                                entity, &data_ds, &event_system);
}
} // namespace

TEST_CASE("processor initialization", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  SECTION("processor can be created") {
    runtime::processor_c processor(logger.get(), &event_system);
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

  runtime::processor_c processor(logger.get(), &event_system);

  SECTION("evaluate integer literal") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "42";
    request.request_id = "req1";

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

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "3.14";
    request.request_id = "req2";

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

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "\"hello world\"";
    request.request_id = "req3";

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

TEST_CASE("processor kv/set and kv/get operations",
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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("kv/set stores value") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(kv/set mykey \"myvalue\")";
    request.request_id = "req1";

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

  SECTION("kv/get retrieves value") {
    session->get_store()->set("testkey", "testvalue");

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(kv/get testkey)";
    request.request_id = "req2";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("kv/set with integer value") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(kv/set counter 42)";
    request.request_id = "req3";

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

TEST_CASE("processor kv/del and kv/exists operations",
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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("kv/del removes value") {
    session->get_store()->set("deletekey", "deleteme");
    CHECK(session->get_store()->exists("deletekey"));

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(kv/del deletekey)";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    CHECK_FALSE(session->get_store()->exists("deletekey"));
  }

  SECTION("kv/exists checks existence") {
    session->get_store()->set("existkey", "value");

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(kv/exists existkey)";
    request.request_id = "req2";

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

TEST_CASE("processor event/pub operation", "[unit][runtime][processor]") {
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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("event/pub publishes event") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(event/pub $CHANNEL_A 100 \"test message\")";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("event/pub with integer data") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(event/pub $CHANNEL_A 100 42)";
    request.request_id = "req2";

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

TEST_CASE("processor event/sub operation", "[unit][runtime][processor]") {
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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("event/sub subscribes to topic") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(event/sub $CHANNEL_A 200)";
    request.request_id = "req1";

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

TEST_CASE("processor runtime/log operation", "[unit][runtime][processor]") {
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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("runtime/log with single string") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(runtime/log \"Hello from SLP\")";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("runtime/log with multiple arguments") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(runtime/log \"Count:\" 42 \"Done\")";
    request.request_id = "req2";

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

  runtime::processor_c processor(logger.get(), &event_system);

  SECTION("null session pointer is handled") {
    runtime::execution_request_s request;
    request.session = nullptr;
    request.script_text = "42";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);
  }

  SECTION("parse error is handled") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(unclosed paren";
    request.request_id = "req2";

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

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(unknown/function arg1 arg2)";
    request.request_id = "req3";

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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("kv/set without permission fails") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(kv/set key \"value\")";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    CHECK_FALSE(session->get_store()->exists("key"));
  }

  SECTION("event/pub without permission fails") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(event/pub $CHANNEL_A 100 \"message\")";
    request.request_id = "req2";

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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("bracket list executes multiple statements") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text =
        "[(kv/set key1 \"value1\") (kv/set key2 \"value2\") (kv/set key3 "
        "\"value3\")]";
    request.request_id = "req1";

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

  runtime::processor_c processor(logger.get(), &event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("complex script with multiple operations") {
    runtime::execution_request_s request;
    request.session = session;
    request.script_text = R"([
      (kv/set user_name "Alice")
      (kv/set user_age 30)
      (runtime/log "User created:" (kv/get user_name))
      (event/sub $CHANNEL_A 100)
      (event/pub $CHANNEL_A 100 "User Alice logged in")
    ])";
    request.request_id = "req1";

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

TEST_CASE("processor runtime/eval operation", "[unit][runtime][processor]") {
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

  runtime::processor_c processor(logger.get(), &event_system);

  SECTION("evaluate simple integer literal") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(runtime/eval \"42\")";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("evaluate expression from kv/get") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = R"([
      (kv/set x "100")
      (runtime/eval (kv/get x))
    ])";
    request.request_id = "req2";

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

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(runtime/eval \"(runtime/log hello-world)\")";
    request.request_id = "req3";

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

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = "(runtime/eval \"(broken syntax\")";
    request.request_id = "req4";

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

TEST_CASE("processor runtime/eval with kv operations",
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

  runtime::processor_c processor(logger.get(), &event_system);

  SECTION("store result of eval in kv") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request;
    request.session = session;
    request.script_text = R"===([
      (kv/set script "(kv/set computed 999)")
      (runtime/eval (kv/get script))
      (kv/get computed)
    ])===";
    request.request_id = "req1";

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

TEST_CASE("processor runtime/await operation", "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_await");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_await_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(100, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(101, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), &event_system);

  SECTION("basic await with response") {
    runtime::session_c *session1 =
        create_test_session(event_system, data_ds, entity.get());
    runtime::session_c *session2 =
        create_test_session(event_system, data_ds, entity.get());

    std::atomic<bool> session1_done{false};
    std::thread session1_thread([&]() {
      runtime::execution_request_s request;
      request.session = session1;
      request.script_text = R"(
        (runtime/await 
          (event/pub $CHANNEL_A 100 "request-data")
          $CHANNEL_A 101)
      )";
      request.request_id = "req1";

      runtime::events::event_s event;
      event.category =
          runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
      event.topic_identifier = 0;
      event.payload = request;

      processor.consume_event(event);
      session1_done = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    runtime::execution_request_s request2;
    request2.session = session2;
    request2.script_text = R"(
      (event/sub $CHANNEL_A 100 {
        (event/pub $CHANNEL_A 101 "response-data")
      })
    )";
    request2.request_id = "req2";

    runtime::events::event_s event2;
    event2.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event2.topic_identifier = 0;
    event2.payload = request2;

    processor.consume_event(event2);

    session1_thread.join();
    CHECK(session1_done);

    delete session1;
    delete session2;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("processor runtime/await async communication",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_await_async");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_runtime_await_async_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(200, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(201, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), &event_system);

  SECTION("await with kv storage") {
    runtime::session_c *session1 =
        create_test_session(event_system, data_ds, entity.get());
    runtime::session_c *session2 =
        create_test_session(event_system, data_ds, entity.get());

    std::atomic<bool> session1_done{false};
    std::thread session1_thread([&]() {
      runtime::execution_request_s request;
      request.session = session1;
      request.script_text = R"([
        (kv/set result 
          (runtime/await 
            (event/pub $CHANNEL_B 200 "get-value")
            $CHANNEL_B 201))
        (runtime/log "Stored result:" (kv/get result))
      ])";
      request.request_id = "req1";

      runtime::events::event_s event;
      event.category =
          runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
      event.topic_identifier = 0;
      event.payload = request;

      processor.consume_event(event);
      session1_done = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    runtime::execution_request_s request2;
    request2.session = session2;
    request2.script_text = R"(
      (event/sub $CHANNEL_B 200 {
        (event/pub $CHANNEL_B 201 "computed-value-42")
      })
    )";
    request2.request_id = "req2";

    runtime::events::event_s event2;
    event2.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event2.topic_identifier = 0;
    event2.payload = request2;

    processor.consume_event(event2);

    session1_thread.join();
    CHECK(session1_done);

    std::string result;
    CHECK(session1->get_store()->get("result", result));
    CHECK(result == "computed-value-42");

    delete session1;
    delete session2;
  }

  SECTION("multiple awaits on different topics") {
    runtime::session_c *session1 =
        create_test_session(event_system, data_ds, entity.get());

    entity->grant_topic_permission(300, runtime::topic_permission::PUBSUB);
    entity->grant_topic_permission(301, runtime::topic_permission::PUBSUB);
    entity->save();

    std::thread responder_thread([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      session1->publish_event(
          runtime::events::event_category_e::RUNTIME_BACKCHANNEL_B, 201,
          std::string("first-response"));
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      session1->publish_event(
          runtime::events::event_category_e::RUNTIME_BACKCHANNEL_C, 301,
          std::string("second-response"));
    });

    runtime::execution_request_s request;
    request.session = session1;
    request.script_text = R"([
      (kv/set r1 
        (runtime/await 
          (event/pub $CHANNEL_B 200 "req1")
          $CHANNEL_B 201))
      (kv/set r2 
        (runtime/await 
          (event/pub $CHANNEL_C 300 "req2")
          $CHANNEL_C 301))
    ])";
    request.request_id = "req1";

    runtime::events::event_s event;
    event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    responder_thread.join();

    std::string r1, r2;
    CHECK(session1->get_store()->get("r1", r1));
    CHECK(r1 == "first-response");
    CHECK(session1->get_store()->get("r2", r2));
    CHECK(r2 == "second-response");

    delete session1;
  }

  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}
