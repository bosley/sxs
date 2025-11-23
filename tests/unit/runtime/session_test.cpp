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

TEST_CASE("explicit key scoping verification", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_scoping_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_scoping_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  
  SECTION("keys are prefixed with scope in underlying store") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("my_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session("sess1", "user1", "my_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("key1", "value1"));
    
    std::string direct_value;
    CHECK(data_ds.get("my_scope/key1", direct_value));
    CHECK(direct_value == "value1");
    
    CHECK_FALSE(data_ds.get("key1", direct_value));
  }
  
  SECTION("different scopes don't interfere") {
    auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity1_opt.has_value());
    REQUIRE(entity2_opt.has_value());
    auto entity1 = std::move(entity1_opt.value());
    auto entity2 = std::move(entity2_opt.value());
    
    entity1->grant_permission("scope_a", runtime::permission::READ_WRITE);
    entity2->grant_permission("scope_b", runtime::permission::READ_WRITE);
    entity1->save();
    entity2->save();
    
    runtime::session_c session1("sess1", "user1", "scope_a", entity1.get(), &data_ds, nullptr);
    runtime::session_c session2("sess2", "user2", "scope_b", entity2.get(), &data_ds, nullptr);
    
    auto* store1 = session1.get_store();
    auto* store2 = session2.get_store();
    
    CHECK(store1->set("shared_key", "value_from_scope_a"));
    CHECK(store2->set("shared_key", "value_from_scope_b"));
    
    std::string val1, val2;
    CHECK(store1->get("shared_key", val1));
    CHECK(store2->get("shared_key", val2));
    CHECK(val1 == "value_from_scope_a");
    CHECK(val2 == "value_from_scope_b");
    
    std::string direct_val_a, direct_val_b;
    CHECK(data_ds.get("scope_a/shared_key", direct_val_a));
    CHECK(data_ds.get("scope_b/shared_key", direct_val_b));
    CHECK(direct_val_a == "value_from_scope_a");
    CHECK(direct_val_b == "value_from_scope_b");
  }
  
  SECTION("multiple sessions same scope share data") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("shared_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session1("sess1", "user1", "shared_scope", entity.get(), &data_ds, nullptr);
    runtime::session_c session2("sess2", "user1", "shared_scope", entity.get(), &data_ds, nullptr);
    
    CHECK(session1.get_store()->set("key1", "from_session1"));
    
    std::string value;
    CHECK(session2.get_store()->get("key1", value));
    CHECK(value == "from_session1");
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("key masking on iteration", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_masking_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_masking_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  
  SECTION("iteration returns unscoped keys") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("key1", "val1"));
    CHECK(store->set("key2", "val2"));
    CHECK(store->set("key3", "val3"));
    
    std::vector<std::string> keys;
    store->iterate("", [&keys](const std::string& key, const std::string& value) {
      keys.push_back(key);
      return true;
    });
    
    CHECK(keys.size() == 3);
    CHECK(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    CHECK(std::find(keys.begin(), keys.end(), "key2") != keys.end());
    CHECK(std::find(keys.begin(), keys.end(), "key3") != keys.end());
    CHECK(std::find(keys.begin(), keys.end(), "test_scope/key1") == keys.end());
  }
  
  SECTION("iteration with prefix filters within session scope") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("user:alice", "data1"));
    CHECK(store->set("user:bob", "data2"));
    CHECK(store->set("config:setting", "data3"));
    
    std::vector<std::string> user_keys;
    store->iterate("user:", [&user_keys](const std::string& key, const std::string& value) {
      user_keys.push_back(key);
      return true;
    });
    
    CHECK(user_keys.size() == 2);
    CHECK(std::find(user_keys.begin(), user_keys.end(), "user:alice") != user_keys.end());
    CHECK(std::find(user_keys.begin(), user_keys.end(), "user:bob") != user_keys.end());
    CHECK(std::find(user_keys.begin(), user_keys.end(), "config:setting") == user_keys.end());
  }
  
  SECTION("iteration only sees keys from session scope") {
    auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity1_opt.has_value());
    REQUIRE(entity2_opt.has_value());
    auto entity1 = std::move(entity1_opt.value());
    auto entity2 = std::move(entity2_opt.value());
    
    entity1->grant_permission("scope_a", runtime::permission::READ_WRITE);
    entity2->grant_permission("scope_b", runtime::permission::READ_WRITE);
    entity1->save();
    entity2->save();
    
    runtime::session_c session1("sess1", "user1", "scope_a", entity1.get(), &data_ds, nullptr);
    runtime::session_c session2("sess2", "user2", "scope_b", entity2.get(), &data_ds, nullptr);
    
    CHECK(session1.get_store()->set("key1", "a1"));
    CHECK(session1.get_store()->set("key2", "a2"));
    CHECK(session2.get_store()->set("key1", "b1"));
    CHECK(session2.get_store()->set("key3", "b3"));
    
    std::vector<std::string> keys_a;
    session1.get_store()->iterate("", [&keys_a](const std::string& key, const std::string& value) {
      keys_a.push_back(key);
      return true;
    });
    
    std::vector<std::string> keys_b;
    session2.get_store()->iterate("", [&keys_b](const std::string& key, const std::string& value) {
      keys_b.push_back(key);
      return true;
    });
    
    CHECK(keys_a.size() == 2);
    CHECK(std::find(keys_a.begin(), keys_a.end(), "key1") != keys_a.end());
    CHECK(std::find(keys_a.begin(), keys_a.end(), "key2") != keys_a.end());
    CHECK(std::find(keys_a.begin(), keys_a.end(), "key3") == keys_a.end());
    
    CHECK(keys_b.size() == 2);
    CHECK(std::find(keys_b.begin(), keys_b.end(), "key1") != keys_b.end());
    CHECK(std::find(keys_b.begin(), keys_b.end(), "key3") != keys_b.end());
    CHECK(std::find(keys_b.begin(), keys_b.end(), "key2") == keys_b.end());
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session time tracking", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_time_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_time_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();
  
  SECTION("creation time is set") {
    auto before = std::time(nullptr);
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto after = std::time(nullptr);
    
    auto creation_time = session.get_creation_time();
    CHECK(creation_time >= before);
    CHECK(creation_time <= after);
  }
  
  SECTION("creation time doesn't change after operations") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto initial_time = session.get_creation_time();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    session.get_store()->set("key1", "value1");
    session.get_store()->set("key2", "value2");
    
    std::string value;
    session.get_store()->get("key1", value);
    
    auto time_after_ops = session.get_creation_time();
    CHECK(time_after_ops == initial_time);
  }
  
  SECTION("multiple sessions have different creation times") {
    runtime::session_c session1("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto time1 = session1.get_creation_time();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    runtime::session_c session2("sess2", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto time2 = session2.get_creation_time();
    
    CHECK(time2 >= time1);
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("comprehensive kv operations", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_ops_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_ops_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();
  
  SECTION("exists checks scoped keys") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK_FALSE(store->exists("key1"));
    CHECK(store->set("key1", "value1"));
    CHECK(store->exists("key1"));
    
    CHECK(data_ds.exists("test_scope/key1"));
    CHECK_FALSE(data_ds.exists("key1"));
  }
  
  SECTION("del removes scoped keys") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("key1", "value1"));
    CHECK(store->exists("key1"));
    CHECK(store->del("key1"));
    CHECK_FALSE(store->exists("key1"));
    
    CHECK_FALSE(data_ds.exists("test_scope/key1"));
  }
  
  SECTION("set_batch with scoping") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    std::map<std::string, std::string> batch = {
      {"key1", "val1"},
      {"key2", "val2"},
      {"key3", "val3"}
    };
    
    CHECK(store->set_batch(batch));
    
    std::string val;
    CHECK(store->get("key1", val));
    CHECK(val == "val1");
    CHECK(store->get("key2", val));
    CHECK(val == "val2");
    CHECK(store->get("key3", val));
    CHECK(val == "val3");
    
    CHECK(data_ds.exists("test_scope/key1"));
    CHECK(data_ds.exists("test_scope/key2"));
    CHECK(data_ds.exists("test_scope/key3"));
  }
  
  SECTION("special characters in keys") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("key with spaces", "value1"));
    CHECK(store->set("key:with:colons", "value2"));
    CHECK(store->set("key_with_underscores", "value3"));
    CHECK(store->set("key-with-dashes", "value4"));
    
    std::string val;
    CHECK(store->get("key with spaces", val));
    CHECK(val == "value1");
    CHECK(store->get("key:with:colons", val));
    CHECK(val == "value2");
    CHECK(store->get("key_with_underscores", val));
    CHECK(val == "value3");
    CHECK(store->get("key-with-dashes", val));
    CHECK(val == "value4");
  }
  
  SECTION("long key names") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    std::string long_key(1000, 'x');
    CHECK(store->set(long_key, "value"));
    
    std::string val;
    CHECK(store->get(long_key, val));
    CHECK(val == "value");
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("permission boundary tests", "[unit][runtime][session]") {
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
  
  SECTION("no permissions blocks all operations") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK_FALSE(store->set("key1", "value1"));
    
    std::string value;
    CHECK_FALSE(store->get("key1", value));
    CHECK_FALSE(store->exists("key1"));
    CHECK_FALSE(store->del("key1"));
    
    std::map<std::string, std::string> batch = {{"key1", "val1"}};
    CHECK_FALSE(store->set_batch(batch));
  }
  
  SECTION("read-only permission blocks writes") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("readonly_user");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("readonly_scope", runtime::permission::READ_ONLY);
    entity->save();
    
    data_ds.set("readonly_scope/existing_key", "existing_value");
    
    runtime::session_c session("readonly_session", "readonly_user", "readonly_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK_FALSE(store->set("key1", "value1"));
    CHECK_FALSE(store->del("existing_key"));
    
    std::map<std::string, std::string> batch = {{"key1", "val1"}};
    CHECK_FALSE(store->set_batch(batch));
    
    std::string value;
    CHECK(store->get("existing_key", value));
    CHECK(value == "existing_value");
    CHECK(store->exists("existing_key"));
  }
  
  SECTION("write-only permission blocks reads") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("writeonly_user");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("writeonly_scope", runtime::permission::WRITE_ONLY);
    entity->save();
    
    data_ds.set("writeonly_scope/key1", "value1");
    
    runtime::session_c session("writeonly_session", "writeonly_user", "writeonly_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    std::string value;
    CHECK_FALSE(store->get("key1", value));
    CHECK_FALSE(store->exists("key1"));
    
    CHECK(store->set("key2", "value2"));
    CHECK(store->del("key1"));
  }
  
  SECTION("read-write permission allows all operations") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("rw_user");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    entity->grant_permission("rw_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session("rw_session", "rw_user", "rw_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("key1", "value1"));
    
    std::string value;
    CHECK(store->get("key1", value));
    CHECK(value == "value1");
    CHECK(store->exists("key1"));
    CHECK(store->del("key1"));
    CHECK_FALSE(store->exists("key1"));
    
    std::map<std::string, std::string> batch = {
      {"key2", "val2"},
      {"key3", "val3"}
    };
    CHECK(store->set_batch(batch));
    CHECK(store->exists("key2"));
    CHECK(store->exists("key3"));
  }
  
  SECTION("permission checks happen before operations") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK_FALSE(store->set("key1", "value1"));
    
    CHECK_FALSE(data_ds.exists("test_scope/key1"));
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session edge cases", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_edge_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_edge_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->save();
  
  SECTION("session metadata accessors") {
    runtime::session_c session("sess123", "user456", "scope789", entity.get(), &data_ds, nullptr);
    
    CHECK(session.get_id() == "sess123");
    CHECK(session.get_entity_id() == "user456");
    CHECK(session.get_scope() == "scope789");
    CHECK(session.is_active());
  }
  
  SECTION("session active state management") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    
    CHECK(session.is_active());
    session.set_active(false);
    CHECK_FALSE(session.is_active());
    session.set_active(true);
    CHECK(session.is_active());
  }
  
  SECTION("operations work regardless of active state") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    auto* store = session.get_store();
    
    CHECK(store->set("key1", "value1"));
    
    session.set_active(false);
    
    std::string value;
    CHECK(store->get("key1", value));
    CHECK(value == "value1");
    
    CHECK(store->set("key2", "value2"));
    CHECK(store->exists("key2"));
  }
  
  SECTION("get_store returns consistent pointer") {
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    
    auto* store1 = session.get_store();
    auto* store2 = session.get_store();
    
    CHECK(store1 == store2);
    CHECK(store1 != nullptr);
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session subsystem management", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c session_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_mgmt_entity");
  std::string session_test_path = get_unique_test_path("/tmp/session_test_mgmt_session");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_mgmt_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(session_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(session_ds.open(session_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  
  SECTION("list sessions returns empty initially") {
    runtime::session_subsystem_c subsystem(logger.get(), 5);
    
    auto sessions = subsystem.list_sessions();
    CHECK(sessions.empty());
  }
  
  SECTION("list sessions by entity returns empty initially") {
    runtime::session_subsystem_c subsystem(logger.get(), 5);
    
    auto sessions = subsystem.list_sessions_by_entity("user1");
    CHECK(sessions.empty());
  }
  
  SECTION("get session returns nullptr for non-existent session") {
    runtime::session_subsystem_c subsystem(logger.get(), 5);
    
    auto session = subsystem.get_session("non_existent");
    CHECK(session == nullptr);
  }
  
  SECTION("close session returns false for non-existent session") {
    runtime::session_subsystem_c subsystem(logger.get(), 5);
    
    CHECK_FALSE(subsystem.close_session("non_existent"));
  }
  
  SECTION("destroy session returns false for non-existent session") {
    runtime::session_subsystem_c subsystem(logger.get(), 5);
    
    CHECK_FALSE(subsystem.destroy_session("non_existent"));
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(session_test_path);
  ensure_db_cleanup(data_test_path);
}

TEST_CASE("session lifecycle management", "[unit][runtime][session]") {
  kvds::datastore_c entity_ds;
  kvds::datastore_c data_ds;
  std::string entity_test_path = get_unique_test_path("/tmp/session_test_lifecycle_entity");
  std::string data_test_path = get_unique_test_path("/tmp/session_test_lifecycle_data");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
  
  CHECK(entity_ds.open(entity_test_path));
  CHECK(data_ds.open(data_test_path));
  
  record::record_manager_c entity_manager(entity_ds, logger);
  
  SECTION("session active state toggles") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    
    CHECK(session.is_active());
    
    session.set_active(false);
    CHECK_FALSE(session.is_active());
    
    session.set_active(true);
    CHECK(session.is_active());
  }
  
  SECTION("multiple sessions can be tracked") {
    auto entity1_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    auto entity2_opt = entity_manager.get_or_create<runtime::entity_c>("user2");
    REQUIRE(entity1_opt.has_value());
    REQUIRE(entity2_opt.has_value());
    auto entity1 = std::move(entity1_opt.value());
    auto entity2 = std::move(entity2_opt.value());
    
    entity1->grant_permission("scope1", runtime::permission::READ_WRITE);
    entity2->grant_permission("scope2", runtime::permission::READ_WRITE);
    entity1->save();
    entity2->save();
    
    runtime::session_c session1("sess1", "user1", "scope1", entity1.get(), &data_ds, nullptr);
    runtime::session_c session2("sess2", "user2", "scope2", entity2.get(), &data_ds, nullptr);
    runtime::session_c session3("sess3", "user1", "scope1", entity1.get(), &data_ds, nullptr);
    
    CHECK(session1.get_id() == "sess1");
    CHECK(session2.get_id() == "sess2");
    CHECK(session3.get_id() == "sess3");
    
    CHECK(session1.get_entity_id() == "user1");
    CHECK(session2.get_entity_id() == "user2");
    CHECK(session3.get_entity_id() == "user1");
  }
  
  SECTION("sessions maintain independent state") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    runtime::session_c session1("sess1", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    runtime::session_c session2("sess2", "user1", "test_scope", entity.get(), &data_ds, nullptr);
    
    session1.get_store()->set("key1", "from_sess1");
    session2.get_store()->set("key2", "from_sess2");
    
    std::string val;
    CHECK(session1.get_store()->get("key1", val));
    CHECK(val == "from_sess1");
    CHECK(session2.get_store()->get("key1", val));
    CHECK(val == "from_sess1");
    
    CHECK(session1.get_store()->get("key2", val));
    CHECK(val == "from_sess2");
    CHECK(session2.get_store()->get("key2", val));
    CHECK(val == "from_sess2");
    
    session1.set_active(false);
    CHECK_FALSE(session1.is_active());
    CHECK(session2.is_active());
    
    CHECK(session1.get_store()->set("key3", "still_works"));
    CHECK(session2.get_store()->get("key3", val));
    CHECK(val == "still_works");
  }
  
  SECTION("session data persists in scoped datastore") {
    auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
    REQUIRE(entity_opt.has_value());
    auto entity = std::move(entity_opt.value());
    entity->grant_permission("persistent_scope", runtime::permission::READ_WRITE);
    entity->save();
    
    {
      runtime::session_c session("sess1", "user1", "persistent_scope", entity.get(), &data_ds, nullptr);
      session.get_store()->set("persistent_key", "persistent_value");
    }
    
    std::string value;
    CHECK(data_ds.get("persistent_scope/persistent_key", value));
    CHECK(value == "persistent_value");
    
    {
      runtime::session_c new_session("sess2", "user1", "persistent_scope", entity.get(), &data_ds, nullptr);
      CHECK(new_session.get_store()->get("persistent_key", value));
      CHECK(value == "persistent_value");
    }
  }
  
  ensure_db_cleanup(entity_test_path);
  ensure_db_cleanup(data_test_path);
}
