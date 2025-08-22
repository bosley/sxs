#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace pkg::quanta {

enum class schema_field_type_e {
    UNSET = 0,
    STRING,
    INT,
    FLOAT,
    BOOL,
    TIMEPOINT,
    DURATION,
    BINARY,
    SENTINEL
};

static const std::map<schema_field_type_e, std::string> schema_field_type_to_string = {
    {schema_field_type_e::SENTINEL, "SENTINEL"},
    {schema_field_type_e::UNSET, "UNSET"},
    {schema_field_type_e::STRING, "STRING"},
    {schema_field_type_e::INT, "INT"},
    {schema_field_type_e::FLOAT, "FLOAT"},
    {schema_field_type_e::BOOL, "BOOL"},
    {schema_field_type_e::TIMEPOINT, "TIMEPOINT"},
    {schema_field_type_e::DURATION, "DURATION"},
    {schema_field_type_e::BINARY, "BINARY"}
};

// JSON serialization for schema_field_type_e
inline void to_json(nlohmann::json& j, const schema_field_type_e& type) {
    j = schema_field_type_to_string.at(type);
}

inline void from_json(const nlohmann::json& j, schema_field_type_e& type) {
    std::string type_str = j.get<std::string>();
    for (const auto& [enum_val, str_val] : schema_field_type_to_string) {
        if (str_val == type_str) {
            type = enum_val;
            return;
        }
    }
    throw std::runtime_error("Unknown schema field type: " + type_str);
}

struct schema_field_meta_c {
    schema_field_type_e type{schema_field_type_e::UNSET}; //! The type of the field
    uint16_t length{0};     //! Minimum length to support the field type (if applicable > 1)
    uint16_t max_length{0}; //! The maximum length of the field at any given time (always applicable)
    bool is_unique{false};  //! Whether the field is unique within any set of records holding the schema
    bool is_required{false}; //! Whether the field is required for any record holding the schema
};

// JSON serialization for schema_field_meta_c
inline void to_json(nlohmann::json& j, const schema_field_meta_c& meta) {
    j = nlohmann::json{
        {"type", meta.type},
        {"length", meta.length},
        {"max_length", meta.max_length},
        {"is_unique", meta.is_unique},
        {"is_required", meta.is_required}
    };
}

inline void from_json(const nlohmann::json& j, schema_field_meta_c& meta) {
    j.at("type").get_to(meta.type);
    j.at("length").get_to(meta.length);
    j.at("max_length").get_to(meta.max_length);
    j.at("is_unique").get_to(meta.is_unique);
    j.at("is_required").get_to(meta.is_required);
}

//! A schema is a grouping of records that constitute a logical unit of data.
//! Think of this similar to standard database schemas
class schema_c {
public:
    schema_c() = default;  // Default constructor for JSON deserialization
    schema_c(const std::string& name, const std::map<std::string, schema_field_meta_c>& fields) : name_(name), fields_(fields) {}
    ~schema_c() = default;

    bool has_field(const std::string& name) const {
        return fields_.find(name) != fields_.end();
    }

    std::optional<schema_field_meta_c> get_field_meta(const std::string& name) const {
        auto it = fields_.find(name);
        if (it == fields_.end()) {
            return std::nullopt;
        }
        return std::make_optional(it->second);
    }

    const std::map<std::string, schema_field_meta_c>& get_fields_meta() const {
        return fields_;
    }

    const std::string& get_name() const {
        return name_;
    }

    // JSON serialization methods
    nlohmann::json to_json() const {
        return nlohmann::json{
            {"name", name_},
            {"fields", fields_}
        };
    }

    static schema_c from_json(const nlohmann::json& j) {
        std::string name = j.at("name").get<std::string>();
        std::map<std::string, schema_field_meta_c> fields = j.at("fields").get<std::map<std::string, schema_field_meta_c>>();
        return schema_c(name, fields);
    }

private:
    std::string name_;
    std::map<std::string, schema_field_meta_c> fields_;
};

class field_builder_c {
public:
    field_builder_c() = default;
    ~field_builder_c() = default;

    field_builder_c& set_type(schema_field_type_e type) {
        field_.type = type;
        return *this;
    }
    field_builder_c& set_length(uint16_t length) {
        field_.length = length;
        return *this;
    }
    field_builder_c& set_max_length(uint16_t max_length) {
        field_.max_length = max_length;
        return *this;
    }
    field_builder_c& set_is_unique(bool is_unique) {
        field_.is_unique = is_unique;
        return *this;
    }
    field_builder_c& set_is_required(bool is_required) {
        field_.is_required = is_required;
        return *this;
    }
    schema_field_meta_c build() const {
        return field_;
    }
private:
    schema_field_meta_c field_{
        .type = schema_field_type_e::UNSET,
        .length = 0,
        .max_length = 0,
        .is_unique = false,
        .is_required = false
    };
};

class schema_builder_c {
public:
    schema_builder_c(const std::string& name) : name_(name) {}
    ~schema_builder_c() = default;

    schema_builder_c& with_field(const std::string& name, const field_builder_c& field) {
        fields_[name] = field.build();
        return *this;
    }   
    schema_c build() const {
        return schema_c(name_, fields_);
    }
private:
    std::string name_;
    std::map<std::string, schema_field_meta_c> fields_;
};

}