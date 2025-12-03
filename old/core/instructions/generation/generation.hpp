#pragma once

#include "slp/slp.hpp"

namespace pkg::core {
class callable_context_if;
}

namespace pkg::core::instructions::generation {

/// All of the core (c++ defined) functions
enum class hll_instruction_e {
  NOP = 0, // not an instruction in hll
  DEFINE,
  FN,
  DEBUG,
  EXPORT,
  IF,
  REFELCT,
  TRY,
  ASSERT,
  RECOVER,
  EVAL,
  APPLY,
  MATCH,
  CAST,
  DO,
  DONE,
  AT,
  EQ
};

typedef std::vector<std::uint8_t> byte_vector_t;

typedef std::function<byte_vector_t(callable_context_if &context,
                                    slp::slp_object_c &args_list)>
    instruction_generator_fn_t;

extern byte_vector_t make_define(callable_context_if &context,
                                 slp::slp_object_c &args_list);

extern byte_vector_t make_fn(callable_context_if &context,
                             slp::slp_object_c &args_list);

extern byte_vector_t make_debug(callable_context_if &context,
                                slp::slp_object_c &args_list);

extern byte_vector_t make_export(callable_context_if &context,
                                 slp::slp_object_c &args_list);

extern byte_vector_t make_if(callable_context_if &context,
                             slp::slp_object_c &args_list);

extern byte_vector_t make_reflect(callable_context_if &context,
                                  slp::slp_object_c &args_list);

extern byte_vector_t make_try(callable_context_if &context,
                              slp::slp_object_c &args_list);

extern byte_vector_t make_assert(callable_context_if &context,
                                 slp::slp_object_c &args_list);

extern byte_vector_t make_recover(callable_context_if &context,
                                  slp::slp_object_c &args_list);

extern byte_vector_t make_eval(callable_context_if &context,
                               slp::slp_object_c &args_list);

extern byte_vector_t make_apply(callable_context_if &context,
                                slp::slp_object_c &args_list);

extern byte_vector_t make_match(callable_context_if &context,
                                slp::slp_object_c &args_list);

extern byte_vector_t make_cast(callable_context_if &context,
                               slp::slp_object_c &args_list);

extern byte_vector_t make_do(callable_context_if &context,
                             slp::slp_object_c &args_list);

extern byte_vector_t make_done(callable_context_if &context,
                               slp::slp_object_c &args_list);

extern byte_vector_t make_at(callable_context_if &context,
                             slp::slp_object_c &args_list);

extern byte_vector_t make_eq(callable_context_if &context,
                             slp::slp_object_c &args_list);

} // namespace pkg::core::instructions::generation