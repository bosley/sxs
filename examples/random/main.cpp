#include <fmt/core.h>
#include "random/string.hpp"
#include "random/generator.hpp"
#include "random/entry.hpp"

int main() {
    pkg::random::random_string_c rng;
    fmt::print("Random string: {}\n", rng.generate_string(32));
    
    pkg::random::generate_random_c<int> int_gen;
    fmt::print("Random int [1-100]: {}\n", int_gen.get_range(1, 100));
    
    std::vector<std::string> options = {"alpha", "beta", "gamma", "delta"};
    pkg::random::random_entry_c<std::string> entry_gen(options);
    fmt::print("Random entry: {}\n", entry_gen.get_value());
    
    return 0;
}
