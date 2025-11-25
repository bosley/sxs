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
#include <vector>

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
  auto logger = spdlog::get("entity_rps_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("entity_rps_test");
  }
  return logger;
}
} // namespace

TEST_CASE("entity rps basic set and get", "[unit][runtime][entity][rps]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_rps_basic");
  auto logger = create_test_logger();

  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));

  record::record_manager_c manager(ds, logger);

  SECTION("default rps is zero (unlimited)") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    CHECK((*entity)->get_max_rps() == 0);
  }

  SECTION("set and get max rps") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());

    (*entity)->set_max_rps(100);
    CHECK((*entity)->get_max_rps() == 100);

    (*entity)->set_max_rps(50);
    CHECK((*entity)->get_max_rps() == 50);

    (*entity)->set_max_rps(0);
    CHECK((*entity)->get_max_rps() == 0);
  }

  SECTION("max rps persists to storage") {
    {
      auto entity = manager.get_or_create<runtime::entity_c>("persistent_user");
      CHECK(entity.has_value());
      (*entity)->set_max_rps(200);
      CHECK((*entity)->save());
    }

    auto entity = manager.get_or_create<runtime::entity_c>("persistent_user");
    CHECK(entity.has_value());
    CHECK((*entity)->get_max_rps() == 200);
  }

  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity rps unlimited publishing", "[unit][runtime][entity][rps]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_rps_unlimited");
  auto logger = create_test_logger();

  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));

  record::record_manager_c manager(ds, logger);

  SECTION("can publish unlimited when rps is zero") {
    auto entity = manager.get_or_create<runtime::entity_c>("unlimited_user");
    CHECK(entity.has_value());
    CHECK((*entity)->get_max_rps() == 0);

    for (int i = 0; i < 1000; ++i) {
      CHECK((*entity)->try_publish());
    }
  }

  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity rps single session rate limiting",
          "[unit][runtime][entity][rps]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_single_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_single_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("can publish exactly max_rps times") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->set_max_rps(10);

    for (int i = 0; i < 10; ++i) {
      CHECK(entity->try_publish());
    }

    CHECK_FALSE(entity->try_publish());
  }

  SECTION("rate limit resets after one second") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->set_max_rps(5);

    for (int i = 0; i < 5; ++i) {
      CHECK(entity->try_publish());
    }

    CHECK_FALSE(entity->try_publish());

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    CHECK(entity->try_publish());
  }

  SECTION("can publish up to limit then block") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user3");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->set_max_rps(3);

    CHECK(entity->try_publish());
    CHECK(entity->try_publish());
    CHECK(entity->try_publish());
    CHECK_FALSE(entity->try_publish());
    CHECK_FALSE(entity->try_publish());
    CHECK_FALSE(entity->try_publish());
  }

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("entity rps multiple sessions share limit",
          "[unit][runtime][entity][rps]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_multi_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_multi_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("multiple sessions for same entity share rps limit") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto shared_entity = entity_opt.value().get();

    shared_entity->set_max_rps(10);
    shared_entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    shared_entity->grant_permission("scope2", runtime::permission::READ_WRITE);
    shared_entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    shared_entity->save();

    runtime::session_c session1("sess1", "user1", "scope1", *shared_entity,
                                &data_ds, &event_system);
    runtime::session_c session2("sess2", "user1", "scope2", *shared_entity,
                                &data_ds, &event_system);

    int total_published = 0;

    for (int i = 0; i < 5; ++i) {
      if (session1.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              i) == runtime::publish_result_e::OK) {
        total_published++;
      }
    }

    for (int i = 0; i < 5; ++i) {
      if (session2.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              i) == runtime::publish_result_e::OK) {
        total_published++;
      }
    }

    CHECK(total_published == 10);

    CHECK(session1.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              99) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
    CHECK(session2.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              99) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
  }

  SECTION("five sessions share 10 rps limit") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity_opt.has_value());
    auto shared_entity = entity_opt.value().get();

    shared_entity->set_max_rps(10);
    shared_entity->grant_permission("scope_a", runtime::permission::READ_WRITE);
    shared_entity->grant_permission("scope_b", runtime::permission::READ_WRITE);
    shared_entity->grant_permission("scope_c", runtime::permission::READ_WRITE);
    shared_entity->grant_permission("scope_d", runtime::permission::READ_WRITE);
    shared_entity->grant_permission("scope_e", runtime::permission::READ_WRITE);
    shared_entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    shared_entity->save();

    runtime::session_c sess1("s1", "user2", "scope_a", *shared_entity, &data_ds,
                             &event_system);
    runtime::session_c sess2("s2", "user2", "scope_b", *shared_entity, &data_ds,
                             &event_system);
    runtime::session_c sess3("s3", "user2", "scope_c", *shared_entity, &data_ds,
                             &event_system);
    runtime::session_c sess4("s4", "user2", "scope_d", *shared_entity, &data_ds,
                             &event_system);
    runtime::session_c sess5("s5", "user2", "scope_e", *shared_entity, &data_ds,
                             &event_system);

    std::vector<runtime::session_c *> sessions = {&sess1, &sess2, &sess3,
                                                  &sess4, &sess5};

    int total_published = 0;
    for (int round = 0; round < 5; ++round) {
      for (auto *sess : sessions) {
        if (sess->publish_event(
                runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
                round) == runtime::publish_result_e::OK) {
          total_published++;
        }
      }
    }

    CHECK(total_published == 10);

    for (auto *sess : sessions) {
      CHECK(sess->publish_event(
                runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
                999) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
    }
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("entity rps concurrent multi-threaded publishing",
          "[unit][runtime][entity][rps]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_concurrent_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_concurrent_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("concurrent can_publish calls are thread-safe") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto shared_entity = entity_opt.value().get();

    shared_entity->set_max_rps(100);

    std::atomic<int> successful_publishes{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 10; ++t) {
      threads.emplace_back([shared_entity, &successful_publishes]() {
        for (int i = 0; i < 20; ++i) {
          if (shared_entity->try_publish()) {
            successful_publishes.fetch_add(1);
          }
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(successful_publishes.load() == 100);

    CHECK_FALSE(shared_entity->try_publish());
  }

  SECTION("concurrent publishing from multiple sessions") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity_opt.has_value());
    auto shared_entity = entity_opt.value().get();

    shared_entity->set_max_rps(50);
    for (int i = 0; i < 5; ++i) {
      std::string scope = "scope_" + std::to_string(i);
      shared_entity->grant_permission(scope, runtime::permission::READ_WRITE);
    }
    shared_entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    shared_entity->save();

    std::vector<std::unique_ptr<runtime::session_c>> sessions;
    for (int i = 0; i < 5; ++i) {
      std::string sess_id = "sess_" + std::to_string(i);
      std::string scope = "scope_" + std::to_string(i);
      sessions.emplace_back(std::make_unique<runtime::session_c>(
          sess_id, "user2", scope, *shared_entity, &data_ds, &event_system));
    }

    std::atomic<int> successful_publishes{0};
    std::vector<std::thread> threads;

    for (size_t i = 0; i < sessions.size(); ++i) {
      threads.emplace_back([&session = *sessions[i], &successful_publishes]() {
        for (int j = 0; j < 20; ++j) {
          if (session.publish_event(
                  runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST,
                  1, j) == runtime::publish_result_e::OK) {
            successful_publishes.fetch_add(1);
          }
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(successful_publishes.load() == 50);

    for (auto &session : sessions) {
      CHECK(session->publish_event(
                runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
                999) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
    }
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("entity rps sliding window behavior",
          "[unit][runtime][entity][rps]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_rps_sliding");
  auto logger = create_test_logger();

  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));

  record::record_manager_c manager(ds, logger);

  SECTION("old timestamps get cleaned up") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(5);

    for (int i = 0; i < 5; ++i) {
      CHECK((*entity)->try_publish());
    }

    CHECK_FALSE((*entity)->try_publish());

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    CHECK_FALSE((*entity)->try_publish());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    CHECK((*entity)->try_publish());
  }

  SECTION("gradual timestamp expiration") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(10);

    for (int i = 0; i < 10; ++i) {
      CHECK((*entity)->try_publish());
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    CHECK_FALSE((*entity)->try_publish());

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    int available_slots = 0;
    for (int i = 0; i < 20; ++i) {
      if ((*entity)->try_publish()) {
        available_slots++;
      }
    }

    CHECK(available_slots > 0);
    CHECK(available_slots < 10);
  }

  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity rps edge cases", "[unit][runtime][entity][rps]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_rps_edge");
  auto logger = create_test_logger();

  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));

  record::record_manager_c manager(ds, logger);

  SECTION("rps limit of 1") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(1);

    CHECK((*entity)->try_publish());
    CHECK_FALSE((*entity)->try_publish());

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    CHECK((*entity)->try_publish());
  }

  SECTION("very high rps limit") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(10000);

    for (int i = 0; i < 10000; ++i) {
      CHECK((*entity)->try_publish());
    }

    CHECK_FALSE((*entity)->try_publish());
  }

  SECTION("changing rps limit mid-operation") {
    auto entity = manager.get_or_create<runtime::entity_c>("user3");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(5);

    for (int i = 0; i < 3; ++i) {
      CHECK((*entity)->try_publish());
    }

    (*entity)->set_max_rps(10);

    for (int i = 0; i < 7; ++i) {
      CHECK((*entity)->try_publish());
    }

    CHECK_FALSE((*entity)->try_publish());
  }

  SECTION("changing rps from limited to unlimited") {
    auto entity = manager.get_or_create<runtime::entity_c>("user4");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(2);

    CHECK((*entity)->try_publish());
    CHECK((*entity)->try_publish());
    CHECK_FALSE((*entity)->try_publish());

    (*entity)->set_max_rps(0);

    for (int i = 0; i < 100; ++i) {
      CHECK((*entity)->try_publish());
    }
  }

  SECTION("try_publish atomically checks and records") {
    auto entity = manager.get_or_create<runtime::entity_c>("user5");
    CHECK(entity.has_value());
    (*entity)->set_max_rps(5);

    for (int i = 0; i < 5; ++i) {
      CHECK((*entity)->try_publish());
    }

    CHECK_FALSE((*entity)->try_publish());
  }

  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity rps with permission blocking",
          "[unit][runtime][entity][rps]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_perm_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_perm_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("rps limit checked before permission") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());

    entity->set_max_rps(2);
    entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity->save();

    runtime::session_c session("sess1", "user1", "scope1", *entity.get(),
                               &data_ds, &event_system);

    CHECK(session.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              0) == runtime::publish_result_e::PERMISSION_DENIED);
  }

  SECTION("both rps and permission must pass") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());

    entity->set_max_rps(10);
    entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    entity->save();

    runtime::session_c session("sess1", "user2", "scope1", *entity.get(),
                               &data_ds, &event_system);

    for (int i = 0; i < 10; ++i) {
      CHECK(session.publish_event(
                runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
                i) == runtime::publish_result_e::OK);
    }

    CHECK(session.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              99) == runtime::publish_result_e::RATE_LIMIT_EXCEEDED);
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("entity rps stress test with rapid publishes",
          "[unit][runtime][entity][rps][stress]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_stress_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_stress_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("rapid fire publishing respects limits") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto shared_entity = entity_opt.value().get();

    shared_entity->set_max_rps(100);
    shared_entity->grant_permission("scope1", runtime::permission::READ_WRITE);
    shared_entity->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    shared_entity->save();

    runtime::session_c session("sess1", "user1", "scope1", *shared_entity,
                               &data_ds, &event_system);

    int successful = 0;
    int failed = 0;

    for (int i = 0; i < 200; ++i) {
      if (session.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              i) == runtime::publish_result_e::OK) {
        successful++;
      } else {
        failed++;
      }
    }

    CHECK(successful == 100);
    CHECK(failed == 100);
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("entity rps different entities independent limits",
          "[unit][runtime][entity][rps]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/entity_rps_independent_entity");
  std::string data_test_path =
      get_unique_test_path("/tmp/entity_rps_independent_data");
  auto logger = create_test_logger();

  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);

  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));

  runtime::events::event_system_c event_system(logger.get());
  event_system.initialize(nullptr);

  record::record_manager_c entity_manager(entity_ds, logger);

  SECTION("different entities have independent rps limits") {
    auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity1_opt.has_value());
    REQUIRE(entity2_opt.has_value());
    auto entity1 = entity1_opt.value().get();
    auto entity2 = entity2_opt.value().get();

    entity1->set_max_rps(5);
    entity1->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity1->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    entity1->save();

    entity2->set_max_rps(10);
    entity2->grant_permission("scope2", runtime::permission::READ_WRITE);
    entity2->grant_topic_permission(1, runtime::topic_permission::PUBSUB);
    entity2->save();

    runtime::session_c session1("sess1", "user1", "scope1", *entity1, &data_ds,
                                &event_system);
    runtime::session_c session2("sess2", "user2", "scope2", *entity2, &data_ds,
                                &event_system);

    int entity1_publishes = 0;
    for (int i = 0; i < 10; ++i) {
      if (session1.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              i) == runtime::publish_result_e::OK) {
        entity1_publishes++;
      }
    }

    int entity2_publishes = 0;
    for (int i = 0; i < 15; ++i) {
      if (session2.publish_event(
              runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST, 1,
              i) == runtime::publish_result_e::OK) {
        entity2_publishes++;
      }
    }

    CHECK(entity1_publishes == 5);
    CHECK(entity2_publishes == 10);
  }

  event_system.shutdown();
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}
