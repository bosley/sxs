#pragma once

#include "types/lifetime.hpp"
#include "types/shared_obj.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace kvds {

class kv_stat {
public:
  virtual ~kv_stat() = default;
  virtual bool is_open() const = 0;
};

class kv_reader_c {
public:
  virtual ~kv_reader_c() = default;
  virtual bool get(const std::string &key, std::string &value) = 0;
  virtual bool exists(const std::string &key) const = 0;
  virtual void
  iterate(const std::string &prefix,
          std::function<bool(const std::string &key, const std::string &value)>
              callback) const = 0;
};

class kv_writer_c {
public:
  virtual ~kv_writer_c() = default;
  virtual bool set(const std::string &key, const std::string &value) = 0;
  virtual bool del(const std::string &key) = 0;
  virtual bool
  set_batch(const std::map<std::string, std::string> &kv_pairs) = 0;
  virtual bool delete_batch(const std::vector<std::string> &keys) = 0;
  virtual bool set_nx(const std::string &key, const std::string &value) = 0;
  virtual bool compare_and_swap(const std::string &key,
                                const std::string &expected_value,
                                const std::string &new_value) = 0;
};

class kv_c : public kv_reader_c, public kv_writer_c, public kv_stat {
public:
  virtual ~kv_c() = default;
};

class kv_c_distributor_c {
private:
  class lifetime_observer_c
      : public pkg::types::lifetime_tagged_c::observer_if {
  public:
    lifetime_observer_c(kv_c_distributor_c &distributor);
    void death_ind(const std::size_t tag) override;

  private:
    kv_c_distributor_c &distributor_;
  };

  class kv_wrapper_c : public pkg::types::shared_c, public kv_c {
  public:
    kv_wrapper_c(std::unique_ptr<kv_c> store,
                 pkg::types::lifetime_tagged_c::observer_if &observer,
                 std::size_t tag);
    ~kv_wrapper_c() = default;

    bool is_open() const override;
    bool set(const std::string &key, const std::string &value) override;
    bool get(const std::string &key, std::string &value) override;
    bool del(const std::string &key) override;
    bool exists(const std::string &key) const override;
    bool set_batch(const std::map<std::string, std::string> &kv_pairs) override;
    bool delete_batch(const std::vector<std::string> &keys) override;
    bool set_nx(const std::string &key, const std::string &value) override;
    bool compare_and_swap(const std::string &key,
                          const std::string &expected_value,
                          const std::string &new_value) override;
    void iterate(
        const std::string &prefix,
        std::function<bool(const std::string &key, const std::string &value)>
            callback) const override;

  private:
    std::unique_ptr<kv_c> store_;
    pkg::types::lifetime_tagged_c lifetime_;
  };

  std::string base_path_;
  std::map<std::string, pkg::types::shared_obj_c<kv_wrapper_c>> memory_stores_;
  std::map<std::string, pkg::types::shared_obj_c<kv_wrapper_c>> disk_stores_;
  std::map<std::size_t, std::string> tag_to_id_;
  std::size_t next_tag_;
  std::mutex mutex_;
  lifetime_observer_c observer_;

  void on_wrapper_death(const std::size_t tag);

public:
  enum class kv_c_backend_e {
    MEMORY = 0,
    DISK = 1,
  };
  kv_c_distributor_c(const std::string &path);
  ~kv_c_distributor_c();

  std::optional<pkg::types::shared_obj_c<kv_wrapper_c>>
  get_or_create_kv_c(const std::string &unique_identifier,
                     const kv_c_backend_e backend);
};

} // namespace kvds