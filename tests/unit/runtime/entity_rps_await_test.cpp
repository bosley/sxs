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
  auto logger = spdlog::get("entity_rps_await_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("entity_rps_await_test");
  }
  return logger;
}
} // namespace

TEST_CASE("entity rps with runtime/await cross-channel limiting",
          "[unit][runtime][entity][rps][await]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_await_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_await_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  runtime::processor_c processor(logger.get(), event_system);

  auto processor_consumer = std::shared_ptr<runtime::events::event_consumer_if>(
      &processor, [](runtime::events::event_consumer_if *) {});
  event_system.register_consumer(0, processor_consumer);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("RPS limit applies across all channels in runtime/await") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = entity_opt.value().get();

    entity->set_max_rps(5);
    entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    entity->grant_topic_permission(2, runtime::topic_permission::PUBSUB);
    entity->grant_topic_permission(3, runtime::topic_permission::PUBSUB);
    entity->save();

    runtime::session_c session("sess1", "user1", "scope1", entity, &data_ds,
                               &event_system);

    int successful_publishes = 0;
    int failed_publishes = 0;

    for (int i = 0; i < 3; ++i) {
      auto result = session.publish_event(
          runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1, i);
      if (result == runtime::publish_result_e::OK)
        successful_publishes++;
      else
        failed_publishes++;
    }

    for (int i = 0; i < 3; ++i) {
      auto result = session.publish_event(
          runtime::events::event_category_e::RUNTIME_BACKCHANNEL_B, 2, i);
      if (result == runtime::publish_result_e::OK)
        successful_publishes++;
      else
        failed_publishes++;
    }

    for (int i = 0; i < 3; ++i) {
      auto result = session.publish_event(
          runtime::events::event_category_e::RUNTIME_BACKCHANNEL_C, 3, i);
      if (result == runtime::publish_result_e::OK)
        successful_publishes++;
      else
        failed_publishes++;
    }

    CHECK(successful_publishes == 5);
    CHECK(failed_publishes == 4);
  }

  SECTION("runtime/await respects RPS limits across channels") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity_opt.has_value());
    auto entity = entity_opt.value().get();

    entity->set_max_rps(3);
    entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    entity->grant_topic_permission(100, runtime::topic_permission::PUBSUB);
    entity->save();

    runtime::session_c session("sess1", "user2", "scope1", entity, &data_ds,
                               &event_system);

    runtime::session_c responder_session("responder", "user2", "scope1", entity,
                                         &data_ds, &event_system);
    responder_session.subscribe_to_topic(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1,
        [&responder_session](const runtime::events::event_s &event) {
          responder_session.publish_event(
              runtime::events::event_category_e::RUNTIME_BACKCHANNEL_B, 100,
              std::string("response"));
        });

    runtime::execution_request_s request;
    request.session = &session;
    request.request_id = "test_1";

    request.script_text = R"(
      [
        (event/pub $CHANNEL_A 1 "msg1")
        (event/pub $CHANNEL_A 1 "msg2")
        (event/pub $CHANNEL_A 1 "msg3")
        (event/pub $CHANNEL_A 1 "msg4")
      ]
    )";

    runtime::events::event_s exec_event;
    exec_event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    exec_event.topic_identifier = 0;
    exec_event.payload = request;

    processor.consume_event(exec_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("entity rps limit tracking across channels",
          "[unit][runtime][entity][rps][multichannel]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_multichannel_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_multichannel_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("Entity RPS is shared across all 6 channels") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = entity_opt.value().get();

    entity->set_max_rps(6);
    entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    for (int i = 1; i <= 6; ++i) {
      entity->grant_topic_permission(i, runtime::topic_permission::PUBSUB);
    }
    entity->save();

    runtime::session_c session("sess1", "user1", "scope1", entity, &data_ds,
                               &event_system);

    int total_published = 0;

    if (session.publish_event(
            runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1, 1) ==
        runtime::publish_result_e::OK)
      total_published++;
    if (session.publish_event(
            runtime::events::event_category_e::RUNTIME_BACKCHANNEL_B, 2, 2) ==
        runtime::publish_result_e::OK)
      total_published++;
    if (session.publish_event(
            runtime::events::event_category_e::RUNTIME_BACKCHANNEL_C, 3, 3) ==
        runtime::publish_result_e::OK)
      total_published++;
    if (session.publish_event(
            runtime::events::event_category_e::RUNTIME_BACKCHANNEL_D, 4, 4) ==
        runtime::publish_result_e::OK)
      total_published++;
    if (session.publish_event(
            runtime::events::event_category_e::RUNTIME_BACKCHANNEL_E, 5, 5) ==
        runtime::publish_result_e::OK)
      total_published++;
    if (session.publish_event(
            runtime::events::event_category_e::RUNTIME_BACKCHANNEL_F, 6, 6) ==
        runtime::publish_result_e::OK)
      total_published++;

    CHECK(total_published == 6);

    CHECK(session.publish_event(
              runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1,
              99) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
    CHECK(session.publish_event(
              runtime::events::event_category_e::RUNTIME_BACKCHANNEL_B, 2,
              99) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
    CHECK(session.publish_event(
              runtime::events::event_category_e::RUNTIME_BACKCHANNEL_C, 3,
              99) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}
