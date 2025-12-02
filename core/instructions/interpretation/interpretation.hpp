#pragma once

#include "slp/slp.hpp"

namespace pkg::core {
class callable_context_if;
}

namespace pkg::core::instructions::interpretation {

extern slp::slp_object_c interpret_define(callable_context_if &context,
                                          slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_fn(callable_context_if &context,
                                      slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_debug(callable_context_if &context,
                                         slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_export(callable_context_if &context,
                                          slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_if(callable_context_if &context,
                                      slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_reflect(callable_context_if &context,
                                           slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_try(callable_context_if &context,
                                       slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_assert(callable_context_if &context,
                                          slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_recover(callable_context_if &context,
                                           slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_eval(callable_context_if &context,
                                        slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_apply(callable_context_if &context,
                                         slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_match(callable_context_if &context,
                                         slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_cast(callable_context_if &context,
                                        slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_do(callable_context_if &context,
                                      slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_done(callable_context_if &context,
                                        slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_at(callable_context_if &context,
                                      slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_eq(callable_context_if &context,
                                      slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_datum_debug(callable_context_if &context,
                                               slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_datum_import(callable_context_if &context,
                                                slp::slp_object_c &args_list);

extern slp::slp_object_c interpret_datum_load(callable_context_if &context,
                                              slp::slp_object_c &args_list);

extern slp::slp_object_c
interpret_datum_define_form(callable_context_if &context,
                            slp::slp_object_c &args_list);

} // namespace pkg::core::instructions::interpretation