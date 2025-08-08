#include <fmt/format.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

int main() {
  json obj = {{"name", "insula"},
              {"features", {"fast", "modern", "header-only"}},
              {"version", 1}};

  std::string dumped = obj.dump(2);
  fmt::print("{}\n", dumped);
  return 0;
}
