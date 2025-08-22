#pragma once

#include <string>
#include <optional>
#include <fstream>
#include <nlohmann/json.hpp>
#include <quanta/schema.hpp>

namespace pkg::quanta {

constexpr size_t ONE_MB = 1024 * 1024;
constexpr size_t ONE_GB = 1024 * 1024 * 1024;

constexpr size_t DEFAULT_WRITE_BUFFER_SIZE = 4 * ONE_MB;
constexpr size_t DEFAULT_MAX_FILE_SIZE = 10 * ONE_GB;
constexpr size_t DEFAULT_MAX_OPEN_FILES = 1000;

enum class quanta_store_type_e {
    UNSET = 0,
    MEMORY,
    FILE
};

// Handed to quanta_store to configre/instantiate the store
struct quanta_config_c {
    quanta_store_type_e type{quanta_store_type_e::UNSET};
    std::optional<std::string> manifest_path;

    bool read_only{false};
    bool create_if_missing{true};
    bool error_if_exists{false};
    bool paranoid_checks{false};
    size_t write_buffer_size{DEFAULT_WRITE_BUFFER_SIZE};
    size_t max_open_files{DEFAULT_MAX_OPEN_FILES};
    size_t max_file_size{DEFAULT_MAX_FILE_SIZE};
};


// these two are used to store in the file that the path points to in a quanta_config

struct quanta_manifest_entry_c {
    quanta_manifest_entry_c() = default;
    quanta_manifest_entry_c(const schema_c& schema, const std::string& datastore_path) : schema(schema), datastore_path(datastore_path) {}

    schema_c schema;
    std::string datastore_path;
};

struct quanta_manifest_c {
    quanta_manifest_c() = default;
    quanta_manifest_c(const std::vector<quanta_manifest_entry_c>& entries) : entries(entries) {}

    std::vector<quanta_manifest_entry_c> entries;
};

inline void to_json(nlohmann::json& j, const quanta_manifest_entry_c& entry) {
    j = nlohmann::json{
        {"schema", entry.schema.to_json()},
        {"datastore_path", entry.datastore_path}
    };
}

inline void from_json(const nlohmann::json& j, quanta_manifest_entry_c& entry) {
    entry.schema = schema_c::from_json(j.at("schema"));
    j.at("datastore_path").get_to(entry.datastore_path);
}

inline void to_json(nlohmann::json& j, const quanta_manifest_c& manifest) {
    j = nlohmann::json{{"entries", manifest.entries}};
}

inline void from_json(const nlohmann::json& j, quanta_manifest_c& manifest) {
    auto entries_json = j.at("entries");
    manifest.entries.clear();
    for (const auto& entry_json : entries_json) {
        quanta_manifest_entry_c entry;
        from_json(entry_json, entry);
        manifest.entries.push_back(entry);
    }
}

/*
    Quanta stores may (under the hood) use more than one database/ memory data store
    A "quanta store" is just a collection of schemas and their data. Inside a quanta store file
    lists the schemas and the datastore they can be found in
*/
class quanta_store_c {
public:
    quanta_store_c(const quanta_config_c& config);
    ~quanta_store_c();

    bool open();
    bool close();

private:
    bool load_manifest();
    bool save_manifest();

    quanta_config_c config_;
    quanta_store_type_e type_;
    quanta_manifest_c manifest_;
};

} // namespace pkg::quanta