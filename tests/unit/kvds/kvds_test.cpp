#include <snitch/snitch.hpp>
#include <kvds/datastore.hpp>
#include <filesystem>
#include <map>
#include <thread>
#include <chrono>
#include <atomic>

namespace {
    void ensure_db_cleanup(const std::string& path) {
        std::filesystem::remove_all(path);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::string get_unique_test_path(const std::string& base) {
        static std::atomic<int> counter{0};
        return base + "_" + std::to_string(counter.fetch_add(1)) + "_" + 
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }
}

TEST_CASE("kvds basic operations", "[unit][kvds]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/kvds_test_basic");
    
    ensure_db_cleanup(test_db_path);
    
    SECTION("open and close database") {
        CHECK(ds.open(test_db_path));
        CHECK(ds.is_open());
        CHECK(ds.close());
        CHECK_FALSE(ds.is_open());
    }
    
    SECTION("basic set/get/exists operations") {
        CHECK(ds.open(test_db_path));
        
        CHECK(ds.set("key1", "value1"));
        
        std::string value;
        CHECK(ds.get("key1", value));
        CHECK(value == "value1");
        
        CHECK(ds.exists("key1"));
        CHECK_FALSE(ds.exists("nonexistent"));
        
        CHECK(ds.del("key1"));
        CHECK_FALSE(ds.exists("key1"));
        
        ds.close();
    }
    
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("kvds with manual prefix management", "[unit][kvds]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/kvds_test_prefix");
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    SECTION("manual prefix key construction") {
        CHECK(ds.set("users:alice:setting1", "value1"));
        CHECK(ds.set("users:alice:setting2", "value2"));
        
        std::string value;
        CHECK(ds.get("users:alice:setting1", value));
        CHECK(value == "value1");
        
        CHECK(ds.set("users:bob:setting1", "different_value"));
        CHECK(ds.get("users:bob:setting1", value));
        CHECK(value == "different_value");
        
        CHECK(ds.get("users:alice:setting1", value));
        CHECK(value == "value1");
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("kvds batch operations", "[unit][kvds]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/kvds_test_batch");
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    SECTION("batch set operations") {
        std::map<std::string, std::string> batch = {
            {"key1", "value1"},
            {"key2", "value2"},
            {"key3", "value3"}
        };
        
        CHECK(ds.set_batch(batch));
        
        std::string value;
        CHECK(ds.get("key1", value));
        CHECK(value == "value1");
        CHECK(ds.get("key2", value));
        CHECK(value == "value2");
        CHECK(ds.get("key3", value));
        CHECK(value == "value3");
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("kvds iteration", "[unit][kvds]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/kvds_test_iteration");
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    SECTION("iterate with prefix") {
        CHECK(ds.set("users:alice", "admin"));
        CHECK(ds.set("users:bob", "user"));
        CHECK(ds.set("users:charlie", "guest"));
        CHECK(ds.set("groups:admins", "alice,root"));
        CHECK(ds.set("groups:users", "bob,charlie"));
        
        std::map<std::string, std::string> found_pairs;
        
        ds.iterate("users:", [&found_pairs](const std::string& key, const std::string& value) {
            found_pairs[key] = value;
            return true;
        });
        
        CHECK(found_pairs.size() == 3);
        CHECK(found_pairs["users:alice"] == "admin");
        CHECK(found_pairs["users:bob"] == "user");
        CHECK(found_pairs["users:charlie"] == "guest");
        CHECK(found_pairs.find("groups:admins") == found_pairs.end());
    }
    
    SECTION("early termination in iteration") {
        CHECK(ds.set("test:item1", "data1"));
        CHECK(ds.set("test:item2", "data2"));
        CHECK(ds.set("test:item3", "data3"));
        
        int count = 0;
        ds.iterate("test:", [&count](const std::string& key, const std::string& value) {
            count++;
            return count < 2;
        });
        
        CHECK(count == 2);
    }
    
    SECTION("iterate all with empty prefix") {
        CHECK(ds.set("key1", "value1"));
        CHECK(ds.set("key2", "value2"));
        
        int count = 0;
        ds.iterate("", [&count](const std::string& key, const std::string& value) {
            count++;
            return true;
        });
        
        CHECK(count >= 2);
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("kvds error conditions", "[unit][kvds]") {
    kvds::datastore_c ds;
    
    SECTION("operations on closed database") {
        CHECK_FALSE(ds.is_open());
        CHECK_FALSE(ds.set("key", "value"));
        
        std::string value;
        CHECK_FALSE(ds.get("key", value));
        CHECK_FALSE(ds.del("key"));
        CHECK_FALSE(ds.exists("key"));
        
        std::map<std::string, std::string> batch = {{"key", "value"}};
        CHECK_FALSE(ds.set_batch(batch));
        
        ds.iterate("", [](const std::string& key, const std::string& value) {
            CHECK(false);
            return true;
        });
    }
    
    SECTION("double open/close") {
        std::string test_db_path = get_unique_test_path("/tmp/kvds_test_error");
        ensure_db_cleanup(test_db_path);
        
        CHECK(ds.open(test_db_path));
        CHECK_FALSE(ds.open(test_db_path));
        
        CHECK(ds.close());
        CHECK_FALSE(ds.close());
        
        ensure_db_cleanup(test_db_path);
    }
}
