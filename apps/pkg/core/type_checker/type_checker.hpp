#pragma once

#include "core/compiler_context.hpp"
#include "core/core.hpp"
#include <string>
#include <vector>

namespace pkg::core::type_checker {

class type_checker_c {
public:
  explicit type_checker_c(logger_t logger,
                          std::vector<std::string> include_paths,
                          std::string working_directory);
  ~type_checker_c();

  bool check(const std::string &file_path);

  bool check_source(const std::string &source,
                    const std::string &source_name = "<string>");

  type_info_s check_expression(const std::string &source,
                               const std::string &source_name = "<expr>");

private:
  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
};

} // namespace pkg::core::type_checker
