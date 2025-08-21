#include <snitch/snitch.hpp>
#include <kvds/memstore.hpp>
#include <thread>
#include <vector>

TEST_CASE("memstore basic operations", "[unit][kvds][memstore]") {
    kvds::memstore_c ms;
    
    SECTION("open and close") {
        CHECK(ms.open("dummy_path"));
        CHECK(ms.is_open());
        CHECK_FALSE(ms.open("another_path"));
        CHECK(ms.close());
        CHECK_FALSE(ms.is_open());
        CHECK_FALSE(ms.close());
    }
    
    SECTION("basic set/get/exists operations") {
        CHECK(ms.open(""));
        
        CHECK(ms.set("key1", "value1"));
        CHECK(ms.set("key2", "value2"));
        
        std::string value;
        CHECK(ms.get("key1", value));
        CHECK(value == "value1");
        
        CHECK(ms.get("key2", value));
        CHECK(value == "value2");
        
        CHECK(ms.exists("key1"));
        CHECK(ms.exists("key2"));
        CHECK_FALSE(ms.exists("key3"));
        
        CHECK(ms.del("key1"));
        CHECK_FALSE(ms.exists("key1"));
        CHECK_FALSE(ms.del("key1"));
        
        ms.close();
    }
}

TEST_CASE("memstore batch operations", "[unit][kvds][memstore]") {
    kvds::memstore_c ms;
    CHECK(ms.open(""));
    
    SECTION("batch set") {
        std::map<std::string, std::string> batch = {
            {"batch1", "value1"},
            {"batch2", "value2"},
            {"batch3", "value3"}
        };
        
        CHECK(ms.set_batch(batch));
        
        std::string value;
        CHECK(ms.get("batch1", value));
        CHECK(value == "value1");
        CHECK(ms.get("batch2", value));
        CHECK(value == "value2");
        CHECK(ms.get("batch3", value));
        CHECK(value == "value3");
    }
    
    ms.close();
}

TEST_CASE("memstore iteration", "[unit][kvds][memstore]") {
    kvds::memstore_c ms;
    CHECK(ms.open(""));
    
    SECTION("prefix iteration") {
        CHECK(ms.set("user:alice", "admin"));
        CHECK(ms.set("user:bob", "member"));
        CHECK(ms.set("user:charlie", "guest"));
        CHECK(ms.set("group:admins", "alice"));
        CHECK(ms.set("group:members", "bob,charlie"));
        
        std::map<std::string, std::string> found_users;
        ms.iterate("user:", [&found_users](const std::string& key, const std::string& value) {
            found_users[key] = value;
            return true;
        });
        
        CHECK(found_users.size() == 3);
        CHECK(found_users["user:alice"] == "admin");
        CHECK(found_users["user:bob"] == "member");
        CHECK(found_users["user:charlie"] == "guest");
        
        std::map<std::string, std::string> found_groups;
        ms.iterate("group:", [&found_groups](const std::string& key, const std::string& value) {
            found_groups[key] = value;
            return true;
        });
        
        CHECK(found_groups.size() == 2);
        CHECK(found_groups["group:admins"] == "alice");
        CHECK(found_groups["group:members"] == "bob,charlie");
    }
    
    SECTION("early termination") {
        CHECK(ms.set("test1", "value1"));
        CHECK(ms.set("test2", "value2"));
        CHECK(ms.set("test3", "value3"));
        
        int count = 0;
        ms.iterate("test", [&count](const std::string& key, const std::string& value) {
            count++;
            return count < 2;
        });
        
        CHECK(count == 2);
    }
    
    ms.close();
}

TEST_CASE("memstore thread safety", "[unit][kvds][memstore]") {
    kvds::memstore_c ms;
    CHECK(ms.open(""));
    
    SECTION("concurrent reads and writes") {
        const int num_threads = 4;
        const int operations_per_thread = 100;
        
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&ms, t, operations_per_thread]() {
                for (int i = 0; i < operations_per_thread; ++i) {
                    std::string key = "thread" + std::to_string(t) + "_key" + std::to_string(i);
                    std::string value = "value" + std::to_string(i);
                    
                    ms.set(key, value);
                    
                    std::string retrieved;
                    ms.get(key, retrieved);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        for (int t = 0; t < num_threads; ++t) {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string key = "thread" + std::to_string(t) + "_key" + std::to_string(i);
                CHECK(ms.exists(key));
            }
        }
    }
    
    ms.close();
}

TEST_CASE("memstore operations on closed store", "[unit][kvds][memstore]") {
    kvds::memstore_c ms;
    
    SECTION("all operations fail when closed") {
        CHECK_FALSE(ms.is_open());
        
        std::string value;
        CHECK_FALSE(ms.set("key", "value"));
        CHECK_FALSE(ms.get("key", value));
        CHECK_FALSE(ms.del("key"));
        CHECK_FALSE(ms.exists("key"));
        
        std::map<std::string, std::string> batch = {{"k1", "v1"}};
        CHECK_FALSE(ms.set_batch(batch));
        
        bool callback_called = false;
        ms.iterate("", [&callback_called](const std::string& key, const std::string& value) {
            callback_called = true;
            return true;
        });
        CHECK_FALSE(callback_called);
    }
}
