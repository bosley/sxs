#pragma once

#include <fstream>
#include <cstdint>
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

namespace app {
using nlohmann::json;

constexpr uint16_t DEFAULT_HTTP_PORT = 8080;
constexpr uint32_t DEFAULT_HTTP_THREADS = 10;
constexpr uint32_t DEFAULT_HTTP_MAX_CONNECTIONS = 1000;
constexpr uint64_t DEFAULT_HTTP_MAX_REQUEST_SIZE = 1024ULL * 1024ULL * 10ULL;
constexpr uint64_t DEFAULT_HTTP_MAX_RESPONSE_SIZE = 1024ULL * 1024ULL * 10ULL;
constexpr uint32_t DEFAULT_HTTP_TIMEOUT = 10;

class config_c {

public:
    std::optional<std::string> get_cert_path() const;
    std::optional<std::string> get_key_path() const;
     std::string get_http_address() const;
     uint16_t get_http_port() const;
     uint32_t get_http_threads() const;
     uint32_t get_http_max_connections() const;
     uint64_t get_http_max_request_size() const;
     uint64_t get_http_max_response_size() const;
     uint32_t get_http_timeout() const;
     bool get_http_keep_alive() const;
     uint32_t get_http_keep_alive_timeout() const;
     uint32_t get_http_keep_alive_max_connections() const;
    friend bool load_config(const std::string& path, config_c& config);

private:
    json config_;
};

inline bool load_config(const std::string& path, config_c& config) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> config.config_;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

inline std::optional<std::string> config_c::get_cert_path() const {
    auto it_tls = config_.find("tls");
    if (it_tls == config_.end() || !it_tls->is_object()) {
        return std::nullopt;
    }
    auto it_cert = it_tls->find("cert_path");
    if (it_cert == it_tls->end() || !it_cert->is_string()) {
        return std::nullopt;
    }
    return it_cert->get<std::string>();
}

inline std::optional<std::string> config_c::get_key_path() const {
    auto it_tls = config_.find("tls");
    if (it_tls == config_.end() || !it_tls->is_object()) {
        return std::nullopt;
    }
    auto it_key = it_tls->find("key_path");
    if (it_key == it_tls->end() || !it_key->is_string()) {
        return std::nullopt;
    }
    return it_key->get<std::string>();
}

inline std::string config_c::get_http_address() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return std::string("0.0.0.0");
    }
    auto it = it_http->find("address");
    if (it == it_http->end() || !it->is_string()) {
        return std::string("0.0.0.0");
    }
    return it->get<std::string>();
}

inline uint16_t config_c::get_http_port() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint16_t>(DEFAULT_HTTP_PORT);
    }
    auto it = it_http->find("port");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint16_t>(DEFAULT_HTTP_PORT);
    }
    uint64_t value = 0;
    if (it->is_number_unsigned()) value = it->get<uint64_t>();
    else {
        auto v = it->get<int64_t>();
        if (v < 0) return static_cast<uint16_t>(DEFAULT_HTTP_PORT);
        value = static_cast<uint64_t>(v);
    }
    if (value > 65535) return static_cast<uint16_t>(DEFAULT_HTTP_PORT);
    return static_cast<uint16_t>(value);
}

inline uint32_t config_c::get_http_threads() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint32_t>(DEFAULT_HTTP_THREADS);
    }
    auto it = it_http->find("threads");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint32_t>(DEFAULT_HTTP_THREADS);
    }
    uint64_t value = 0;
    if (it->is_number_unsigned()) value = it->get<uint64_t>();
    else {
        auto v = it->get<int64_t>();
        if (v < 0) return static_cast<uint32_t>(DEFAULT_HTTP_THREADS);
        value = static_cast<uint64_t>(v);
    }
    if (value > UINT32_MAX) return static_cast<uint32_t>(DEFAULT_HTTP_THREADS);
    return static_cast<uint32_t>(value);
}

inline uint32_t config_c::get_http_max_connections() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
    }
    auto it = it_http->find("max_connections");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
    }
    uint64_t value = 0;
    if (it->is_number_unsigned()) value = it->get<uint64_t>();
    else {
        auto v = it->get<int64_t>();
        if (v < 0) return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
        value = static_cast<uint64_t>(v);
    }
    if (value > UINT32_MAX) return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
    return static_cast<uint32_t>(value);
}

inline uint64_t config_c::get_http_max_request_size() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint64_t>(DEFAULT_HTTP_MAX_REQUEST_SIZE);
    }
    auto it = it_http->find("max_request_size");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint64_t>(DEFAULT_HTTP_MAX_REQUEST_SIZE);
    }
    if (it->is_number_unsigned()) return it->get<uint64_t>();
    auto v = it->get<int64_t>();
    if (v < 0) return static_cast<uint64_t>(DEFAULT_HTTP_MAX_REQUEST_SIZE);
    return static_cast<uint64_t>(v);
}

inline uint64_t config_c::get_http_max_response_size() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint64_t>(DEFAULT_HTTP_MAX_RESPONSE_SIZE);
    }
    auto it = it_http->find("max_response_size");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint64_t>(DEFAULT_HTTP_MAX_RESPONSE_SIZE);
    }
    if (it->is_number_unsigned()) return it->get<uint64_t>();
    auto v = it->get<int64_t>();
    if (v < 0) return static_cast<uint64_t>(DEFAULT_HTTP_MAX_RESPONSE_SIZE);
    return static_cast<uint64_t>(v);
}

inline uint32_t config_c::get_http_timeout() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
    }
    auto it = it_http->find("timeout");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
    }
    uint64_t value = 0;
    if (it->is_number_unsigned()) value = it->get<uint64_t>();
    else {
        auto v = it->get<int64_t>();
        if (v < 0) return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
        value = static_cast<uint64_t>(v);
    }
    if (value > UINT32_MAX) return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
    return static_cast<uint32_t>(value);
}

inline bool config_c::get_http_keep_alive() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return true;
    }
    auto it = it_http->find("keep_alive");
    if (it == it_http->end() || !it->is_boolean()) {
        return true;
    }
    return it->get<bool>();
}

inline uint32_t config_c::get_http_keep_alive_timeout() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
    }
    auto it = it_http->find("keep_alive_timeout");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
    }
    uint64_t value = 0;
    if (it->is_number_unsigned()) value = it->get<uint64_t>();
    else {
        auto v = it->get<int64_t>();
        if (v < 0) return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
        value = static_cast<uint64_t>(v);
    }
    if (value > UINT32_MAX) return static_cast<uint32_t>(DEFAULT_HTTP_TIMEOUT);
    return static_cast<uint32_t>(value);
}

inline uint32_t config_c::get_http_keep_alive_max_connections() const {
    auto it_http = config_.find("http");
    if (it_http == config_.end() || !it_http->is_object()) {
        return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
    }
    auto it = it_http->find("keep_alive_max_connections");
    if (it == it_http->end() || !(it->is_number_unsigned() || it->is_number_integer())) {
        return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
    }
    uint64_t value = 0;
    if (it->is_number_unsigned()) value = it->get<uint64_t>();
    else {
        auto v = it->get<int64_t>();
        if (v < 0) return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
        value = static_cast<uint64_t>(v);
    }
    if (value > UINT32_MAX) return static_cast<uint32_t>(DEFAULT_HTTP_MAX_CONNECTIONS);
    return static_cast<uint32_t>(value);
}

inline bool new_config(const std::string& path)  {
    // Create a new config with default values and dump it to the file
    json config;
    config["http"]["address"] = "0.0.0.0";
    config["http"]["port"] = DEFAULT_HTTP_PORT;
    config["http"]["threads"] = DEFAULT_HTTP_THREADS;
    config["http"]["max_connections"] = DEFAULT_HTTP_MAX_CONNECTIONS;
    config["http"]["max_request_size"] = DEFAULT_HTTP_MAX_REQUEST_SIZE;
    config["http"]["max_response_size"] = DEFAULT_HTTP_MAX_RESPONSE_SIZE;
    config["http"]["timeout"] = DEFAULT_HTTP_TIMEOUT;
    config["http"]["keep_alive"] = true;
    config["http"]["keep_alive_timeout"] = DEFAULT_HTTP_TIMEOUT;
    config["http"]["keep_alive_max_connections"] = DEFAULT_HTTP_MAX_CONNECTIONS;
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << config.dump(4);
    return true;
}

} // namespace app