#pragma once

#include "core/context.hpp"
#include "slp/slp.hpp"

namespace pkg::core::instructions::typechecking {

extern type_info_s typecheck_define(compiler_context_if &context,
                                    slp::slp_object_c &args_list);

extern type_info_s typecheck_fn(compiler_context_if &context,
                                slp::slp_object_c &args_list);

extern type_info_s typecheck_debug(compiler_context_if &context,
                                   slp::slp_object_c &args_list);

extern type_info_s typecheck_if(compiler_context_if &context,
                                slp::slp_object_c &args_list);

extern type_info_s typecheck_reflect(compiler_context_if &context,
                                     slp::slp_object_c &args_list);

extern type_info_s typecheck_try(compiler_context_if &context,
                                 slp::slp_object_c &args_list);

extern type_info_s typecheck_assert(compiler_context_if &context,
                                    slp::slp_object_c &args_list);

extern type_info_s typecheck_recover(compiler_context_if &context,
                                     slp::slp_object_c &args_list);

extern type_info_s typecheck_eval(compiler_context_if &context,
                                  slp::slp_object_c &args_list);

extern type_info_s typecheck_apply(compiler_context_if &context,
                                   slp::slp_object_c &args_list);

extern type_info_s typecheck_match(compiler_context_if &context,
                                   slp::slp_object_c &args_list);

extern type_info_s typecheck_cast(compiler_context_if &context,
                                  slp::slp_object_c &args_list);

extern type_info_s typecheck_do(compiler_context_if &context,
                                slp::slp_object_c &args_list);

extern type_info_s typecheck_done(compiler_context_if &context,
                                  slp::slp_object_c &args_list);

extern type_info_s typecheck_at(compiler_context_if &context,
                                slp::slp_object_c &args_list);

extern type_info_s typecheck_eq(compiler_context_if &context,
                                slp::slp_object_c &args_list);

extern type_info_s typecheck_load(compiler_context_if &context,
                                  slp::slp_object_c &args_list);

extern type_info_s typecheck_define_form(compiler_context_if &context,
                                         slp::slp_object_c &args_list);

} // namespace pkg::core::instructions::typechecking
