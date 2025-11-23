#pragma once

#include "runtime/events/events.hpp"
#include "runtime/runtime.hpp"
#include "runtime/session/session.hpp"
#include <functional>
#include <map>
#include <slp/slp.hpp>
#include <string>
#include <variant>

namespace runtime {

struct execution_request_s {
  session_c *session;
  std::string script_text;
  std::string request_id;
};

struct execution_result_s {
  std::string request_id;
  bool success;
  std::string result_data;
  std::string error_message;
};

using function_result_t = std::variant<std::int64_t, double, std::string, bool>;

class processor_c : public events::event_consumer_if {
public:
  using function_handler_t =
      std::function<function_result_t(session_c *, const slp::slp_object_c &)>;

  processor_c(logger_t logger, events::event_system_c *event_system);
  ~processor_c() = default;

  void consume_event(const events::event_s &event) override;

  void register_function(const std::string &name, function_handler_t handler);

private:
  struct eval_context_s {
    std::map<std::string, function_result_t> bindings;
  };

  struct subscription_handler_s {
    session_c *session;
    std::uint16_t topic_id;
    std::vector<std::uint8_t> handler_data;
    std::map<std::uint64_t, std::string> handler_symbols;
    size_t handler_root_offset;
  };

  logger_t logger_;
  events::event_system_c *event_system_;
  std::map<std::string, function_handler_t> function_registry_;
  std::vector<subscription_handler_s> subscription_handlers_;
  const eval_context_s *current_context_;

  void register_builtin_functions();

  execution_result_s execute_script(session_c *session,
                                    const std::string &script_text,
                                    const std::string &request_id);

  function_result_t eval_object(session_c *session,
                                const slp::slp_object_c &obj);

  function_result_t eval_object_with_context(session_c *session,
                                             const slp::slp_object_c &obj,
                                             const eval_context_s &context);

  function_result_t call_function(session_c *session, const std::string &name,
                                  const slp::slp_object_c &args);

  void send_result_to_session(session_c *session,
                              const execution_result_s &result);
};

} // namespace runtime
