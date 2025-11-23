#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
#include <runtime/session/session.hpp>
#include <runtime/entity/entity.hpp>
#include <record/record.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
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
    auto logger = spdlog::get("session_test");
    if (!logger) {
        logger = spdlog::stdout_color_mt("session_test");
    }
    return logger;
}
}

TEST_CASE("session subsystem initialization", "[unit][runtime][session]") {
  auto logger = create_test_logger();
  
  runtime::session_subsystem_c subsystem(logger.get(), 5);
  
  SECTION("subsystem starts not running") {
    CHECK_FALSE(subsystem.is_running());
  }
  
  SECTION("subsystem can be initialized") {
    CHECK(std::string(subsystem.get_name()) == "session_subsystem_c");
  }
}

TEST_CASE("session creation with entity and scope", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("test_user");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();
  
  SECTION("session can be created with valid parameters") {
    runtime::session_c session("test_user_session_0", "test_user", "test_scope", entity.get(), &data_ds);
    
    CHECK(session.get_id() == "test_user_session_0");
    CHECK(session.get_entity_id() == "test_user");
    CHECK(session.get_scope() == "test_scope");
    CHECK(session.is_active());
  }
  
  SECTION("session provides scoped kv store") {
    runtime::session_c session("test_user_session_0", "test_user", "test_scope", entity.get(), &data_ds);
    
    auto* store = session.get_store();
    REQUIRE(store != nullptr);
    CHECK(store->is_open());
  }
  
  SECTION("scoped kv store respects permissions") {
    runtime::session_c session("test_user_session_0", "test_user", "test_scope", entity.get(), &data_ds);
    
    auto* store = session.get_store();
    REQUIRE(store != nullptr);
    
    CHECK(store->set("key1", "value1"));
    
    std::string value;
    CHECK(store->get("key1", value));
    CHECK(value == "value1");
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("scoped kv operations with permission checks", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_perm_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_perm_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  
  SECTION("read-only permission blocks writes") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("readonly_user");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("readonly_scope", runtime::permission::READ_ONLY);
    entity->save();
    
    runtime::session_c session("readonly_session", "readonly_user", "readonly_scope", entity.get(), &data_ds);
    auto* store = session.get_store();
    REQUIRE(store != nullptr);
    
    CHECK_FALSE(store->set("key1", "value1"));
  }
  
  SECTION("write-only permission blocks reads") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("writeonly_user");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("writeonly_scope", runtime::permission::WRITE_ONLY);
    entity->save();
    
    data_ds.set("writeonly_scope/key1", "value1");
    
    runtime::session_c session("writeonly_session", "writeonly_user", "writeonly_scope", entity.get(), &data_ds);
    auto* store = session.get_store();
    REQUIRE(store != nullptr);
    
    std::string value;
    CHECK_FALSE(store->get("key1", value));
  }
  
  SECTION("read-write permission allows both operations") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("rw_user");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("rw_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session("rw_session", "rw_user", "rw_scope", entity.get(), &data_ds);
    auto* store = session.get_store();
    REQUIRE(store != nullptr);
    
    CHECK(store->set("key1", "value1"));
    
    std::string value;
    CHECK(store->get("key1", value));
    CHECK(value == "value1");
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}
