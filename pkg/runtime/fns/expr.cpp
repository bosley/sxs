#include "runtime/fns/expr.hpp"
#include "runtime/fns/helpers.hpp"
#include "runtime/processor.hpp"
#include "runtime/session/session.hpp"
#include <any>
#include <sstream>

namespace runtime::fns {

function_group_s get_expr_functions(runtime_information_if &runtime_info) {
  function_group_s group;
  group.group_name = "core/expr";

  group.functions["eval"].return_type = slp::slp_type_e::NONE;
  group.functions["eval"].parameters = {{"script_text", slp::slp_type_e::NONE, true}};
  group.functions["eval"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/expr/eval requires script text");
        }

        auto script_obj =
            runtime_info.eval_object(session, list.at(1), context);
        std::string script_text = object_to_storage_string(script_obj);

        auto parse_result = slp::parse(script_text);
        if (parse_result.is_error()) {
          return SLP_ERROR("core/expr/eval parse error");
        }

        return runtime_info.eval_object(session, parse_result.object(),
                                        context);
      };

  return group;
}

} // namespace runtime::fns
