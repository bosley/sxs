#pragma once

#include <slp/slp.hpp>
#include <string>
#include <vector>

namespace pkg::core {

struct type_info_s;

std::string type_to_string(const type_info_s &type);

std::string type_enum_to_string(slp::slp_type_e type);

struct source_location_s {
  std::string file_path;
  std::string source_code;
  size_t byte_offset;
};

struct formatted_error_s {
  std::string title;
  std::string message;
  std::vector<std::string> context_lines;
  size_t error_line;
  size_t error_column;
};

std::string format_type_error(const std::string &context_name,
                              const std::string &error_type,
                              const type_info_s &expected,
                              const type_info_s &actual,
                              const source_location_s &location);

std::string colorize_error_output(const formatted_error_s &error);

std::pair<size_t, size_t> byte_offset_to_line_col(const std::string &source,
                                                  size_t byte_offset);

std::vector<std::string> get_context_lines(const std::string &source,
                                           size_t line_number,
                                           size_t context_size = 3);

} // namespace pkg::core
