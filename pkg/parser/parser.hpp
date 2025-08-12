#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pkg::parser {

struct error_s {
  std::size_t pos{0};
  std::string message;
};

struct parser_result_s {
  std::string origin;         // file path or name of segment
  std::vector<uint8_t> data;
  std::vector<error_s> errors;  
};

// Parse a file and attempt to get a result
// errors, if present will exist solely in the errors path
extern parser_result_s parse_file(std::string file_path);


} // namespace pkg::parser