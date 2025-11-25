#include "runtime/fns/util.hpp"
#include "runtime/fns/helpers.hpp"
#include "runtime/processor.hpp"
#include "runtime/session/session.hpp"
#include <sstream>

namespace runtime::fns {

function_group_s get_util_functions(runtime_information_if &runtime_info) {
  function_group_s group;
  group.group_name = "core/util";

  group.functions["log"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["log"].parameters = {{"message", slp::slp_type_e::NONE}};
  group.functions["log"].is_variadic = true;
  group.functions["log"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/util/log requires message");
        }

        std::stringstream ss;
        for (size_t i = 1; i < list.size(); i++) {
          if (i > 1) {
            ss << " ";
          }
          auto msg_result =
              runtime_info.eval_object(session, list.at(i), context);
          ss << runtime_info.object_to_string(msg_result);
        }

        std::string message = ss.str();
        logger->info("[session:{}] {}", session.get_id(), message);
        return SLP_BOOL(true);
      };

  return group;
}

} // namespace runtime::fns
