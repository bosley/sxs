#pragma once

#include "sxs/slp/slp.hpp"

namespace pkg::core {
class callable_context_if;
}

namespace pkg::core::instructions::interpretation {

typedef std::function<slp::slp_object_c(callable_context_if &context,
                                        slp::slp_object_c &args_list)>
    instruction_interpreter_fn_t;

extern instruction_interpreter_fn_t get_define(callable_context_if &context,
                                               slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_fn(callable_context_if &context,
                                           slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_debug(callable_context_if &context,
                                              slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_export(callable_context_if &context,
                                               slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_if(callable_context_if &context,
                                           slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_reflect(callable_context_if &context,
                                                slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_try(callable_context_if &context,
                                            slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_assert(callable_context_if &context,
                                               slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_recover(callable_context_if &context,
                                                slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_eval(callable_context_if &context,
                                             slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_apply(callable_context_if &context,
                                              slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_match(callable_context_if &context,
                                              slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_cast(callable_context_if &context,
                                             slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_do(callable_context_if &context,
                                           slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_done(callable_context_if &context,
                                             slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_at(callable_context_if &context,
                                           slp::slp_object_c &args_list);

extern instruction_interpreter_fn_t get_eq(callable_context_if &context,
                                           slp::slp_object_c &args_list);

} // namespace pkg::core::instructions::interpretation