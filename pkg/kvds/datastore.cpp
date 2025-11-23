#include "kvds/datastore.hpp"
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>

namespace kvds {

datastore_c::datastore_c() : db_(nullptr), is_open_(false) {}

datastore_c::~datastore_c() {
  if (is_open_) {
    close();
  }
}

bool datastore_c::open(const std::string &path) {
  if (is_open_) {
    return false;
  }

  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::DB *raw_db;
  rocksdb::Status status = rocksdb::DB::Open(options, path, &raw_db);

  if (status.ok()) {
    db_.reset(raw_db);
    is_open_ = true;
    return true;
  }

  return false;
}

bool datastore_c::close() {
  if (!is_open_) {
    return false;
  }

  db_.reset();
  is_open_ = false;
  return true;
}

bool datastore_c::is_open() const { return is_open_; }

bool datastore_c::set(const std::string &key, const std::string &value) {
  if (!is_open_) {
    return false;
  }

  rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
  return status.ok();
}

bool datastore_c::get(const std::string &key, std::string &value) {
  if (!is_open_) {
    return false;
  }

  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
  return status.ok();
}

bool datastore_c::del(const std::string &key) {
  if (!is_open_) {
    return false;
  }

  rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
  return status.ok();
}

bool datastore_c::exists(const std::string &key) const {
  if (!is_open_) {
    return false;
  }
  std::string value;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
  return status.ok();
}

bool datastore_c::set_batch(
    const std::map<std::string, std::string> &kv_pairs) {
  if (!is_open_) {
    return false;
  }

  rocksdb::WriteBatch batch;

  for (const auto &pair : kv_pairs) {
    batch.Put(pair.first, pair.second);
  }

  rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch);
  return status.ok();
}

bool datastore_c::delete_batch(const std::vector<std::string> &keys) {
  if (!is_open_) {
    return false;
  }

  rocksdb::WriteBatch batch;

  for (const auto &key : keys) {
    batch.Delete(key);
  }

  rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch);
  return status.ok();
}

bool datastore_c::set_nx(const std::string &key, const std::string &value) {
  if (!is_open_) {
    return false;
  }

  std::string existing_value;
  rocksdb::Status status =
      db_->Get(rocksdb::ReadOptions(), key, &existing_value);

  if (status.ok()) {
    return false;
  }

  status = db_->Put(rocksdb::WriteOptions(), key, value);
  return status.ok();
}

bool datastore_c::compare_and_swap(const std::string &key,
                                   const std::string &expected_value,
                                   const std::string &new_value) {
  if (!is_open_) {
    return false;
  }

  std::string current_value;
  rocksdb::Status status =
      db_->Get(rocksdb::ReadOptions(), key, &current_value);

  if (!status.ok()) {
    return false;
  }

  if (current_value != expected_value) {
    return false;
  }

  status = db_->Put(rocksdb::WriteOptions(), key, new_value);
  return status.ok();
}

void datastore_c::iterate(
    const std::string &prefix,
    std::function<bool(const std::string &key, const std::string &value)>
        callback) const {
  if (!is_open_) {
    return;
  }

  std::unique_ptr<rocksdb::Iterator> it(
      db_->NewIterator(rocksdb::ReadOptions()));

  for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix);
       it->Next()) {
    std::string key = it->key().ToString();
    std::string value = it->value().ToString();

    bool continue_iteration = callback(key, value);
    if (!continue_iteration) {
      break;
    }
  }
}

} // namespace kvds