#include "record/record.hpp"

namespace record {

record_manager_c::record_manager_c(kvds::kv_c& store, std::shared_ptr<spdlog::logger> logger)
    : store_(store), logger_(logger) {}

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

}

