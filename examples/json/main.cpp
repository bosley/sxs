#include <nlohmann/json.hpp>
#include <fmt/format.h>

using nlohmann::json;

int main() {
    json obj = {
        {"name", "insula"},
        {"features", {"fast", "modern", "header-only"}},
        {"version", 1}
    };

    std::string dumped = obj.dump(2);
    fmt::print("{}\n", dumped);
    return 0;
}


