#include <any>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
#include <record/record.hpp>
#include <runtime/entity/entity.hpp>
#include <runtime/events/events.hpp>
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
  auto logger = spdlog::get("session_event_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("session_event_test");
  }
  return logger;
}
} // namespace

TEST_CASE("entity topic permission granting",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_grant_entity");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  SECTION("can grant publish permission") {
    entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
    CHECK(entity->is_permitted_topic(100, runtime::topic_permission::PUBLISH));
  }

  SECTION("can grant subscribe permission") {
    entity->grant_topic_permission(200, runtime::topic_permission::SUBSCRIBE);
    CHECK(
        entity->is_permitted_topic(200, runtime::topic_permission::SUBSCRIBE));
  }

  SECTION("can grant pubsub permission") {
    entity->grant_topic_permission(300, runtime::topic_permission::PUBSUB);
    CHECK(entity->is_permitted_topic(300, runtime::topic_permission::PUBSUB));
    CHECK(entity->is_permitted_topic(300, runtime::topic_permission::PUBLISH));
    CHECK(
        entity->is_permitted_topic(300, runtime::topic_permission::SUBSCRIBE));
  }

  SECTION("multiple topic permissions are independent") {
    entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
    entity->grant_topic_permission(200, runtime::topic_permission::SUBSCRIBE);
    entity->grant_topic_permission(300, runtime::topic_permission::PUBSUB);

    CHECK(entity->is_permitted_topic(100, runtime::topic_permission::PUBLISH));
    CHECK_FALSE(
        entity->is_permitted_topic(100, runtime::topic_permission::SUBSCRIBE));

    CHECK_FALSE(
        entity->is_permitted_topic(200, runtime::topic_permission::PUBLISH));
    CHECK(
        entity->is_permitted_topic(200, runtime::topic_permission::SUBSCRIBE));

    CHECK(entity->is_permitted_topic(300, runtime::topic_permission::PUBLISH));
    CHECK(
        entity->is_permitted_topic(300, runtime::topic_permission::SUBSCRIBE));
  }

  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("entity topic permission revocation",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_revoke_entity");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  SECTION("can revoke granted permission") {
    entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
    CHECK(entity->is_permitted_topic(100, runtime::topic_permission::PUBLISH));

    entity->revoke_topic_permission(100);
    CHECK_FALSE(
        entity->is_permitted_topic(100, runtime::topic_permission::PUBLISH));
  }

  SECTION("revoking one topic doesnt affect others") {
    entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
    entity->grant_topic_permission(200, runtime::topic_permission::SUBSCRIBE);

    entity->revoke_topic_permission(100);

    CHECK_FALSE(
        entity->is_permitted_topic(100, runtime::topic_permission::PUBLISH));
    CHECK(
        entity->is_permitted_topic(200, runtime::topic_permission::SUBSCRIBE));
  }

  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("entity topic permission persistence",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_persist_entity");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  {
    record::record_manager_c entity_manager(entity_ds, logger);
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());

    entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
    entity->grant_topic_permission(200, runtime::topic_permission::SUBSCRIBE);
    entity->grant_topic_permission(300, runtime::topic_permission::PUBSUB);
    entity->save();
  }

  {
    record::record_manager_c entity_manager(entity_ds, logger);
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());

    CHECK(entity->is_permitted_topic(100, runtime::topic_permission::PUBLISH));
    CHECK(
        entity->is_permitted_topic(200, runtime::topic_permission::SUBSCRIBE));
    CHECK(entity->is_permitted_topic(300, runtime::topic_permission::PUBSUB));
    CHECK(entity->is_permitted_topic(300, runtime::topic_permission::PUBLISH));
    CHECK(
        entity->is_permitted_topic(300, runtime::topic_permission::SUBSCRIBE));
  }

  ensure_db_cleanup(entity_test_path);
}

TEST_CASE("session publish with permissions",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_pub_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_pub_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  SECTION("can publish with publish permission") {
    entity->grant_topic_permission(100, runtime::topic_permission::PUBLISH);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    CHECK(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 100,
        std::string("test_payload")));
  }

  SECTION("can publish with pubsub permission") {
    entity->grant_topic_permission(101, runtime::topic_permission::PUBSUB);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    CHECK(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 101,
        std::string("test_payload")));
  }

  SECTION("cannot publish without permission") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    CHECK_FALSE(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 102,
        std::string("test_payload")));
  }

  SECTION("cannot publish with only subscribe permission") {
    entity->grant_topic_permission(103, runtime::topic_permission::SUBSCRIBE);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    CHECK_FALSE(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 103,
        std::string("test_payload")));
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session subscribe with permissions",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_sub_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_sub_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();

  SECTION("can subscribe with subscribe permission") {
    entity->grant_topic_permission(200, runtime::topic_permission::SUBSCRIBE);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    bool handler_called = false;
    auto handler = [&handler_called](const runtime::events::event_s &event) {
      handler_called = true;
    };

    CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 200, handler));
  }

  SECTION("can subscribe with pubsub permission") {
    entity->grant_topic_permission(201, runtime::topic_permission::PUBSUB);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    bool handler_called = false;
    auto handler = [&handler_called](const runtime::events::event_s &event) {
      handler_called = true;
    };

    CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 201, handler));
  }

  SECTION("cannot subscribe without permission") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    bool handler_called = false;
    auto handler = [&handler_called](const runtime::events::event_s &event) {
      handler_called = true;
    };

    CHECK_FALSE(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 202, handler));
  }

  SECTION("cannot subscribe with only publish permission") {
    entity->grant_topic_permission(203, runtime::topic_permission::PUBLISH);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    bool handler_called = false;
    auto handler = [&handler_called](const runtime::events::event_s &event) {
      handler_called = true;
    };

    CHECK_FALSE(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 203, handler));
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session event publish and consume",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_pubsub_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_pubsub_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("single session can publish and receive its own events") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
    entity->grant_topic_permission(300, runtime::topic_permission::PUBSUB);
    entity->save();

    runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                               &data_ds, &event_system);

    std::atomic<int> event_count{0};
    std::string received_payload;

    auto handler = [&event_count,
                    &received_payload](const runtime::events::event_s &event) {
      event_count++;
      if (event.payload.has_value()) {
        received_payload = std::any_cast<std::string>(event.payload);
      }
    };

    CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 300, handler));
    CHECK(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 300,
        std::string("hello_world")));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(event_count.load() == 1);
    CHECK(received_payload == "hello_world");
  }

  SECTION("multiple sessions can communicate via events") {
    auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity1_opt.has_value());
    REQUIRE(entity2_opt.has_value());
    auto entity1 = std::move(entity1_opt.value());
    auto entity2 = std::move(entity2_opt.value());

    entity1->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity1->grant_topic_permission(400, runtime::topic_permission::PUBLISH);
    entity1->save();

    entity2->grant_permission("scope2", runtime::permission::READ_WRITE);
    entity2->grant_topic_permission(400, runtime::topic_permission::SUBSCRIBE);
    entity2->save();

    runtime::session_c session1("sess1", "user1", "scope1", entity1.get(),
                                &data_ds, &event_system);
    runtime::session_c session2("sess2", "user2", "scope2", entity2.get(),
                                &data_ds, &event_system);

    std::atomic<int> session2_event_count{0};
    std::string session2_payload;

    auto handler2 = [&session2_event_count,
                     &session2_payload](const runtime::events::event_s &event) {
      session2_event_count++;
      if (event.payload.has_value()) {
        session2_payload = std::any_cast<std::string>(event.payload);
      }
    };

    CHECK(session2.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 400, handler2));
    CHECK(session1.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 400,
        std::string("message_from_user1")));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(session2_event_count.load() == 1);
    CHECK(session2_payload == "message_from_user1");
  }

  SECTION("multiple subscribers receive same event") {
    auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    auto entity3_opt = entity_manager.get_or_create<runtime::entity_c>("user3");
    REQUIRE(entity1_opt.has_value());
    REQUIRE(entity2_opt.has_value());
    REQUIRE(entity3_opt.has_value());
    auto entity1 = std::move(entity1_opt.value());
    auto entity2 = std::move(entity2_opt.value());
    auto entity3 = std::move(entity3_opt.value());

    entity1->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity1->grant_topic_permission(500, runtime::topic_permission::PUBLISH);
    entity1->save();

    entity2->grant_permission("scope2", runtime::permission::READ_WRITE);
    entity2->grant_topic_permission(500, runtime::topic_permission::SUBSCRIBE);
    entity2->save();

    entity3->grant_permission("scope3", runtime::permission::READ_WRITE);
    entity3->grant_topic_permission(500, runtime::topic_permission::SUBSCRIBE);
    entity3->save();

    runtime::session_c session1("sess1", "user1", "scope1", entity1.get(),
                                &data_ds, &event_system);
    runtime::session_c session2("sess2", "user2", "scope2", entity2.get(),
                                &data_ds, &event_system);
    runtime::session_c session3("sess3", "user3", "scope3", entity3.get(),
                                &data_ds, &event_system);

    std::atomic<int> count2{0};
    std::atomic<int> count3{0};

    auto handler2 = [&count2](const runtime::events::event_s &event) {
      count2++;
    };
    auto handler3 = [&count3](const runtime::events::event_s &event) {
      count3++;
    };

    CHECK(session2.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 500, handler2));
    CHECK(session3.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 500, handler3));

    CHECK(session1.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 500,
        std::string("broadcast")));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(count2.load() == 1);
    CHECK(count3.load() == 1);
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session unsubscribe from topics",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_unsub_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_unsub_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(600, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                             &data_ds, &event_system);

  std::atomic<int> event_count{0};
  auto handler = [&event_count](const runtime::events::event_s &event) {
    event_count++;
  };

  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 600, handler));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 600,
      std::string("msg1")));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  CHECK(event_count.load() == 1);

  CHECK(session.unsubscribe_from_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 600));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 600,
      std::string("msg2")));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  CHECK(event_count.load() == 1);

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session multiple topic subscriptions",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_multi_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_multi_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(700, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(701, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(702, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                             &data_ds, &event_system);

  std::atomic<int> count700{0};
  std::atomic<int> count701{0};
  std::atomic<int> count702{0};

  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      700, [&count700](const runtime::events::event_s &e) { count700++; }));
  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      701, [&count701](const runtime::events::event_s &e) { count701++; }));
  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      702, [&count702](const runtime::events::event_s &e) { count702++; }));

  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 700,
      std::string("msg700")));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 701,
      std::string("msg701")));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 701,
      std::string("msg701_2")));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 702,
      std::string("msg702")));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(count700.load() == 1);
  CHECK(count701.load() == 2);
  CHECK(count702.load() == 1);

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session event payload types", "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_payload_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_payload_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(800, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(801, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                             &data_ds, &event_system);

  SECTION("string payload") {
    std::string received;
    auto handler = [&received](const runtime::events::event_s &event) {
      if (event.payload.has_value()) {
        received = std::any_cast<std::string>(event.payload);
      }
    };

    CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 800, handler));
    CHECK(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 800,
        std::string("test_string")));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(received == "test_string");
  }

  SECTION("int payload") {
    int received = 0;
    auto handler = [&received](const runtime::events::event_s &event) {
      if (event.payload.has_value()) {
        received = std::any_cast<int>(event.payload);
      }
    };

    CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 801, handler));
    CHECK(session.publish_event(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 801, 42));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(received == 42);
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session event category verification",
          "[unit][runtime][session][events]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_category_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_category_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(900, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                             &data_ds, &event_system);

  runtime::events::event_category_e received_category =
      runtime::events::event_category_e::RUNTIME_SUBSYSTEM_UNKNOWN;
  auto handler = [&received_category](const runtime::events::event_s &event) {
    received_category = event.category;
  };

  // Subscribe to EXECUTION_REQUEST category to receive events on that category
  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 900, handler));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 900,
      std::string("test")));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  CHECK(received_category ==
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST);

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("bidirectional communication on same topic",
          "[unit][runtime][session][events][integration]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_bidir_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_bidir_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
  REQUIRE(entity1_opt.has_value());
  REQUIRE(entity2_opt.has_value());
  auto entity1 = std::move(entity1_opt.value());
  auto entity2 = std::move(entity2_opt.value());

  entity1->grant_permission("scope1", runtime::permission::READ_WRITE);
  entity1->grant_topic_permission(1000, runtime::topic_permission::PUBSUB);
  entity1->save();

  entity2->grant_permission("scope2", runtime::permission::READ_WRITE);
  entity2->grant_topic_permission(1000, runtime::topic_permission::PUBSUB);
  entity2->save();

  runtime::session_c session1("sess1", "user1", "scope1", entity1.get(),
                              &data_ds, &event_system);
  runtime::session_c session2("sess2", "user2", "scope2", entity2.get(),
                              &data_ds, &event_system);

  std::atomic<int> session1_count{0};
  std::atomic<int> session2_count{0};
  std::string session1_received;
  std::string session2_received;

  auto handler1 = [&session1_count,
                   &session1_received](const runtime::events::event_s &event) {
    session1_count++;
    if (event.payload.has_value()) {
      session1_received = std::any_cast<std::string>(event.payload);
    }
  };

  auto handler2 = [&session2_count,
                   &session2_received](const runtime::events::event_s &event) {
    session2_count++;
    if (event.payload.has_value()) {
      session2_received = std::any_cast<std::string>(event.payload);
    }
  };

  CHECK(session1.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1000, handler1));
  CHECK(session2.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1000, handler2));

  CHECK(session1.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1000,
      std::string("hello_from_1")));
  CHECK(session2.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1000,
      std::string("hello_from_2")));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK(session1_count.load() == 2);
  CHECK(session2_count.load() == 2);
  CHECK((session1_received == "hello_from_1" ||
         session1_received == "hello_from_2"));
  CHECK((session2_received == "hello_from_1" ||
         session2_received == "hello_from_2"));

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("topic isolation prevents cross-topic leakage",
          "[unit][runtime][session][events][integration]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_isolation_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_isolation_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(1100, runtime::topic_permission::PUBSUB);
  entity->grant_topic_permission(1101, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::session_c session("sess1", "user1", "test_scope", entity.get(),
                             &data_ds, &event_system);

  std::atomic<int> topic1100_count{0};
  std::atomic<int> topic1101_count{0};

  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      1100, [&topic1100_count](const runtime::events::event_s &e) {
        topic1100_count++;
      }));
  CHECK(session.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      1101, [&topic1101_count](const runtime::events::event_s &e) {
        topic1101_count++;
      }));

  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1100,
      std::string("to_1100")));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1100,
      std::string("to_1100_again")));
  CHECK(session.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1101,
      std::string("to_1101")));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(topic1100_count.load() == 2);
  CHECK(topic1101_count.load() == 1);

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("mixed permissions on same topic",
          "[unit][runtime][session][events][integration]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_mixed_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_mixed_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  auto entity1_opt =
      entity_manager.get_or_create<runtime::entity_c>("publisher");
  auto entity2_opt =
      entity_manager.get_or_create<runtime::entity_c>("subscriber");
  auto entity3_opt =
      entity_manager.get_or_create<runtime::entity_c>("no_permission");
  REQUIRE(entity1_opt.has_value());
  REQUIRE(entity2_opt.has_value());
  REQUIRE(entity3_opt.has_value());
  auto entity1 = std::move(entity1_opt.value());
  auto entity2 = std::move(entity2_opt.value());
  auto entity3 = std::move(entity3_opt.value());

  entity1->grant_permission("scope1", runtime::permission::READ_WRITE);
  entity1->grant_topic_permission(1200, runtime::topic_permission::PUBLISH);
  entity1->save();

  entity2->grant_permission("scope2", runtime::permission::READ_WRITE);
  entity2->grant_topic_permission(1200, runtime::topic_permission::SUBSCRIBE);
  entity2->save();

  entity3->grant_permission("scope3", runtime::permission::READ_WRITE);
  entity3->save();

  runtime::session_c session1("sess1", "publisher", "scope1", entity1.get(),
                              &data_ds, &event_system);
  runtime::session_c session2("sess2", "subscriber", "scope2", entity2.get(),
                              &data_ds, &event_system);
  runtime::session_c session3("sess3", "no_permission", "scope3", entity3.get(),
                              &data_ds, &event_system);

  std::atomic<int> session2_count{0};
  auto handler2 = [&session2_count](const runtime::events::event_s &e) {
    session2_count++;
  };

  CHECK(session2.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1200, handler2));

  CHECK(session1.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1200,
      std::string("from_publisher")));
  CHECK_FALSE(session2.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1200,
      std::string("should_fail")));
  CHECK_FALSE(session3.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1200,
      std::string("should_fail")));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(session2_count.load() == 1);

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("multiple sessions per entity receive events",
          "[unit][runtime][session][events][integration]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/session_event_test_multisess_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/session_event_test_multisess_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get(), 4, 1000);
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());

  entity->grant_permission("scope_a", runtime::permission::READ_WRITE);
  entity->grant_permission("scope_b", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(1300, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::session_c session_a("sess_a", "user1", "scope_a", entity.get(),
                               &data_ds, &event_system);
  runtime::session_c session_b("sess_b", "user1", "scope_b", entity.get(),
                               &data_ds, &event_system);

  std::atomic<int> count_a{0};
  std::atomic<int> count_b{0};

  CHECK(session_a.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      1300, [&count_a](const runtime::events::event_s &e) { count_a++; }));
  CHECK(session_b.subscribe_to_topic(runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 
      1300, [&count_b](const runtime::events::event_s &e) { count_b++; }));

  CHECK(session_a.publish_event(
      runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A, 1300,
      std::string("message")));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  CHECK(count_a.load() == 1);
  CHECK(count_b.load() == 1);

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}
