#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace pkg::core {

class callable_context_if;

namespace kernels {
class kernel_manager_c;
}

typedef std::shared_ptr<spdlog::logger> logger_t;

struct option_s {
  std::string file_path;
  std::vector<std::string> include_paths;
  std::string working_directory;
  logger_t logger;
};

class core_c {
public:
  explicit core_c(const option_s &options);
  ~core_c();

  int run();

private:
  option_s options_;
  std::unique_ptr<kernels::kernel_manager_c> kernel_manager_;
};

} // namespace pkg::core