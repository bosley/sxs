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
  auto logger = spdlog::get("processor_kv_context_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("processor_kv_context_test");
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

TEST_CASE("basic iterate with $key del", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_del");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_del_entity");
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

  for (int i = 1; i <= 10; i++) {
    session->get_store()->set("temp:" + std::to_string(i), "value" + std::to_string(i));
    CHECK(session->get_store()->exists("temp:" + std::to_string(i)));
  }

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate temp: 0 100 {
      (core/kv/del $key)
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 1; i <= 10; i++) {
    CHECK_FALSE(session->get_store()->exists("temp:" + std::to_string(i)));
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("iterate with $key exists check", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_exists");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_exists_entity");
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

  session->get_store()->set("cache:a", "val_a");
  session->get_store()->set("cache:b", "val_b");
  session->get_store()->set("cache:c", "val_c");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate cache: 0 100 {
      (core/util/log "Checking existence of" $key)
      (core/kv/exists $key)
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(session->get_store()->exists("cache:a"));
  CHECK(session->get_store()->exists("cache:b"));
  CHECK(session->get_store()->exists("cache:c"));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("iterate with $key load", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_load");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_load_entity");
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

  session->get_store()->set("data:a", "100");
  session->get_store()->set("data:b", "200");
  session->get_store()->set("data:c", "300");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate data: 0 100 {
      (core/kv/load $key)
      (core/kv/set load_success "true")
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::string success;
  CHECK(session->get_store()->get("load_success", success));
  CHECK(success == "true");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("combined del + exists + load in iteration", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_combined");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_combined_entity");
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

  for (int i = 1; i <= 5; i++) {
    session->get_store()->set("item:" + std::to_string(i), "data_" + std::to_string(i));
  }

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate item: 0 100 {
      (core/kv/exists $key)
      (core/kv/load $key)
      (core/kv/del $key)
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 1; i <= 5; i++) {
    CHECK_FALSE(session->get_store()->exists("item:" + std::to_string(i)));
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("insist with $key operations in iteration", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_insist");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_insist_entity");
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

  session->get_store()->set("valid:1", "val1");
  session->get_store()->set("valid:2", "val2");
  session->get_store()->set("valid:3", "val3");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate valid: 0 100 {
      (core/util/insist (core/kv/exists $key))
      (core/util/insist (core/kv/load $key))
      (core/kv/set success "true")
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::string success;
  CHECK(session->get_store()->get("success", success));
  CHECK(success == "true");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("insist failure with $key in iteration", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_insist_fail");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_insist_fail_entity");
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

  session->get_store()->set("fail:1", "val1");
  session->get_store()->set("fail:2", "val2");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate fail: 0 100 {
      (core/kv/del $key)
      (core/util/insist (core/kv/load $key))
      (core/kv/set should_not_reach "true")
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK_FALSE(session->get_store()->exists("should_not_reach"));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("event handler with $key operations", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_event");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_event_entity");
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

  runtime::execution_request_s setup_request{*session, R"({
    (core/event/sub $CHANNEL_A 100 :str {
      (core/kv/set user:1 "alice")
      (core/kv/set user:2 "bob")
      (core/kv/set user:3 "charlie")
      (core/kv/set user:4 "diana")
      (core/kv/set user:5 "eve")
      (core/kv/set user:6 "frank")
      (core/kv/set user:7 "grace")
      (core/kv/set user:8 "henry")
      (core/kv/set user:9 "iris")
      (core/kv/set user:10 "jack")
    })
  })", "setup"};

  runtime::events::event_s setup_event;
  setup_event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  setup_event.topic_identifier = 0;
  setup_event.payload = setup_request;

  processor.consume_event(setup_event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  runtime::execution_request_s trigger_request{*session, R"(
    (core/event/pub $CHANNEL_A 100 "trigger")
  )", "trigger"};

  runtime::events::event_s trigger_event;
  trigger_event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  trigger_event.topic_identifier = 0;
  trigger_event.payload = trigger_request;

  processor.consume_event(trigger_event);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  for (int i = 1; i <= 10; i++) {
    CHECK(session->get_store()->exists("user:" + std::to_string(i)));
  }

  runtime::execution_request_s delete_request{*session, R"(
    (core/kv/iterate user: 0 100 {
      (core/kv/del $key)
    })
  )", "delete"};

  runtime::events::event_s delete_event;
  delete_event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  delete_event.topic_identifier = 0;
  delete_event.payload = delete_request;

  processor.consume_event(delete_event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 1; i <= 10; i++) {
    CHECK_FALSE(session->get_store()->exists("user:" + std::to_string(i)));
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("complex integration: events + iterate + insist", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_complex");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_complex_entity");
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

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  runtime::execution_request_s setup_request{*session, R"({
    (core/event/sub $CHANNEL_B 200 :str {
      (core/kv/set product:1 "laptop")
      (core/kv/set product:2 "mouse")
      (core/kv/set product:3 "keyboard")
      (core/kv/set product:4 "monitor")
      (core/kv/set product:5 "headset")
    })
    (core/event/sub $CHANNEL_B 201 :str {
      (core/kv/iterate product: 0 100 {
        (core/util/insist (core/kv/exists $key))
        (core/util/insist (core/kv/load $key))
        (core/kv/del $key)
      })
      (core/kv/set cleanup_done "true")
    })
  })", "setup"};

  runtime::events::event_s setup_event;
  setup_event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  setup_event.topic_identifier = 0;
  setup_event.payload = setup_request;

  processor.consume_event(setup_event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  runtime::execution_request_s create_request{*session, R"(
    (core/event/pub $CHANNEL_B 200 "create_products")
  )", "create"};

  runtime::events::event_s create_event;
  create_event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  create_event.topic_identifier = 0;
  create_event.payload = create_request;

  processor.consume_event(create_event);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  for (int i = 1; i <= 5; i++) {
    CHECK(session->get_store()->exists("product:" + std::to_string(i)));
  }

  runtime::execution_request_s cleanup_request{*session, R"(
    (core/event/pub $CHANNEL_B 201 "cleanup_products")
  )", "cleanup"};

  runtime::events::event_s cleanup_event;
  cleanup_event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  cleanup_event.topic_identifier = 0;
  cleanup_event.payload = cleanup_request;

  processor.consume_event(cleanup_event);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  for (int i = 1; i <= 5; i++) {
    CHECK_FALSE(session->get_store()->exists("product:" + std::to_string(i)));
  }

  std::string cleanup_done;
  CHECK(session->get_store()->get("cleanup_done", cleanup_done));
  CHECK(cleanup_done == "true");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("error case: $key not available", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_error1");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_error1_entity");
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

  session->get_store()->set("somekey", "somevalue");

  runtime::execution_request_s request{*session, R"(
    (core/kv/del $key)
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(session->get_store()->exists("somekey"));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("iterate with $key exists returning false", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_exists_false");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_exists_false_entity");
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

  session->get_store()->set("check:1", "val1");
  session->get_store()->set("check:2", "val2");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate check: 0 100 {
      (core/kv/del $key)
      (core/kv/set exists_result (core/kv/exists $key))
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::string exists_result;
  CHECK(session->get_store()->get("exists_result", exists_result));
  CHECK(exists_result == "false");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("iterate loads all values into separate keys", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_load_all");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_load_all_entity");
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

  session->get_store()->set("src:1", "100");
  session->get_store()->set("src:2", "200");
  session->get_store()->set("src:3", "300");
  session->get_store()->set("src:4", "400");
  session->get_store()->set("src:5", "500");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate src: 0 100 {
      (core/kv/load $key)
      (core/util/log "Loaded" $key)
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(session->get_store()->exists("src:1"));
  CHECK(session->get_store()->exists("src:2"));
  CHECK(session->get_store()->exists("src:3"));
  CHECK(session->get_store()->exists("src:4"));
  CHECK(session->get_store()->exists("src:5"));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("context variable vs literal key behavior", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_literal");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_literal_entity");
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

  session->get_store()->set("test:x", "dynamic_value");
  session->get_store()->set("$key", "literal_dollar_key_value");

  runtime::execution_request_s request{*session, R"({
    (core/kv/iterate test: 0 100 {
      (core/kv/load $key)
      (core/kv/set iter_ran "true")
    })
    (core/kv/set from_literal (core/kv/get $key))
  })", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::string iter_ran;
  CHECK(session->get_store()->get("iter_ran", iter_ran));
  CHECK(iter_ran == "true");

  std::string from_literal;
  CHECK(session->get_store()->get("from_literal", from_literal));
  CHECK(from_literal == "literal_dollar_key_value");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("error case: insist with missing $key", "[unit][runtime][processor][context]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_error2");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_kv_context_error2_entity");
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

  session->get_store()->set("missing:1", "val1");
  session->get_store()->set("missing:2", "val2");

  runtime::execution_request_s request{*session, R"(
    (core/kv/iterate missing: 0 100 {
      (core/kv/del $key)
      (core/util/insist (core/kv/load $key))
      (core/kv/set should_not_reach "true")
    })
  )", "req1"};

  runtime::events::event_s event;
  event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  event.topic_identifier = 0;
  event.payload = request;

  processor.consume_event(event);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK_FALSE(session->get_store()->exists("should_not_reach"));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

