#include "interpretation.hpp"
#include "core/interpreter.hpp"
#include "sxs/slp/slp.hpp"
#include <fmt/core.h>

namespace pkg::core::instructions::interpretation {

instruction_interpreter_fn_t get_define(callable_context_if &context,
                                        slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppdefine\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_fn(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppfn\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_debug(callable_context_if &context,
                                       slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppdebug\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_export(callable_context_if &context,
                                        slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppexport\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_if(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppif\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_reflect(callable_context_if &context,
                                         slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppreflect\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_try(callable_context_if &context,
                                     slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cpptry\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_assert(callable_context_if &context,
                                        slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppassert\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_recover(callable_context_if &context,
                                         slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cpprecover\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_eval(callable_context_if &context,
                                      slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppeval\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_apply(callable_context_if &context,
                                       slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppapply\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_match(callable_context_if &context,
                                       slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppmatch\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_cast(callable_context_if &context,
                                      slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppcast\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_do(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppdo\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_done(callable_context_if &context,
                                      slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppdone\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_at(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppat\n");
  return instruction_interpreter_fn_t();
}

instruction_interpreter_fn_t get_eq(callable_context_if &context,
                                    slp::slp_object_c &args_list) {
  fmt::print("[TODO] replace with definition from instructions.cppeq\n");
  return instruction_interpreter_fn_t();
}

} // namespace pkg::core::instructions::interpretation