#include <fmt/core.h>
#include <string>
#include <vector>

#include "parser/parser.hpp"


int main(int argc, char **argv) {

  std::vector<std::string> args(argv, argv + argc);

  if (args.size() == 1) {
    fmt::print("no input; expected file");
    return 0;
  }

  for (auto i = 1; i < args.size(); i++) {
    if (args[i] == "--help" || args[i] == "-h") {
      fmt::print("{} <file path>\n", args[0]);
      return 0;
    }
  }

  auto result = pkg::parser::parse_file(args[1]);

  fmt::print("origin: {}\n", result.origin);
  fmt::print("data  : {}\n", result.data.size());
  fmt::print("errors: {}\n", result.errors.size());

  return 0;
}