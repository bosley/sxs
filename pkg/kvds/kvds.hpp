#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <rocksdb/db.h>

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

class datastore_c : public kv_c {
public:
    datastore_c(const datastore_c&) = delete;
    datastore_c(datastore_c&&) = delete;
    datastore_c& operator=(const datastore_c&) = delete;
    datastore_c& operator=(datastore_c&&) = delete;

    datastore_c();
    ~datastore_c() = default;

    bool open(const std::string& path);
    bool close();

    bool is_open() const override;
    virtual bool set(const std::string& key, const std::string& value) override;
    virtual bool get(const std::string& key, std::string& value) override;
    virtual bool del(const std::string& key) override;
    virtual bool exists(const std::string& key) const override;
    virtual bool set_batch(const std::map<std::string, std::string>& kv_pairs) override;
    virtual void iterate(
        const std::string& prefix,
        std::function<bool(
            const std::string& key, const std::string& value)> callback) const override;
private:
    std::unique_ptr<rocksdb::DB> db_ {nullptr};
    bool is_open_ {false};
};

}