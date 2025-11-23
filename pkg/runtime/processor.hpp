#pragma once

#include "runtime/events/events.hpp"
#include "runtime/runtime.hpp"
#include "runtime/session/session.hpp"
#include <functional>
#include <map>
#include <mutex>
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

class processor_c : public events::event_consumer_if {
public:
  struct eval_context_s {
    std::map<std::string, slp::slp_object_c> bindings;
  };

  using function_handler_t =
      std::function<slp::slp_object_c(session_c *, const slp::slp_object_c &,
                                      const eval_context_s &)>;

  processor_c(logger_t logger, events::event_system_c *event_system);
  ~processor_c() = default;

  void consume_event(const events::event_s &event) override;

  void register_function(const std::string &name, function_handler_t handler);

private:

  struct subscription_handler_s {
    session_c *session;
    events::event_category_e category;
    std::uint16_t topic_id;
    std::vector<std::uint8_t> handler_data;
    std::map<std::uint64_t, std::string> handler_symbols;
    size_t handler_root_offset;
  };

  logger_t logger_;
  events::event_system_c *event_system_;
  std::map<std::string, function_handler_t> function_registry_;
  std::vector<subscription_handler_s> subscription_handlers_;
  std::mutex subscription_handlers_mutex_;
  eval_context_s global_context_;

  void register_builtin_functions();

  execution_result_s execute_script(session_c *session,
                                    const std::string &script_text,
                                    const std::string &request_id);

  slp::slp_object_c eval_object(session_c *session,
                                const slp::slp_object_c &obj);

  slp::slp_object_c eval_object_with_context(session_c *session,
                                             const slp::slp_object_c &obj,
                                             const eval_context_s &context);

  slp::slp_object_c call_function(session_c *session, const std::string &name,
                                  const slp::slp_object_c &args,
                                  const eval_context_s &context);
  
  std::string slp_object_to_string(const slp::slp_object_c &obj) const;

  void send_result_to_session(session_c *session,
                              const execution_result_s &result);
};

} // namespace runtime
