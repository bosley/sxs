#pragma once

#include <string>
#include <functional>
#include <map>

namespace kvds {

class kv_stat {
public:
    virtual ~kv_stat() = default;
    virtual bool is_open() const = 0;
};

class kv_reader_c {
public:
    virtual ~kv_reader_c() = default;
    virtual bool get(const std::string& key, std::string& value) = 0;
    virtual bool exists(const std::string& key) const = 0;
    virtual void iterate(
        const std::string& prefix,
        std::function<bool(
            const std::string& key, const std::string& value)> callback) const = 0;
};

class kv_writer_c {
public:
    virtual ~kv_writer_c() = default;
    virtual bool set(const std::string& key, const std::string& value) = 0;
    virtual bool del(const std::string& key) = 0;
    virtual bool set_batch(const std::map<std::string, std::string>& kv_pairs) = 0;
};

class kv_c : public kv_reader_c, public kv_writer_c, public kv_stat {
public:
    virtual ~kv_c() = default;
};

}