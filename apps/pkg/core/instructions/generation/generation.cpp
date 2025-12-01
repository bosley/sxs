#pragma once

#include "generation.hpp"
#include "core/interpreter.hpp"
#include "sxs/slp/slp.hpp"
#include <fmt/core.h>

// FOR NOW WE JUST STUB - Generation not yet on the target, we are just wiring for the future

namespace pkg::core::instructions::generation {

byte_vector_t make_define(
    callable_context_if &context,
    slp::slp_object_c &args_list) {
    fmt::print("[GENERATION] make_define\n");
    return byte_vector_t();
}

} // namespace pkg::core::instructions::generation