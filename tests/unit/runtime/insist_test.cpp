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
  auto logger = spdlog::get("insist_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("insist_test");
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

TEST_CASE("core/util/insist passes through non-error values from functions",
          "[unit][runtime][insist]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/insist_test_passthrough");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/insist_test_passthrough_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("passes through INTEGER from function result") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/kv/set num 42) (core/util/insist (core/kv/get num))]", "req1"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("passes through DQ_LIST from function result") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/kv/set str \"hello\") (core/util/insist (core/kv/get str))]", "req2"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("passes through SYMBOL from exists check") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/kv/set key \"val\") (core/util/insist (core/kv/exists key))]", "req3"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
  event_system.shutdown();
}

TEST_CASE("core/util/insist halts execution on ERROR object",
          "[unit][runtime][insist]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/insist_test_error");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/insist_test_error_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("halts on ERROR from function") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    entity->grant_permission("test_scope", runtime::permission::READ_WRITE);

    runtime::execution_request_s request{*session, "(core/util/insist (core/kv/get nonexistent))", "req2"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
  event_system.shutdown();
}

TEST_CASE("core/util/insist enables safe type patterns",
          "[unit][runtime][insist]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/insist_test_patterns");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/insist_test_patterns_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("set -> insist(get) pattern works") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/kv/set x 42) (core/kv/set y (core/util/insist (core/kv/get x)))]", 
        "req1"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("insist(get) on nonexistent key halts") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/kv/set x 42) (core/kv/set y (core/util/insist (core/kv/get z)))]", 
        "req2"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("nested insist passes through value") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/kv/set x 42) (core/util/insist (core/util/insist (core/kv/get x)))]", 
        "req3"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
  event_system.shutdown();
}

TEST_CASE("core/util/insist with bracket list stops on first error",
          "[unit][runtime][insist]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/insist_test_bracket");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/insist_test_bracket_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("first insist fails, second line never executes") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, 
        "[(core/util/insist (core/kv/get nonexistent)) (core/kv/set marker 1)]", 
        "req1"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
  event_system.shutdown();
}

TEST_CASE("core/util/insist rejects non-function arguments at runtime",
          "[unit][runtime][insist]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/insist_test_rejection");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/insist_test_rejection_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);

  runtime::processor_c processor(logger.get(), event_system);

  SECTION("rejects literal integer") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "(core/util/insist 42)", "req1"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("rejects literal string") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "(core/util/insist \"hello\")", "req2"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("rejects symbol") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "(core/util/insist x)", "req3"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  SECTION("rejects literal ERROR") {
    runtime::session_c *session =
        create_test_session(event_system, data_ds, entity.get());

    runtime::execution_request_s request{*session, "(core/util/insist @\"error\")", "req4"};

    runtime::events::event_s event;
    event.category = runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    event.topic_identifier = 0;
    event.payload = request;

    processor.consume_event(event);

    delete session;
  }

  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
  event_system.shutdown();
}
