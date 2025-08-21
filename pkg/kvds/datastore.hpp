#pragma once

#include "kvds/kvds.hpp"
#include <memory>

namespace rocksdb {
    class DB;
}

namespace kvds {

class datastore_c : public kv_c {
public:
    datastore_c(const datastore_c&) = delete;
    datastore_c(datastore_c&&) = delete;
    datastore_c& operator=(const datastore_c&) = delete;
    datastore_c& operator=(datastore_c&&) = delete;

    datastore_c();
    ~datastore_c();

    bool open(const std::string& path);
    bool close();

    bool is_open() const override;
    bool set(const std::string& key, const std::string& value) override;
    bool get(const std::string& key, std::string& value) override;
    bool del(const std::string& key) override;
    bool exists(const std::string& key) const override;
    bool set_batch(const std::map<std::string, std::string>& kv_pairs) override;
    void iterate(
        const std::string& prefix,
        std::function<bool(
            const std::string& key, const std::string& value)> callback) const override;

private:
    std::unique_ptr<rocksdb::DB> db_;
    bool is_open_;
};

}