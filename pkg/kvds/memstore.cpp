#include "kvds/memstore.hpp"

namespace kvds {

memstore_c::memstore_c() : is_open_(false) {}

bool memstore_c::open(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (is_open_) {
    return false;
  }

  is_open_ = true;
  return true;
}

bool memstore_c::close() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  data_.clear();
  is_open_ = false;
  return true;
}

bool memstore_c::is_open() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return is_open_;
}

bool memstore_c::set(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  data_[key] = value;
  return true;
}

bool memstore_c::get(const std::string &key, std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  auto it = data_.find(key);
  if (it != data_.end()) {
    value = it->second;
    return true;
  }
  return false;
}

bool memstore_c::del(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  return data_.erase(key) > 0;
}

bool memstore_c::exists(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  return data_.find(key) != data_.end();
}

bool memstore_c::set_batch(const std::map<std::string, std::string> &kv_pairs) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  for (const auto &pair : kv_pairs) {
    data_[pair.first] = pair.second;
  }
  return true;
}

bool memstore_c::delete_batch(const std::vector<std::string> &keys) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  for (const auto &key : keys) {
    data_.erase(key);
  }
  return true;
}

bool memstore_c::set_nx(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  if (data_.find(key) != data_.end()) {
    return false;
  }

  data_[key] = value;
  return true;
}

bool memstore_c::compare_and_swap(const std::string &key,
                                  const std::string &expected_value,
                                  const std::string &new_value) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return false;
  }

  auto it = data_.find(key);
  if (it == data_.end()) {
    return false;
  }

  if (it->second != expected_value) {
    return false;
  }

  it->second = new_value;
  return true;
}

void memstore_c::iterate(
    const std::string &prefix,
    std::function<bool(const std::string &key, const std::string &value)>
        callback) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_open_) {
    return;
  }

  auto it = data_.lower_bound(prefix);

  while (it != data_.end() && it->first.substr(0, prefix.size()) == prefix) {
    bool continue_iteration = callback(it->first, it->second);
    if (!continue_iteration) {
      break;
    }
    ++it;
  }
}

} // namespace kvds
