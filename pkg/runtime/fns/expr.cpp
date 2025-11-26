#include "runtime/fns/expr.hpp"
#include "runtime/fns/helpers.hpp"
#include "runtime/processor.hpp"
#include "runtime/session/session.hpp"

namespace runtime::fns {

function_group_s get_expr_functions(runtime_information_if &runtime_info) {
  function_group_s group;
  group.group_name = "core/expr";

  group.functions["eval"].return_type = slp::slp_type_e::NONE;
  group.functions["eval"].parameters = {
      {"script_text", slp::slp_type_e::NONE, true}};
  group.functions["eval"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/expr/eval requires script text");
        }

        auto script_obj =
            runtime_info.eval_object(session, list.at(1), context);
        
        /*
            Despite SLP being homoiconic, we take a step to compress SLP objects (a form of hydration)
            when parsing them. We can execute them directly, but 'eval' can take
            a raw unprocessed text (a string) and evaluate that as well

            So here, we check if we have a raw yet-to-be parsed string. If so
            then we parse it before running

            Otherwise we just eval the script object directlry
        */
        if (script_obj.type() == slp::slp_type_e::DQ_LIST) {
          auto script_text = script_obj.as_string().to_string();
          auto parse_result = slp::parse(script_text);
          if (parse_result.is_error()) {
            return SLP_ERROR("core/expr/eval parse error");
          }
          return runtime_info.eval_object(session, parse_result.object(),
                                        context);
        }

        // Already ready to be evaluated
        return runtime_info.eval_object(session, script_obj,
                                        context);
      };

  group.functions["proc"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["proc"].parameters = {
      {"processor_id", slp::slp_type_e::INTEGER, false},
      {"body", slp::slp_type_e::BRACE_LIST, false}};
  group.functions["proc"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 3) {
          return SLP_ERROR("core/expr/proc requires processor_id and body");
        }

        auto processor_id_obj = list.at(1);
        auto body_obj = list.at(2);

        if (processor_id_obj.type() != slp::slp_type_e::INTEGER) {
          return SLP_ERROR("processor_id must be integer");
        }

        if (body_obj.type() != slp::slp_type_e::BRACE_LIST) {
          return SLP_ERROR("body must be a brace list {}");
        }

        std::uint16_t processor_id =
            static_cast<std::uint16_t>(processor_id_obj.as_int());
        std::string script_text = runtime_info.object_to_string(body_obj);

        publish_result_e result = runtime_info.publish_to_processor(
            session, processor_id, script_text, "proc_exec");

        switch (result) {
        case publish_result_e::OK:
          break;
        case publish_result_e::NO_PRODUCER:
          return SLP_ERROR("core/expr/proc failed (no producer)");
        case publish_result_e::NO_TOPIC_WRITER:
          return SLP_ERROR("core/expr/proc failed (processor not configured)");
        default:
          return SLP_ERROR("core/expr/proc failed (unknown error)");
        }

        logger->debug("[expr] proc sent to processor {} with body {}",
                      processor_id, script_text);
        return SLP_BOOL(true);
      };

  return group;
}

} // namespace runtime::fns
