#pragma once

#include "runtime/runtime.hpp"
#include <kvds/kvds.hpp>
#include <memory>

namespace runtime {

class system_c : public runtime_subsystem_if {
public:
  system_c(logger_t logger, std::string root_path);
  ~system_c();

  const char* get_name() const override final;
  void initialize(runtime_accessor_t accessor) override final;
  void shutdown() override final;
  bool is_running() const override final;


private:
  bool ensure_directory_exists(const std::string& path);

  logger_t logger_;
  std::string root_path_;
  bool running_;
  runtime_accessor_t accessor_;
  const char* name_{"system_c"};
  
  std::unique_ptr<kvds::kv_c_distributor_c> distributor_;
};

} // namespace runtime