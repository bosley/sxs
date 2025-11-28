#pragma once

#include "core/core.hpp"

namespace pkg::core::kernels {

class kernel_manager_c {
public:
  explicit kernel_manager_c(logger_t logger,
                            std::vector<std::string> include_paths,
                            std::string working_directory);
  ~kernel_manager_c() = default;

  kernel_manager_c(const kernel_manager_c &) = delete;
  kernel_manager_c &operator=(const kernel_manager_c &) = delete;

private:
  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
};

} // namespace pkg::core::kernels