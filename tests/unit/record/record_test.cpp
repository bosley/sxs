#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
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
    auto logger = spdlog::get("record_test");
    if (!logger) {
        logger = spdlog::stdout_color_mt("record_test");
    }
    return logger;
}

class user_record_c : public record::record_base_c {
public:
    user_record_c() {
        field_values_.resize(3);
    }
    
    std::string get_type_id() const override {
        return "user";
    }
    
    std::string get_schema() const override {
        return R"([(name "string") (age 42) (email "string")])";
    }
    
    size_t field_count() const override {
        return 3;
    }
    
    bool get_field(size_t index, std::string& value) override {
        if (index >= field_values_.size()) {
            return false;
        }
        value = field_values_[index];
        return true;
    }
    
    bool set_field(size_t index, const std::string& value) override {
        if (index >= field_values_.size()) {
            return false;
        }
        field_values_[index] = value;
        return true;
    }
    
    bool save() override {
        if (!manager_) {
            return false;
        }
        
        for (size_t i = 0; i < field_count(); ++i) {
            std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
            if (!manager_->get_store().set(key, field_values_[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    bool load() override {
        if (!manager_) {
            return false;
        }
        
        for (size_t i = 0; i < field_count(); ++i) {
            std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
            if (!manager_->get_store().get(key, field_values_[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    bool del() override {
        if (!manager_) {
            return false;
        }
        
        for (size_t i = 0; i < field_count(); ++i) {
            std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
            if (!manager_->get_store().del(key)) {
                return false;
            }
        }
        
        return true;
    }
    
    void set_name(const std::string& name) { field_values_[0] = name; }
    void set_age(const std::string& age) { field_values_[1] = age; }
    void set_email(const std::string& email) { field_values_[2] = email; }
    
    std::string get_name() const { return field_values_[0]; }
    std::string get_age() const { return field_values_[1]; }
    std::string get_email() const { return field_values_[2]; }
};

class product_record_c : public record::record_base_c {
public:
    product_record_c() {
        field_values_.resize(2);
    }
    
    std::string get_type_id() const override {
        return "product";
    }
    
    std::string get_schema() const override {
        return R"([(sku "string") (price 99.99)])";
    }
    
    size_t field_count() const override {
        return 2;
    }
    
    bool get_field(size_t index, std::string& value) override {
        if (index >= field_values_.size()) {
            return false;
        }
        value = field_values_[index];
        return true;
    }
    
    bool set_field(size_t index, const std::string& value) override {
        if (index >= field_values_.size()) {
            return false;
        }
        field_values_[index] = value;
        return true;
    }
    
    bool save() override {
        if (!manager_) {
            return false;
        }
        
        for (size_t i = 0; i < field_count(); ++i) {
            std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
            if (!manager_->get_store().set(key, field_values_[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    bool load() override {
        if (!manager_) {
            return false;
        }
        
        for (size_t i = 0; i < field_count(); ++i) {
            std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
            if (!manager_->get_store().get(key, field_values_[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    bool del() override {
        if (!manager_) {
            return false;
        }
        
        for (size_t i = 0; i < field_count(); ++i) {
            std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
            if (!manager_->get_store().del(key)) {
                return false;
            }
        }
        
        return true;
    }
    
    void set_sku(const std::string& sku) { field_values_[0] = sku; }
    void set_price(const std::string& price) { field_values_[1] = price; }
    
    std::string get_sku() const { return field_values_[0]; }
    std::string get_price() const { return field_values_[1]; }
};

}

TEST_CASE("record schema registration", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_schema");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("schema auto-registration on first get_or_create") {
        auto user = manager.get_or_create<user_record_c>("user_001");
        CHECK(user.has_value());
        
        std::string meta_key = manager.make_meta_key("user");
        CHECK(ds.exists(meta_key));
        
        std::string schema;
        CHECK(ds.get(meta_key, schema));
        CHECK(schema.find("name") != std::string::npos);
    }
    
    SECTION("schema persistence across manager instances") {
        {
            record::record_manager_c manager1(ds, logger);
            auto user = manager1.get_or_create<user_record_c>("user_001");
            CHECK(user.has_value());
        }
        
        record::record_manager_c manager2(ds, logger);
        auto user = manager2.get_or_create<user_record_c>("user_002");
        CHECK(user.has_value());
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("record creation and persistence", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_persist");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("create new record and save") {
        auto user = manager.get_or_create<user_record_c>("alice");
        CHECK(user.has_value());
        
        (*user)->set_name("Alice Smith");
        (*user)->set_age("30");
        (*user)->set_email("alice@example.com");
        
        CHECK((*user)->save());
        
        CHECK(ds.exists(manager.make_data_key("user", "alice", 0)));
        CHECK(ds.exists(manager.make_data_key("user", "alice", 1)));
        CHECK(ds.exists(manager.make_data_key("user", "alice", 2)));
    }
    
    SECTION("load existing record") {
        {
            auto user = manager.get_or_create<user_record_c>("bob");
            (*user)->set_name("Bob Jones");
            (*user)->set_age("25");
            (*user)->set_email("bob@example.com");
            (*user)->save();
        }
        
        auto user = manager.get_or_create<user_record_c>("bob");
        CHECK(user.has_value());
        
        CHECK((*user)->get_name() == "Bob Jones");
        CHECK((*user)->get_age() == "25");
        CHECK((*user)->get_email() == "bob@example.com");
    }
    
    SECTION("update existing record") {
        auto user = manager.get_or_create<user_record_c>("charlie");
        (*user)->set_name("Charlie Brown");
        (*user)->set_age("35");
        (*user)->set_email("charlie@example.com");
        (*user)->save();
        
        auto user2 = manager.get_or_create<user_record_c>("charlie");
        (*user2)->set_age("36");
        (*user2)->save();
        
        auto user3 = manager.get_or_create<user_record_c>("charlie");
        CHECK((*user3)->get_age() == "36");
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("record field operations", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_fields");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("get and set fields by index") {
        auto user = manager.get_or_create<user_record_c>("dave");
        
        CHECK((*user)->set_field(0, "Dave Wilson"));
        CHECK((*user)->set_field(1, "40"));
        CHECK((*user)->set_field(2, "dave@example.com"));
        
        std::string value;
        CHECK((*user)->get_field(0, value));
        CHECK(value == "Dave Wilson");
        
        CHECK((*user)->get_field(1, value));
        CHECK(value == "40");
        
        CHECK((*user)->get_field(2, value));
        CHECK(value == "dave@example.com");
    }
    
    SECTION("invalid field index") {
        auto user = manager.get_or_create<user_record_c>("eve");
        
        std::string value;
        CHECK_FALSE((*user)->get_field(999, value));
        CHECK_FALSE((*user)->set_field(999, "invalid"));
    }
    
    SECTION("field count") {
        auto user = manager.get_or_create<user_record_c>("frank");
        CHECK((*user)->field_count() == 3);
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("record existence checks", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_exists");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("check existence by type and instance") {
        CHECK_FALSE(manager.exists("user", "grace"));
        
        auto user = manager.get_or_create<user_record_c>("grace");
        (*user)->set_name("Grace Hopper");
        (*user)->save();
        
        CHECK(manager.exists("user", "grace"));
        CHECK_FALSE(manager.exists("user", "nonexistent"));
    }
    
    SECTION("check existence across types") {
        auto user = manager.get_or_create<user_record_c>("henry");
        (*user)->set_name("Henry");
        (*user)->save();
        
        auto product = manager.get_or_create<product_record_c>("prod_001");
        (*product)->set_sku("SKU001");
        (*product)->save();
        
        CHECK(manager.exists("user", "henry"));
        CHECK(manager.exists("product", "prod_001"));
        CHECK_FALSE(manager.exists("user", "prod_001"));
        CHECK_FALSE(manager.exists("product", "henry"));
    }
    
    SECTION("exists_any checks all types") {
        auto user = manager.get_or_create<user_record_c>("iris");
        (*user)->set_name("Iris");
        (*user)->save();
        
        CHECK(manager.exists_any("iris"));
        CHECK_FALSE(manager.exists_any("nonexistent"));
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("record type iteration", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_iter_type");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("iterate instances of a type") {
        auto user1 = manager.get_or_create<user_record_c>("jack");
        (*user1)->set_name("Jack");
        (*user1)->save();
        
        auto user2 = manager.get_or_create<user_record_c>("jill");
        (*user2)->set_name("Jill");
        (*user2)->save();
        
        auto user3 = manager.get_or_create<user_record_c>("john");
        (*user3)->set_name("John");
        (*user3)->save();
        
        std::vector<std::string> instance_ids;
        manager.iterate_type("user", [&instance_ids](const std::string& id) {
            instance_ids.push_back(id);
            return true;
        });
        
        CHECK(instance_ids.size() == 3);
        CHECK(std::find(instance_ids.begin(), instance_ids.end(), "jack") != instance_ids.end());
        CHECK(std::find(instance_ids.begin(), instance_ids.end(), "jill") != instance_ids.end());
        CHECK(std::find(instance_ids.begin(), instance_ids.end(), "john") != instance_ids.end());
    }
    
    SECTION("early termination in iteration") {
        for (int i = 0; i < 5; ++i) {
            auto user = manager.get_or_create<user_record_c>("user_" + std::to_string(i));
            (*user)->set_name("User " + std::to_string(i));
            (*user)->save();
        }
        
        int count = 0;
        manager.iterate_type("user", [&count](const std::string& id) {
            count++;
            return count < 3;
        });
        
        CHECK(count == 3);
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("record iteration across all types", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_iter_all");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("iterate all records") {
        auto user1 = manager.get_or_create<user_record_c>("kate");
        (*user1)->set_name("Kate");
        (*user1)->save();
        
        auto user2 = manager.get_or_create<user_record_c>("kevin");
        (*user2)->set_name("Kevin");
        (*user2)->save();
        
        auto prod1 = manager.get_or_create<product_record_c>("prod_100");
        (*prod1)->set_sku("SKU100");
        (*prod1)->save();
        
        auto prod2 = manager.get_or_create<product_record_c>("prod_200");
        (*prod2)->set_sku("SKU200");
        (*prod2)->save();
        
        std::map<std::string, std::vector<std::string>> records;
        manager.iterate_all([&records](const std::string& type_id, const std::string& instance_id) {
            records[type_id].push_back(instance_id);
            return true;
        });
        
        CHECK(records.size() == 2);
        CHECK(records["user"].size() == 2);
        CHECK(records["product"].size() == 2);
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("record deletion", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_delete");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("delete record") {
        auto user = manager.get_or_create<user_record_c>("larry");
        (*user)->set_name("Larry");
        (*user)->set_age("45");
        (*user)->set_email("larry@example.com");
        (*user)->save();
        
        CHECK(manager.exists("user", "larry"));
        
        CHECK((*user)->del());
        
        CHECK_FALSE(manager.exists("user", "larry"));
        CHECK_FALSE(ds.exists(manager.make_data_key("user", "larry", 0)));
        CHECK_FALSE(ds.exists(manager.make_data_key("user", "larry", 1)));
        CHECK_FALSE(ds.exists(manager.make_data_key("user", "larry", 2)));
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

TEST_CASE("multiple record types", "[unit][record]") {
    kvds::datastore_c ds;
    std::string test_db_path = get_unique_test_path("/tmp/record_test_multi");
    auto logger = create_test_logger();
    
    ensure_db_cleanup(test_db_path);
    CHECK(ds.open(test_db_path));
    
    record::record_manager_c manager(ds, logger);
    
    SECTION("different record types coexist") {
        auto user = manager.get_or_create<user_record_c>("mary");
        (*user)->set_name("Mary");
        (*user)->save();
        
        auto product = manager.get_or_create<product_record_c>("prod_300");
        (*product)->set_sku("SKU300");
        (*product)->save();
        
        CHECK(manager.exists("user", "mary"));
        CHECK(manager.exists("product", "prod_300"));
        
        auto user_loaded = manager.get_or_create<user_record_c>("mary");
        CHECK((*user_loaded)->get_name() == "Mary");
        
        auto product_loaded = manager.get_or_create<product_record_c>("prod_300");
        CHECK((*product_loaded)->get_sku() == "SKU300");
    }
    
    ds.close();
    ensure_db_cleanup(test_db_path);
}

