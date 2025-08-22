#include <quanta/quanta.hpp>
#include <iostream>
#include <filesystem>

using namespace pkg::quanta;

quanta_store_c::quanta_store_c(const quanta_config_c& config) : config_(config), type_(config.type) {}

quanta_store_c::~quanta_store_c() {
    close();
}

bool quanta_store_c::open() {
    if (type_ == quanta_store_type_e::UNSET) {
        return false;
    }

    // For file-based stores, load the manifest
    if (type_ == quanta_store_type_e::FILE && config_.manifest_path.has_value()) {
        if (!load_manifest()) {
            return false;
        }
    }

    // TODO: open the datastores and check the schemas are valid etc

    for (const auto& entry : manifest_.entries) {
        std::cout << "Schema: " << entry.schema.get_name() << " Datastore: " << entry.datastore_path << std::endl;
    }

    return true;
}

bool quanta_store_c::close() {
    // Save manifest if we have changes and it's a file store
    if (type_ == quanta_store_type_e::FILE && config_.manifest_path.has_value()) {
        return save_manifest();
    }

    // TODO: close the datastores appropriately

    return true;
}

bool quanta_store_c::load_manifest() {
    if (!config_.manifest_path.has_value()) {
        return false;
    }

    const std::string& manifest_path = config_.manifest_path.value();

    // If manifest doesn't exist and create_if_missing is true, we'll create it later
    if (!std::filesystem::exists(manifest_path)) {
        if (config_.create_if_missing) {
            return true; // Will create empty manifest
        }
        return false;
    }

    try {
        std::ifstream file(manifest_path);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j;
        file >> j;
        manifest_ = j.get<quanta_manifest_c>();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading manifest: " << e.what() << std::endl;
        return false;
    }
}

bool quanta_store_c::save_manifest() {
    if (!config_.manifest_path.has_value()) {
        return false;
    }

    const std::string& manifest_path = config_.manifest_path.value();

    try {
        // Ensure directory exists
        std::filesystem::path manifest_file_path(manifest_path);
        std::filesystem::create_directories(manifest_file_path.parent_path());

        std::ofstream file(manifest_path);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j = manifest_;
        file << j.dump(4); // Pretty print with 4 space indent
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving manifest: " << e.what() << std::endl;
        return false;
    }
}