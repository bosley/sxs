#include "record/record.hpp"
#include <chrono>
#include <random>

namespace record {

record_manager_c::record_manager_c(kvds::kv_c& store, std::shared_ptr<spdlog::logger> logger)
    : store_(store), logger_(logger) {
    release_all_locks();
    logger_->info("Record manager initialized, all locks released");
}

std::string record_manager_c::make_meta_key(const std::string& type_id) const {
    return "record:meta:" + type_id;
}

std::string record_manager_c::make_data_key(const std::string& type_id, 
                                            const std::string& instance_id, 
                                            size_t field_index) const {
    return "record:data:" + type_id + ":" + instance_id + ":" + std::to_string(field_index);
}

std::string record_manager_c::make_data_prefix(const std::string& type_id) const {
    return "record:data:" + type_id + ":";
}

std::string record_manager_c::make_instance_prefix(const std::string& type_id, 
                                                   const std::string& instance_id) const {
    return "record:data:" + type_id + ":" + instance_id + ":";
}

std::string record_manager_c::make_lock_key(const std::string& type_id, 
                                            const std::string& instance_id) const {
    return "record:lock:" + type_id + ":" + instance_id;
}

void record_manager_c::release_all_locks() {
    std::vector<std::string> lock_keys;
    
    store_.iterate("record:lock:", [&lock_keys](const std::string& key, const std::string& value) {
        lock_keys.push_back(key);
        return true;
    });
    
    for (const auto& key : lock_keys) {
        store_.del(key);
    }
    
    if (!lock_keys.empty()) {
        logger_->info("Released {} stale locks during initialization", lock_keys.size());
    }
}

bool record_manager_c::ensure_schema_registered(const std::string& type_id, const std::string& schema) {
    std::string meta_key = make_meta_key(type_id);
    
    if (store_.exists(meta_key)) {
        std::string existing_schema;
        if (!store_.get(meta_key, existing_schema)) {
            logger_->error("Failed to read existing schema for type: {}", type_id);
            return false;
        }
        
        if (existing_schema != schema) {
            logger_->error("Schema mismatch for type: {}. Existing and new schemas differ", type_id);
            return false;
        }
        
        return true;
    }
    
    if (!validate_schema(schema)) {
        logger_->error("Invalid schema for type: {}", type_id);
        return false;
    }
    
    if (!store_.set(meta_key, schema)) {
        logger_->error("Failed to store schema for type: {}", type_id);
        return false;
    }
    
    logger_->debug("Registered schema for type: {}", type_id);
    return true;
}

bool record_manager_c::validate_schema(const std::string& schema) {
    auto parse_result = slp::parse(schema);
    if (parse_result.is_error()) {
        logger_->error("Schema parsing failed: {}", parse_result.error().message);
        return false;
    }
    
    auto builder = sconf::sconf_builder_c::from(schema);
    auto result = builder.Parse();
    
    if (result.is_error()) {
        logger_->error("Schema validation failed: {}", result.error().message);
        return false;
    }
    
    return true;
}

bool record_manager_c::exists(const std::string& type_id, const std::string& instance_id) {
    std::string prefix = make_instance_prefix(type_id, instance_id);
    
    bool found = false;
    store_.iterate(prefix, [&found](const std::string& key, const std::string& value) {
        found = true;
        return false;
    });
    
    return found;
}

bool record_manager_c::exists_any(const std::string& instance_id) {
    bool found = false;
    
    store_.iterate("record:meta:", [&](const std::string& meta_key, const std::string& schema) {
        std::string type_id = meta_key.substr(12);
        
        if (exists(type_id, instance_id)) {
            found = true;
            return false;
        }
        
        return true;
    });
    
    return found;
}

void record_manager_c::iterate_type(const std::string& type_id, 
                                    std::function<bool(const std::string& instance_id)> callback) {
    std::string prefix = make_data_prefix(type_id);
    std::string last_instance_id;
    
    store_.iterate(prefix, [&](const std::string& key, const std::string& value) {
        size_t prefix_len = prefix.length();
        std::string remainder = key.substr(prefix_len);
        
        size_t colon_pos = remainder.find(':');
        if (colon_pos == std::string::npos) {
            return true;
        }
        
        std::string instance_id = remainder.substr(0, colon_pos);
        
        if (instance_id != last_instance_id) {
            last_instance_id = instance_id;
            return callback(instance_id);
        }
        
        return true;
    });
}

void record_manager_c::iterate_all(std::function<bool(const std::string& type_id, 
                                                      const std::string& instance_id)> callback) {
    store_.iterate("record:meta:", [&](const std::string& meta_key, const std::string& schema) {
        std::string type_id = meta_key.substr(12);
        
        bool continue_outer = true;
        iterate_type(type_id, [&](const std::string& instance_id) {
            if (!callback(type_id, instance_id)) {
                continue_outer = false;
                return false;
            }
            return true;
        });
        
        return continue_outer;
    });
}

std::string record_base_c::generate_lock_token() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t random_value = dis(gen);
    
    return std::to_string(timestamp) + "_" + std::to_string(random_value);
}

bool record_base_c::acquire_lock() {
    if (!manager_) {
        return false;
    }
    
    std::string lock_key = manager_->make_lock_key(get_type_id(), instance_id_);
    
    if (lock_token_.empty()) {
        lock_token_ = generate_lock_token();
    }
    
    std::string existing_lock;
    if (manager_->get_store().get(lock_key, existing_lock)) {
        if (existing_lock == lock_token_) {
            return true;
        }
        return false;
    }
    
    if (!manager_->get_store().set(lock_key, lock_token_)) {
        return false;
    }
    
    return true;
}

bool record_base_c::verify_lock() {
    if (!manager_) {
        return false;
    }
    
    std::string lock_key = manager_->make_lock_key(get_type_id(), instance_id_);
    std::string current_lock;
    
    if (!manager_->get_store().get(lock_key, current_lock)) {
        return false;
    }
    
    return current_lock == lock_token_;
}

void record_base_c::release_lock() {
    if (!manager_) {
        return;
    }
    
    std::string lock_key = manager_->make_lock_key(get_type_id(), instance_id_);
    manager_->get_store().del(lock_key);
    lock_token_.clear();
}

bool record_base_c::save() {
    if (!manager_) {
        return false;
    }
    
    if (!acquire_lock()) {
        return false;
    }
    
    if (!verify_lock()) {
        release_lock();
        return false;
    }
    
    for (size_t i = 0; i < field_count(); ++i) {
        std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
        if (!manager_->get_store().set(key, field_values_[i])) {
            release_lock();
            return false;
        }
    }
    
    release_lock();
    return true;
}

bool record_base_c::del() {
    if (!manager_) {
        return false;
    }
    
    if (!acquire_lock()) {
        return false;
    }
    
    for (size_t i = 0; i < field_count(); ++i) {
        std::string key = manager_->make_data_key(get_type_id(), instance_id_, i);
        if (!manager_->get_store().del(key)) {
            release_lock();
            return false;
        }
    }
    
    std::string lock_key = manager_->make_lock_key(get_type_id(), instance_id_);
    manager_->get_store().del(lock_key);
    lock_token_.clear();
    
    return true;
}

}

