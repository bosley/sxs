#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/kvds.hpp>
#include <snitch/snitch.hpp>
#include <thread>
#include <vector>

namespace {
void ensure_cleanup(const std::string &path) {
  std::filesystem::remove_all(path);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
} // namespace

TEST_CASE("distributor memory-backed store", "[unit][kvds][distributor]") {
  kvds::kv_c_distributor_c distributor("/tmp/test_distributor");

  SECTION("create and use memory store") {
    auto store_opt = distributor.get_or_create_kv_c(
        "mem_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);

    REQUIRE(store_opt.has_value());
    auto store = *store_opt;
    CHECK(store->is_open());
    CHECK(store->set("key1", "value1"));

    std::string value;
    CHECK(store->get("key1", value));
    CHECK(value == "value1");

    CHECK(store->exists("key1"));
    CHECK_FALSE(store->exists("nonexistent"));
  }

  SECTION("get same identifier returns same instance") {
    auto store1_opt = distributor.get_or_create_kv_c(
        "shared_mem", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store1_opt.has_value());
    auto store1 = *store1_opt;
    CHECK(store1->set("shared_key", "shared_value"));

    auto store2_opt = distributor.get_or_create_kv_c(
        "shared_mem", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store2_opt.has_value());
    auto store2 = *store2_opt;

    std::string value;
    CHECK(store2->get("shared_key", value));
    CHECK(value == "shared_value");

    CHECK(store1.get() == store2.get());
  }
}

TEST_CASE("distributor disk-backed store", "[unit][kvds][distributor]") {
  std::string test_path = "/tmp/test_distributor_disk";
  ensure_cleanup(test_path);

  kvds::kv_c_distributor_c distributor(test_path);

  SECTION("create and use disk store") {
    auto store_opt = distributor.get_or_create_kv_c(
        "disk_test", kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

    REQUIRE(store_opt.has_value());
    auto store = *store_opt;
    CHECK(store->is_open());
    CHECK(store->set("persistent_key", "persistent_value"));

    std::string value;
    CHECK(store->get("persistent_key", value));
    CHECK(value == "persistent_value");
  }

  SECTION("disk store persists data") {
    {
      auto store_opt = distributor.get_or_create_kv_c(
          "persist_test", kvds::kv_c_distributor_c::kv_c_backend_e::DISK);
      REQUIRE(store_opt.has_value());
      auto store = *store_opt;
      CHECK(store->set("key1", "value1"));
      CHECK(store->set("key2", "value2"));
    }

    auto store_opt = distributor.get_or_create_kv_c(
        "persist_test", kvds::kv_c_distributor_c::kv_c_backend_e::DISK);
    REQUIRE(store_opt.has_value());
    auto store = *store_opt;
    std::string value;
    CHECK(store->get("key1", value));
    CHECK(value == "value1");
    CHECK(store->get("key2", value));
    CHECK(value == "value2");
  }

  ensure_cleanup(test_path);
}

TEST_CASE("distributor multiple stores", "[unit][kvds][distributor]") {
  std::string test_path = "/tmp/test_distributor_multi";
  ensure_cleanup(test_path);

  kvds::kv_c_distributor_c distributor(test_path);

  SECTION("different identifiers create separate stores") {
    auto mem_store1_opt = distributor.get_or_create_kv_c(
        "mem1", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    auto mem_store2_opt = distributor.get_or_create_kv_c(
        "mem2", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    auto disk_store1_opt = distributor.get_or_create_kv_c(
        "disk1", kvds::kv_c_distributor_c::kv_c_backend_e::DISK);
    auto disk_store2_opt = distributor.get_or_create_kv_c(
        "disk2", kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

    REQUIRE(mem_store1_opt.has_value());
    REQUIRE(mem_store2_opt.has_value());
    REQUIRE(disk_store1_opt.has_value());
    REQUIRE(disk_store2_opt.has_value());

    auto mem_store1 = *mem_store1_opt;
    auto mem_store2 = *mem_store2_opt;
    auto disk_store1 = *disk_store1_opt;
    auto disk_store2 = *disk_store2_opt;

    CHECK(mem_store1.get() != mem_store2.get());
    CHECK(disk_store1.get() != disk_store2.get());
    CHECK(mem_store1.get() != disk_store1.get());

    CHECK(mem_store1->set("key", "mem1_value"));
    CHECK(mem_store2->set("key", "mem2_value"));
    CHECK(disk_store1->set("key", "disk1_value"));
    CHECK(disk_store2->set("key", "disk2_value"));

    std::string value;
    CHECK(mem_store1->get("key", value));
    CHECK(value == "mem1_value");
    CHECK(mem_store2->get("key", value));
    CHECK(value == "mem2_value");
    CHECK(disk_store1->get("key", value));
    CHECK(value == "disk1_value");
    CHECK(disk_store2->get("key", value));
    CHECK(value == "disk2_value");
  }

  ensure_cleanup(test_path);
}

TEST_CASE("distributor shared_obj_c references", "[unit][kvds][distributor]") {
  kvds::kv_c_distributor_c distributor("/tmp/test_distributor_refs");

  SECTION("multiple shared_obj_c references work correctly") {
    auto store1_opt = distributor.get_or_create_kv_c(
        "ref_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store1_opt.has_value());
    auto store1 = *store1_opt;
    CHECK(store1->set("key1", "value1"));

    auto store2 = store1;
    auto store3_opt = distributor.get_or_create_kv_c(
        "ref_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store3_opt.has_value());
    auto store3 = *store3_opt;

    std::string value;
    CHECK(store2->get("key1", value));
    CHECK(value == "value1");
    CHECK(store3->get("key1", value));
    CHECK(value == "value1");

    CHECK(store2->set("key2", "value2"));
    CHECK(store1->get("key2", value));
    CHECK(value == "value2");
  }
}

TEST_CASE("distributor lazy cleanup", "[unit][kvds][distributor]") {
  std::string test_path = "/tmp/test_distributor_lazy";
  ensure_cleanup(test_path);

  kvds::kv_c_distributor_c distributor(test_path);

  SECTION("store remains accessible after references released") {
    {
      auto store_opt = distributor.get_or_create_kv_c(
          "lazy_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
      REQUIRE(store_opt.has_value());
      auto store = *store_opt;
      CHECK(store->set("key", "value"));
    }

    auto store_opt = distributor.get_or_create_kv_c(
        "lazy_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store_opt.has_value());
    auto store = *store_opt;
    std::string value;
    CHECK(store->get("key", value));
    CHECK(value == "value");
  }

  ensure_cleanup(test_path);
}

TEST_CASE("distributor thread safety", "[unit][kvds][distributor]") {
  std::string test_path = "/tmp/test_distributor_threads";
  ensure_cleanup(test_path);

  kvds::kv_c_distributor_c distributor(test_path);

  SECTION("concurrent get_or_create calls") {
    const int num_threads = 8;
    const int operations_per_thread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> error_count{0};

    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back(
          [&distributor, &error_count, t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
              int store_id = t % 4;
              std::string id = "thread_" + std::to_string(store_id);
              kvds::kv_c_distributor_c::kv_c_backend_e backend =
                  (store_id % 2 == 0)
                      ? kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY
                      : kvds::kv_c_distributor_c::kv_c_backend_e::DISK;

              auto store_opt = distributor.get_or_create_kv_c(id, backend);

              if (!store_opt.has_value()) {
                error_count++;
                continue;
              }

              auto store = *store_opt;
              if (!store->is_open()) {
                error_count++;
                continue;
              }

              std::string key = "key_" + std::to_string(i);
              std::string value =
                  "value_" + std::to_string(t) + "_" + std::to_string(i);
              if (!store->set(key, value)) {
                error_count++;
              }

              std::string retrieved;
              if (!store->get(key, retrieved)) {
                error_count++;
              }
            }
          });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(error_count == 0);
  }

  ensure_cleanup(test_path);
}

TEST_CASE("distributor batch operations", "[unit][kvds][distributor]") {
  kvds::kv_c_distributor_c distributor("/tmp/test_distributor_batch");

  SECTION("batch set through distributor") {
    auto store_opt = distributor.get_or_create_kv_c(
        "batch_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store_opt.has_value());
    auto store = *store_opt;

    std::map<std::string, std::string> batch = {
        {"batch1", "value1"}, {"batch2", "value2"}, {"batch3", "value3"}};

    CHECK(store->set_batch(batch));

    std::string value;
    CHECK(store->get("batch1", value));
    CHECK(value == "value1");
    CHECK(store->get("batch2", value));
    CHECK(value == "value2");
    CHECK(store->get("batch3", value));
    CHECK(value == "value3");
  }
}

TEST_CASE("distributor iteration", "[unit][kvds][distributor]") {
  kvds::kv_c_distributor_c distributor("/tmp/test_distributor_iter");

  SECTION("iterate through distributor store") {
    auto store_opt = distributor.get_or_create_kv_c(
        "iter_test", kvds::kv_c_distributor_c::kv_c_backend_e::MEMORY);
    REQUIRE(store_opt.has_value());
    auto store = *store_opt;

    CHECK(store->set("user:alice", "admin"));
    CHECK(store->set("user:bob", "member"));
    CHECK(store->set("user:charlie", "guest"));
    CHECK(store->set("group:admins", "alice"));

    std::map<std::string, std::string> found_users;
    store->iterate("user:", [&found_users](const std::string &key,
                                           const std::string &value) {
      found_users[key] = value;
      return true;
    });

    CHECK(found_users.size() == 3);
    CHECK(found_users["user:alice"] == "admin");
    CHECK(found_users["user:bob"] == "member");
    CHECK(found_users["user:charlie"] == "guest");
  }
}
