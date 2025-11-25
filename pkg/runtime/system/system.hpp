#pragma once

#include "runtime/runtime.hpp"
#include <kvds/kvds.hpp>
#include <memory>

namespace runtime {

class system_c : public runtime_subsystem_if {
public:
  system_c(logger_t logger, const std::string &root_path);
  ~system_c();

  const char *get_name() const override final;
  void initialize(runtime_accessor_t accessor) override final;
  void shutdown() override final;
  bool is_running() const override final;

  /*
    The entity storage is our records of users/entities that can interact with
    the system
  */
  kvds::kv_c *get_entity_store();

  /*
    The session storage is to disk-back all entity actions that are happening
    via a session object. These sessions persist data between related calls.
  */
  kvds::kv_c *get_session_store();

  /*
    The runtime storage is to disk-back all runtime meta-configuration data for
    the server
  */
  kvds::kv_c *get_runtime_store();

  /*
    The datastore storage is where all actual user-facing data is stored.
    The session object itself provides the
  */
  kvds::kv_c *get_datastore_store();

private:
  bool ensure_directory_exists(const std::string &path);

  logger_t logger_;
  const std::string root_path_;
  bool running_;
  runtime_accessor_t accessor_;
  const char *name_{"system_c"};

  std::unique_ptr<kvds::kv_c_distributor_c> distributor_;
  decltype(std::declval<kvds::kv_c_distributor_c>().get_or_create_kv_c(
      "", kvds::kv_c_distributor_c::kv_c_backend_e::DISK)) kv_entity_store_;
  decltype(std::declval<kvds::kv_c_distributor_c>().get_or_create_kv_c(
      "", kvds::kv_c_distributor_c::kv_c_backend_e::DISK)) kv_session_store_;
  decltype(std::declval<kvds::kv_c_distributor_c>().get_or_create_kv_c(
      "", kvds::kv_c_distributor_c::kv_c_backend_e::DISK)) kv_runtime_store_;

  decltype(std::declval<kvds::kv_c_distributor_c>().get_or_create_kv_c(
      "", kvds::kv_c_distributor_c::kv_c_backend_e::DISK)) kv_ds_store_;
};

} // namespace runtime