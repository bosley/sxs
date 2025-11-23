#pragma once

#include "kvds/kvds.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace kvds {

class memstore_c : public kv_c {
public:
  memstore_c(const memstore_c &) = delete;
  memstore_c(memstore_c &&) = delete;
  memstore_c &operator=(const memstore_c &) = delete;
  memstore_c &operator=(memstore_c &&) = delete;

  memstore_c();
  ~memstore_c() = default;

  bool open(const std::string &path);
  bool close();

  bool is_open() const override;
  bool set(const std::string &key, const std::string &value) override;
  bool get(const std::string &key, std::string &value) override;
  bool del(const std::string &key) override;
  bool exists(const std::string &key) const override;
  bool set_batch(const std::map<std::string, std::string> &kv_pairs) override;
  bool set_nx(const std::string &key, const std::string &value) override;
  bool compare_and_swap(const std::string &key,
                        const std::string &expected_value,
                        const std::string &new_value) override;
  void
  iterate(const std::string &prefix,
          std::function<bool(const std::string &key, const std::string &value)>
              callback) const override;

private:
  mutable std::mutex mutex_;
  std::map<std::string, std::string> data_;
  bool is_open_;
};

} // namespace kvds
