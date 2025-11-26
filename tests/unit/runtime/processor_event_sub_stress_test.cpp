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

/*
    NOTE: The even system NEEDS TO SEND VALID SLP
    Sending std::string("this is a string") IS INVALID
    The automated testing system likes to flag this as an error. IT IS NOT.
    std::string("foo") -> SYMBOL SLP
    std::string("\"foo\"") -> DQ_LIST (string) - Stop fucking flagging this 
*/

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
  auto logger = spdlog::get("processor_stress_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("processor_stress_test");
  }
  return logger;
}

class test_accessor_c : public runtime::runtime_accessor_if {
public:
  void raise_warning(const char *message) override {}
  void raise_error(const char *message) override {}
};

runtime::session_c *
create_test_session(const std::string &session_id,
                    runtime::events::event_system_c &event_system,
                    kvds::datastore_c &data_ds, runtime::entity_c *entity) {
  return new runtime::session_c(session_id, "test_entity", "test_scope",
                                *entity, &data_ds, &event_system);
}
} // namespace

TEST_CASE("multiple sessions subscribe to same topic",
          "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_multi_session");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_multi_session_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(400, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);

  runtime::session_c *session1 =
      create_test_session("session1", event_system, data_ds, entity.get());
  runtime::session_c *session2 =
      create_test_session("session2", event_system, data_ds, entity.get());
  runtime::session_c *session3 =
      create_test_session("session3", event_system, data_ds, entity.get());

  runtime::execution_request_s req1{
      *session1,
      R"((core/event/sub $CHANNEL_A 400 :str {(core/kv/set session1_data $data)}))",
      "req1"};

  runtime::execution_request_s req2{
      *session2,
      R"((core/event/sub $CHANNEL_A 400 :str {(core/kv/set session2_data $data)}))",
      "req2"};

  runtime::execution_request_s req3{
      *session3,
      R"((core/event/sub $CHANNEL_A 400 :str {(core/kv/set session3_data $data)}))",
      "req3"};

  runtime::events::event_s sub_event1;
  sub_event1.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event1.topic_identifier = 0;
  sub_event1.payload = req1;

  runtime::events::event_s sub_event2 = sub_event1;
  sub_event2.payload = req2;

  runtime::events::event_s sub_event3 = sub_event1;
  sub_event3.payload = req3;

  processor.consume_event(sub_event1);
  processor.consume_event(sub_event2);
  processor.consume_event(sub_event3);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
  auto writer = producer->get_topic_writer_for_topic(400);

  runtime::events::event_s data_event;
  data_event.category =
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
  data_event.topic_identifier = 400;
  data_event.payload = std::string("\"broadcast message\"");

  writer->write_event(data_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::string s1_data, s2_data, s3_data;
  CHECK(session1->get_store()->get("session1_data", s1_data));
  CHECK(s1_data == "broadcast message");
  CHECK(session2->get_store()->get("session2_data", s2_data));
  CHECK(s2_data == "broadcast message");
  CHECK(session3->get_store()->get("session3_data", s3_data));
  CHECK(s3_data == "broadcast message");

  delete session1;
  delete session2;
  delete session3;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("session subscribes to multiple topics",
          "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_multi_topic");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_multi_topic_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(401, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(402, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(403, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session("session1", event_system, data_ds, entity.get());

  runtime::execution_request_s req{*session, R"([
    (core/event/sub $CHANNEL_A 401 :str {(core/kv/set topic401 $data)})
    (core/event/sub $CHANNEL_A 402 :str {(core/kv/set topic402 $data)})
    (core/event/sub $CHANNEL_A 403 :str {(core/kv/set topic403 $data)})
  ])",
                                   "multi_sub"};

  runtime::events::event_s sub_event;
  sub_event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event.topic_identifier = 0;
  sub_event.payload = req;

  processor.consume_event(sub_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);

  auto writer401 = producer->get_topic_writer_for_topic(401);
  auto writer402 = producer->get_topic_writer_for_topic(402);
  auto writer403 = producer->get_topic_writer_for_topic(403);

  runtime::events::event_s event401;
  event401.category = runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
  event401.topic_identifier = 401;
  event401.payload = std::string("\"message for 401\"");

  runtime::events::event_s event402 = event401;
  event402.topic_identifier = 402;
  event402.payload = std::string("\"message for 402\"");

  runtime::events::event_s event403 = event401;
  event403.topic_identifier = 403;
  event403.payload = std::string("\"message for 403\"");

  writer401->write_event(event401);
  writer402->write_event(event402);
  writer403->write_event(event403);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::string t401, t402, t403;
  CHECK(session->get_store()->get("topic401", t401));
  CHECK(t401 == "message for 401");
  CHECK(session->get_store()->get("topic402", t402));
  CHECK(t402 == "message for 402");
  CHECK(session->get_store()->get("topic403", t403));
  CHECK(t403 == "message for 403");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("rapid fire event delivery to handler",
          "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 200);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_rapid");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_rapid_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(500, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session("session1", event_system, data_ds, entity.get());

  runtime::execution_request_s req{*session, R"((core/event/sub $CHANNEL_A 500 :str {
    (core/kv/set last_event $data)
  }))",
                                   "rapid_sub"};

  runtime::events::event_s sub_event;
  sub_event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event.topic_identifier = 0;
  sub_event.payload = req;

  processor.consume_event(sub_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
  auto writer = producer->get_topic_writer_for_topic(500);

  for (int i = 0; i < 50; i++) {
    runtime::events::event_s data_event;
    data_event.category =
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
    data_event.topic_identifier = 500;
    data_event.payload = std::string("\"event_") + std::to_string(i) + "\"";
    writer->write_event(data_event);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::string last;
  CHECK(session->get_store()->get("last_event", last));
  CHECK(last.find("event_") == 0);

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("handler with parse error in body",
          "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_error");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_error_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(600, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session("session1", event_system, data_ds, entity.get());

  runtime::execution_request_s req{*session, R"((core/event/sub $CHANNEL_A 600 :str {
    (unknown/function arg1 arg2)
    (core/kv/set should_not_reach "here")
  }))",
                                   "error_sub"};

  runtime::events::event_s sub_event;
  sub_event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event.topic_identifier = 0;
  sub_event.payload = req;

  processor.consume_event(sub_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
  auto writer = producer->get_topic_writer_for_topic(600);

  runtime::events::event_s data_event;
  data_event.category =
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
  data_event.topic_identifier = 600;
  data_event.payload = std::string("\"test\"");

  writer->write_event(data_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  CHECK_FALSE(session->get_store()->exists("should_not_reach"));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("handler with nested function calls",
          "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_nested");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_nested_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(700, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session("session1", event_system, data_ds, entity.get());

  session->get_store()->set("base_value", "42");

  runtime::execution_request_s req{*session, R"((core/event/sub $CHANNEL_A 700 :str {
    (core/kv/set event_copy $data)
    (core/kv/set retrieved (core/kv/get base_value))
    (core/kv/set exists_check (core/kv/exists base_value))
    (core/util/log "Nested call with" $data)
  }))",
                                   "nested_sub"};

  runtime::events::event_s sub_event;
  sub_event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event.topic_identifier = 0;
  sub_event.payload = req;

  processor.consume_event(sub_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
  auto writer = producer->get_topic_writer_for_topic(700);

  runtime::events::event_s data_event;
  data_event.category =
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
  data_event.topic_identifier = 700;
  data_event.payload = std::string("\"nested test\"");

  writer->write_event(data_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::string event_copy, retrieved, exists_check;
  CHECK(session->get_store()->get("event_copy", event_copy));
  CHECK(event_copy == "nested test");
  CHECK(session->get_store()->get("retrieved", retrieved));
  CHECK(retrieved == "42");
  CHECK(session->get_store()->get("exists_check", exists_check));
  CHECK(exists_check == "true");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("handler publishes event creating chain",
          "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 4, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_chain");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_chain_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(800, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(801, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session("session1", event_system, data_ds, entity.get());

  runtime::execution_request_s req{*session, R"([
    (core/event/sub $CHANNEL_A 800 :str {
      (core/kv/set step1 $data)
      (core/event/pub $CHANNEL_A 801 "chained")
    })
    (core/event/sub $CHANNEL_A 801 :str {
      (core/kv/set step2 $data)
    })
  ])",
                                   "chain_sub"};

  runtime::events::event_s sub_event;
  sub_event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event.topic_identifier = 0;
  sub_event.payload = req;

  processor.consume_event(sub_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
  auto writer = producer->get_topic_writer_for_topic(800);

  runtime::events::event_s data_event;
  data_event.category =
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
  data_event.topic_identifier = 800;
  data_event.payload = std::string("\"initial\"");

  writer->write_event(data_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  std::string step1, step2;
  CHECK(session->get_store()->get("step1", step1));
  CHECK(step1 == "initial");
  CHECK(session->get_store()->get("step2", step2));
  CHECK(step2 == "chained");

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("empty handler body", "[unit][runtime][processor][stress]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_stress_empty");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_stress_empty_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_topic_permission(900, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session("session1", event_system, data_ds, entity.get());

  runtime::execution_request_s req{
      *session, R"((core/event/sub $CHANNEL_A 900 :str {}))", "empty_sub"};

  runtime::events::event_s sub_event;
  sub_event.category =
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
  sub_event.topic_identifier = 0;
  sub_event.payload = req;

  processor.consume_event(sub_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto producer = event_system.get_event_producer_for_category(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
  auto writer = producer->get_topic_writer_for_topic(900);

  runtime::events::event_s data_event;
  data_event.category =
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
  data_event.topic_identifier = 900;
  data_event.payload = std::string("\"test\"");

  writer->write_event(data_event);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}
