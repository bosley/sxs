#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
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
    auto logger = spdlog::get("entity_test");
    if (!logger) {
        logger = spdlog::stdout_color_mt("entity_test");
    }
    return logger;
}
}

TEST_CASE("entity creation and basic properties", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_basic");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("entity creation with unique identifier") {
    auto entity = manager.get_or_create<runtime::entity_c>("user@example.com");
    CHECK(entity.has_value());
    CHECK((*entity)->get_id() == "user@example.com");
  }
  
  SECTION("entity type identification") {
    auto entity = manager.get_or_create<runtime::entity_c>("test_id");
    CHECK(entity.has_value());
    CHECK((*entity)->get_type_id() == "entity");
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity permission granting", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_grant");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("grant read permission") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope1", runtime::permission::READ_ONLY);
    CHECK((*entity)->is_permitted("scope1", runtime::permission::READ_ONLY));
  }
  
  SECTION("grant write permission") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope2", runtime::permission::WRITE_ONLY);
    CHECK((*entity)->is_permitted("scope2", runtime::permission::WRITE_ONLY));
  }
  
  SECTION("grant read-write permission") {
    auto entity = manager.get_or_create<runtime::entity_c>("user3");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope3", runtime::permission::READ_WRITE);
    CHECK((*entity)->is_permitted("scope3", runtime::permission::READ_WRITE));
  }
  
  SECTION("grant multiple permissions to different scopes") {
    auto entity = manager.get_or_create<runtime::entity_c>("user4");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope_a", runtime::permission::READ_ONLY);
    (*entity)->grant_permission("scope_b", runtime::permission::WRITE_ONLY);
    (*entity)->grant_permission("scope_c", runtime::permission::READ_WRITE);
    
    CHECK((*entity)->is_permitted("scope_a", runtime::permission::READ_ONLY));
    CHECK((*entity)->is_permitted("scope_b", runtime::permission::WRITE_ONLY));
    CHECK((*entity)->is_permitted("scope_c", runtime::permission::READ_WRITE));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity permission checking", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_check");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("read-write permission includes read") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("data", runtime::permission::READ_WRITE);
    CHECK((*entity)->is_permitted("data", runtime::permission::READ_ONLY));
  }
  
  SECTION("read-write permission includes write") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("data", runtime::permission::READ_WRITE);
    CHECK((*entity)->is_permitted("data", runtime::permission::WRITE_ONLY));
  }
  
  SECTION("read-only permission does not grant write") {
    auto entity = manager.get_or_create<runtime::entity_c>("user3");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("data", runtime::permission::READ_ONLY);
    CHECK_FALSE((*entity)->is_permitted("data", runtime::permission::WRITE_ONLY));
  }
  
  SECTION("write-only permission does not grant read") {
    auto entity = manager.get_or_create<runtime::entity_c>("user4");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("data", runtime::permission::WRITE_ONLY);
    CHECK_FALSE((*entity)->is_permitted("data", runtime::permission::READ_ONLY));
  }
  
  SECTION("no permission by default") {
    auto entity = manager.get_or_create<runtime::entity_c>("user5");
    CHECK(entity.has_value());
    
    CHECK_FALSE((*entity)->is_permitted("nonexistent", runtime::permission::READ_ONLY));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity permission revocation", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_revoke");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("revoke granted permission") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope1", runtime::permission::READ_ONLY);
    CHECK((*entity)->is_permitted("scope1", runtime::permission::READ_ONLY));
    
    (*entity)->revoke_permission("scope1");
    CHECK_FALSE((*entity)->is_permitted("scope1", runtime::permission::READ_ONLY));
  }
  
  SECTION("revoke one permission leaves others intact") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope_a", runtime::permission::READ_ONLY);
    (*entity)->grant_permission("scope_b", runtime::permission::WRITE_ONLY);
    
    (*entity)->revoke_permission("scope_a");
    
    CHECK_FALSE((*entity)->is_permitted("scope_a", runtime::permission::READ_ONLY));
    CHECK((*entity)->is_permitted("scope_b", runtime::permission::WRITE_ONLY));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity permission persistence", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_persist");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("permissions persist across save and load") {
    {
      auto entity = manager.get_or_create<runtime::entity_c>("persistent_user");
      CHECK(entity.has_value());
      
      (*entity)->grant_permission("api", runtime::permission::READ_WRITE);
      (*entity)->grant_permission("database", runtime::permission::READ_ONLY);
      (*entity)->grant_permission("logs", runtime::permission::WRITE_ONLY);
      
      CHECK((*entity)->save());
    }
    
    auto entity = manager.get_or_create<runtime::entity_c>("persistent_user");
    CHECK(entity.has_value());
    
    CHECK((*entity)->is_permitted("api", runtime::permission::READ_WRITE));
    CHECK((*entity)->is_permitted("database", runtime::permission::READ_ONLY));
    CHECK((*entity)->is_permitted("logs", runtime::permission::WRITE_ONLY));
  }
  
  SECTION("permission updates persist") {
    {
      auto entity = manager.get_or_create<runtime::entity_c>("update_user");
      (*entity)->grant_permission("scope", runtime::permission::READ_ONLY);
      (*entity)->save();
    }
    
    {
      auto entity = manager.get_or_create<runtime::entity_c>("update_user");
      (*entity)->grant_permission("scope", runtime::permission::READ_WRITE);
      (*entity)->save();
    }
    
    auto entity = manager.get_or_create<runtime::entity_c>("update_user");
    CHECK((*entity)->is_permitted("scope", runtime::permission::READ_WRITE));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity get and set permissions", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_getset");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("get permissions returns map") {
    auto entity = manager.get_or_create<runtime::entity_c>("user1");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope1", runtime::permission::READ_ONLY);
    (*entity)->grant_permission("scope2", runtime::permission::WRITE_ONLY);
    
    auto perms = (*entity)->get_permissions();
    CHECK(perms.size() == 2);
    CHECK(perms["scope1"] == "R");
    CHECK(perms["scope2"] == "W");
  }
  
  SECTION("set permissions replaces all permissions") {
    auto entity = manager.get_or_create<runtime::entity_c>("user2");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("old_scope", runtime::permission::READ_ONLY);
    
    std::map<std::string, std::string> new_perms;
    new_perms["new_scope1"] = "R";
    new_perms["new_scope2"] = "RW";
    
    (*entity)->set_permissions(new_perms);
    
    CHECK_FALSE((*entity)->is_permitted("old_scope", runtime::permission::READ_ONLY));
    CHECK((*entity)->is_permitted("new_scope1", runtime::permission::READ_ONLY));
    CHECK((*entity)->is_permitted("new_scope2", runtime::permission::READ_WRITE));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("multiple entities with different permissions", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_multi");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("different users have independent permissions") {
    auto user1 = manager.get_or_create<runtime::entity_c>("alice@example.com");
    auto user2 = manager.get_or_create<runtime::entity_c>("bob@example.com");
    
    CHECK(user1.has_value());
    CHECK(user2.has_value());
    
    (*user1)->grant_permission("project_a", runtime::permission::READ_WRITE);
    (*user2)->grant_permission("project_b", runtime::permission::READ_ONLY);
    
    CHECK((*user1)->is_permitted("project_a", runtime::permission::READ_WRITE));
    CHECK_FALSE((*user1)->is_permitted("project_b", runtime::permission::READ_ONLY));
    
    CHECK((*user2)->is_permitted("project_b", runtime::permission::READ_ONLY));
    CHECK_FALSE((*user2)->is_permitted("project_a", runtime::permission::READ_WRITE));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity permission edge cases", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_edge");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("empty permissions by default") {
    auto entity = manager.get_or_create<runtime::entity_c>("empty_user");
    CHECK(entity.has_value());
    
    auto perms = (*entity)->get_permissions();
    CHECK(perms.empty());
  }
  
  SECTION("overwriting existing permission") {
    auto entity = manager.get_or_create<runtime::entity_c>("override_user");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope", runtime::permission::READ_ONLY);
    (*entity)->grant_permission("scope", runtime::permission::READ_WRITE);
    
    CHECK((*entity)->is_permitted("scope", runtime::permission::READ_WRITE));
    CHECK((*entity)->is_permitted("scope", runtime::permission::READ_ONLY));
  }
  
  SECTION("revoking non-existent permission") {
    auto entity = manager.get_or_create<runtime::entity_c>("revoke_user");
    CHECK(entity.has_value());
    
    (*entity)->revoke_permission("nonexistent");
    
    auto perms = (*entity)->get_permissions();
    CHECK(perms.empty());
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

TEST_CASE("entity deletion", "[unit][runtime][entity]") {
  kvds::datastore_c ds;
  std::string test_db_path = get_unique_test_path("/tmp/entity_test_delete");
  auto logger = create_test_logger();
  
  ensure_db_cleanup(test_db_path);
  CHECK(ds.open(test_db_path));
  
  record::record_manager_c manager(ds, logger);
  
  SECTION("delete entity removes it from storage") {
    auto entity = manager.get_or_create<runtime::entity_c>("delete_user");
    CHECK(entity.has_value());
    
    (*entity)->grant_permission("scope", runtime::permission::READ_WRITE);
    (*entity)->save();
    
    CHECK(manager.exists("entity", "delete_user"));
    
    CHECK((*entity)->del());
    
    CHECK_FALSE(manager.exists("entity", "delete_user"));
  }
  
  ds.close();
  ensure_db_cleanup(test_db_path);
}

