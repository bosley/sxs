#pragma once

#include "core/interpreter.hpp"
#include <map>
#include <string>

namespace pkg::core::instructions {

extern std::map<std::string, pkg::core::callable_symbol_s>
get_standard_callable_symbols();

}