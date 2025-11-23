#include "runtime/system/system.hpp"
#include "runtime/system/const.hpp"
#include <filesystem>

namespace runtime {

system_c::system_c(logger_t logger, std::string root_path) 
  : logger_(logger), root_path_(root_path), running_(false) {}

system_c::~system_c() {
  if (running_) {
    shutdown();
  }
}

const char* system_c::get_name() const {
  return name_;
}

bool system_c::ensure_directory_exists(const std::string& path) {
  if (path.empty()) {
    return true;
  }
  
  try {
    if (std::filesystem::exists(path)) {
      if (!std::filesystem::is_directory(path)) {
        logger_->error("[{}] Path exists but is not a directory: {}", name_, path);
        return false;
      }
      logger_->debug("[{}] Directory already exists: {}", name_, path);
      return true;
    }
    
    if (std::filesystem::create_directories(path)) {
      logger_->info("[{}] Created directory: {}", name_, path);
      return true;
    }
    
    logger_->error("[{}] Failed to create directory: {}", name_, path);
    return false;
    
  } catch (const std::filesystem::filesystem_error& e) {
    logger_->error("[{}] Filesystem error for path {}: {}", name_, path, e.what());
    return false;
  }
}

void system_c::initialize(runtime_accessor_t accessor) {
  accessor_ = accessor;
  
  logger_->info("[{}] Initializing with root path: {}", name_, root_path_);
  
  if (root_path_.empty()) {
    accessor_->raise_warning("Root path is empty, using current directory");
    root_path_ = ".";
  }
  
  if (!ensure_directory_exists(root_path_)) {
    accessor_->raise_error("Failed to ensure root directory exists");
    return;
  }
  
  logger_->info("[{}] Creating kvds distributor with base path: {}", name_, root_path_);
  
  try {
    distributor_ = std::make_unique<kvds::kv_c_distributor_c>(root_path_);
    logger_->info("[{}] Successfully created kvds distributor", name_);
  } catch (const std::exception& e) {
    logger_->error("[{}] Failed to create distributor: {}", name_, e.what());
    accessor_->raise_error("Failed to create kvds distributor");
    return;
  }


  kv_entity_store_ = distributor_->get_or_create_kv_c(
    std::string(system::KV_ENTITY_CONTEXT_MARKER), 
    kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

  kv_session_store_ = distributor_->get_or_create_kv_c(
    std::string(system::KV_SESSION_CONTEXT_MARKER), 
    kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

  kv_runtime_store_ = distributor_->get_or_create_kv_c(
    std::string(system::KV_RUNTIME_CONTEXT_MARKER), 
    kvds::kv_c_distributor_c::kv_c_backend_e::DISK);

  kv_ds_store_ = distributor_->get_or_create_kv_c(
    std::string(system::KV_DATASTORE_CONTEXT_MARKER), 
    kvds::kv_c_distributor_c::kv_c_backend_e::DISK);
  
  if (!kv_entity_store_.has_value()) {
    accessor_->raise_error("Failed to create entity context store");
    return;
  }
  
  if (!kv_session_store_.has_value()) {
    accessor_->raise_error("Failed to create session context store");
    return;
  }

  if (!kv_runtime_store_.has_value()) {
    accessor_->raise_error("Failed to create runtime context store");
    return;
  }

  if (!kv_ds_store_.has_value()) {
    accessor_->raise_error("Failed to create datastore context store");
    return;
  }

  running_ = true;
}

void system_c::shutdown() {
  logger_->info("[{}] Shutting down", name_);
  
  kv_entity_store_ = std::nullopt;
  kv_session_store_ = std::nullopt;
  kv_runtime_store_ = std::nullopt;
  kv_ds_store_ = std::nullopt;
  
  if (distributor_) {
    logger_->info("[{}] Destroying kvds distributor", name_);
    distributor_.reset();
  }
  
  running_ = false;
}

bool system_c::is_running() const { 
  return running_; 
}

kvds::kv_c* system_c::get_entity_store() {
  return kv_entity_store_.has_value() ? kv_entity_store_.value().ptr() : nullptr;
}

kvds::kv_c* system_c::get_session_store() {
  return kv_session_store_.has_value() ? kv_session_store_.value().ptr() : nullptr;
}

kvds::kv_c* system_c::get_runtime_store() {
  return kv_runtime_store_.has_value() ? kv_runtime_store_.value().ptr() : nullptr;
}

} // namespace runtime