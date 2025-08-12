#include "parser/parser.hpp"
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <streambuf>

namespace pkg::parser {

parser_result_s parse_file(std::string file_path) {

  // 
  parser_result_s result{
    .origin = file_path,
    .data = {},
    .errors = {}
  };

  std::ifstream f(file_path);

  if (!f.is_open()) {
    result.errors.push_back({
      .pos = 0,
      .message = fmt::format("Failed to open file: {}", file_path)
    });
    return result;
  }

  std::string raw((std::istreambuf_iterator<char>(f)),
    std::istreambuf_iterator<char>());

  f.close();

  fmt::print("raw\n\tsize: {}\n\tdata: {}\n", raw.size(), raw);


  return result;
}




} // namespace pkg:parser