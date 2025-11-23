#include "kvds/datastore.hpp"
#include "kvds/kvds.hpp"
#include "kvds/memstore.hpp"
#include <filesystem>

namespace kvds {

kv_c_distributor_c::lifetime_observer_c::lifetime_observer_c(
    kv_c_distributor_c &distributor)
    : distributor_(distributor) {}

void kv_c_distributor_c::lifetime_observer_c::death_ind(const std::size_t tag) {
  distributor_.on_wrapper_death(tag);
}

kv_c_distributor_c::kv_wrapper_c::kv_wrapper_c(
    std::unique_ptr<kv_c> store,
    pkg::types::lifetime_tagged_c::observer_if &observer, std::size_t tag)
    : store_(std::move(store)), lifetime_(observer, tag) {}

bool kv_c_distributor_c::kv_wrapper_c::is_open() const {
  return store_->is_open();
}

bool kv_c_distributor_c::kv_wrapper_c::set(const std::string &key,
                                           const std::string &value) {
  return store_->set(key, value);
}

bool kv_c_distributor_c::kv_wrapper_c::get(const std::string &key,
                                           std::string &value) {
  return store_->get(key, value);
}

bool kv_c_distributor_c::kv_wrapper_c::del(const std::string &key) {
  return store_->del(key);
}

bool kv_c_distributor_c::kv_wrapper_c::exists(const std::string &key) const {
  return store_->exists(key);
}

bool kv_c_distributor_c::kv_wrapper_c::set_batch(
    const std::map<std::string, std::string> &kv_pairs) {
  return store_->set_batch(kv_pairs);
}

bool kv_c_distributor_c::kv_wrapper_c::delete_batch(
    const std::vector<std::string> &keys) {
  return store_->delete_batch(keys);
}

bool kv_c_distributor_c::kv_wrapper_c::set_nx(const std::string &key,
                                               const std::string &value) {
  return store_->set_nx(key, value);
}

bool kv_c_distributor_c::kv_wrapper_c::compare_and_swap(
    const std::string &key, const std::string &expected_value,
    const std::string &new_value) {
  return store_->compare_and_swap(key, expected_value, new_value);
}

void kv_c_distributor_c::kv_wrapper_c::iterate(
    const std::string &prefix,
    std::function<bool(const std::string &key, const std::string &value)>
        callback) const {
  store_->iterate(prefix, callback);
}

kv_c_distributor_c::kv_c_distributor_c(const std::string &path)
    : base_path_(path), next_tag_(0), observer_(*this) {}

kv_c_distributor_c::~kv_c_distributor_c() {
  std::lock_guard<std::mutex> lock(mutex_);

  memory_stores_.clear();
  disk_stores_.clear();
  tag_to_id_.clear();
}

void kv_c_distributor_c::on_wrapper_death(const std::size_t tag) {}

std::optional<pkg::types::shared_obj_c<kv_c_distributor_c::kv_wrapper_c>>
kv_c_distributor_c::get_or_create_kv_c(const std::string &unique_identifier,
                                       const kv_c_backend_e backend) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto &store_map =
      (backend == kv_c_backend_e::MEMORY) ? memory_stores_ : disk_stores_;

  auto it = store_map.find(unique_identifier);
  if (it != store_map.end()) {
    return it->second;
  }

  std::unique_ptr<kv_c> new_store;

  if (backend == kv_c_backend_e::MEMORY) {
    auto mem_store = std::make_unique<memstore_c>();
    if (!mem_store->open("")) {
      return std::nullopt;
    }
    new_store = std::move(mem_store);
  } else {
    std::string disk_path = base_path_;
    if (!disk_path.empty() && disk_path.back() != '/') {
      disk_path += '/';
    }
    disk_path += unique_identifier;

    std::filesystem::create_directories(disk_path);

    auto disk_store = std::make_unique<datastore_c>();
    if (!disk_store->open(disk_path)) {
      return std::nullopt;
    }
    new_store = std::move(disk_store);
  }

  std::size_t tag = next_tag_++;
  auto *wrapper = new kv_wrapper_c(std::move(new_store), observer_, tag);

  auto shared_wrapper = pkg::types::shared_obj_c<kv_wrapper_c>(wrapper);
  store_map[unique_identifier] = shared_wrapper;
  tag_to_id_[tag] = unique_identifier;

  return shared_wrapper;
}

} // namespace kvds
