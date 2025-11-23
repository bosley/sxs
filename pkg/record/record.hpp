#pragma once

#include <spdlog/spdlog.h>
#include <kvds/kvds.hpp>
#include <sconf/sconf.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace record {

class record_manager_c;

class record_if {
public:
    virtual ~record_if() = default;
    
    virtual std::string get_type_id() const = 0;
    virtual std::string get_schema() const = 0;
    virtual size_t field_count() const = 0;
    virtual bool get_field(size_t index, std::string& value) = 0;
    virtual bool set_field(size_t index, const std::string& value) = 0;
    virtual bool save() = 0;
    virtual bool load() = 0;
    virtual bool del() = 0;
    
protected:
    friend class record_manager_c;
    virtual void set_manager(record_manager_c* manager) = 0;
    virtual void set_instance_id(const std::string& instance_id) = 0;
};

struct field_descriptor_s {
    std::string name;
    sconf::sconf_type_e type;
    size_t index;
};

class record_manager_c {
public:
    explicit record_manager_c(kvds::kv_c& store, std::shared_ptr<spdlog::logger> logger);
    ~record_manager_c() = default;
    
    record_manager_c(const record_manager_c&) = delete;
    record_manager_c& operator=(const record_manager_c&) = delete;
    
    void release_all_locks();
    
    template<typename T>
    std::optional<std::unique_ptr<T>> get_or_create(const std::string& instance_id) {
        static_assert(std::is_base_of<record_if, T>::value, "T must inherit from record_if");
        
        auto record = std::make_unique<T>();
        std::string type_id = record->get_type_id();
        std::string schema = record->get_schema();
        
        if (!ensure_schema_registered(type_id, schema)) {
            logger_->error("Failed to register schema for type: {}", type_id);
            return std::nullopt;
        }
        
        record->set_manager(this);
        record->set_instance_id(instance_id);
        
        if (exists(type_id, instance_id)) {
            if (!record->load()) {
                logger_->error("Failed to load existing record: {}:{}", type_id, instance_id);
                return std::nullopt;
            }
        }
        
        return record;
    }
    
    bool exists(const std::string& type_id, const std::string& instance_id);
    bool exists_any(const std::string& instance_id);
    
    void iterate_type(const std::string& type_id, 
                     std::function<bool(const std::string& instance_id)> callback);
    void iterate_all(std::function<bool(const std::string& type_id, const std::string& instance_id)> callback);
    
    kvds::kv_c& get_store() { return store_; }
    
    std::string make_meta_key(const std::string& type_id) const;
    std::string make_data_key(const std::string& type_id, const std::string& instance_id, size_t field_index) const;
    std::string make_data_prefix(const std::string& type_id) const;
    std::string make_instance_prefix(const std::string& type_id, const std::string& instance_id) const;
    std::string make_lock_key(const std::string& type_id, const std::string& instance_id) const;
    
private:
    bool ensure_schema_registered(const std::string& type_id, const std::string& schema);
    bool validate_schema(const std::string& schema);
    
    kvds::kv_c& store_;
    std::shared_ptr<spdlog::logger> logger_;
};

class record_base_c : public record_if {
public:
    record_base_c() : manager_(nullptr) {}
    virtual ~record_base_c() = default;
    
    bool save() override;
    bool del() override;
    
protected:
    friend class record_manager_c;
    
    void set_manager(record_manager_c* manager) override {
        manager_ = manager;
    }
    
    void set_instance_id(const std::string& instance_id) override {
        instance_id_ = instance_id;
    }
    
    record_manager_c* manager_;
    std::string instance_id_;
    std::vector<std::string> field_values_;
    
private:
    std::string lock_token_;
    
    std::string generate_lock_token();
    bool acquire_lock();
    bool verify_lock();
    void release_lock();
};

}


