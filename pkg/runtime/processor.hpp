#pragma once

#include "runtime/events/events.hpp"
#include "runtime/runtime.hpp"
#include "runtime/session/session.hpp"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <slp/buffer.hpp>
#include <slp/slp.hpp>
#include <string>

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

class runtime_information_if {
public:
  virtual ~runtime_information_if() = default;

  virtual logger_t get_logger() = 0;

  virtual slp::slp_object_c
  eval_object(session_c &session, const slp::slp_object_c &obj,
              const std::map<std::string, slp::slp_object_c> &context) = 0;

  virtual std::string object_to_string(const slp::slp_object_c &obj) = 0;

  struct subscription_handler_s {
    session_c *session;
    events::event_category_e category;
    std::uint16_t topic_id;
    slp::slp_buffer_c handler_data;
    std::map<std::uint64_t, std::string> handler_symbols;
    size_t handler_root_offset;
  };

  struct pending_await_s {
    std::condition_variable cv;
    std::mutex mutex;
    bool completed{false};
    slp::slp_object_c result;
  };

  virtual std::vector<subscription_handler_s> *get_subscription_handlers() = 0;
  virtual std::mutex *get_subscription_handlers_mutex() = 0;
  virtual std::map<std::string, std::shared_ptr<pending_await_s>> *
  get_pending_awaits() = 0;
  virtual std::mutex *get_pending_awaits_mutex() = 0;
  virtual std::chrono::seconds get_max_await_timeout() = 0;
};

class processor_c : public events::event_consumer_if,
                    private runtime_information_if {
public:
  using eval_context_s = std::map<std::string, slp::slp_object_c>;

  struct subscription_handler_s {
    session_c *session;
    events::event_category_e category;
    std::uint16_t topic_id;
    slp::slp_buffer_c handler_data;
    std::map<std::uint64_t, std::string> handler_symbols;
    size_t handler_root_offset;
  };

  using function_handler_t = std::function<slp::slp_object_c(
      session_c &, const slp::slp_object_c &, const eval_context_s &)>;

  processor_c(logger_t logger, events::event_system_c &event_system);
  ~processor_c() = default;

  void consume_event(const events::event_s &event) override;

  void register_function(const std::string &name, function_handler_t handler);

private:
  logger_t logger_;
  events::event_system_c &event_system_;
  std::map<std::string, function_handler_t> function_registry_;
  std::vector<subscription_handler_s> subscription_handlers_;
  std::mutex subscription_handlers_mutex_;
  std::map<std::string,
           std::shared_ptr<runtime_information_if::pending_await_s>>
      pending_awaits_;
  std::mutex pending_awaits_mutex_;
  eval_context_s global_context_;

  void register_builtin_functions();

  execution_result_s execute_script(session_c &session,
                                    const std::string &script_text,
                                    const std::string &request_id);

  slp::slp_object_c eval_object(session_c &session,
                                const slp::slp_object_c &obj);

  slp::slp_object_c eval_object_with_context(session_c &session,
                                             const slp::slp_object_c &obj,
                                             const eval_context_s &context);

  slp::slp_object_c call_function(session_c &session, const std::string &name,
                                  const slp::slp_object_c &args,
                                  const eval_context_s &context);

  std::string slp_object_to_string(const slp::slp_object_c &obj) const;

  void send_result_to_session(session_c *session,
                              const execution_result_s &result);

  /*
    Runtime information interface implementation
  */
  logger_t get_logger() override final;
  slp::slp_object_c eval_object(session_c &session,
                                const slp::slp_object_c &obj,
                                const eval_context_s &context) override final;
  std::string object_to_string(const slp::slp_object_c &obj) override final;
  std::vector<runtime_information_if::subscription_handler_s> *
  get_subscription_handlers() override final;
  std::mutex *get_subscription_handlers_mutex() override final;
  std::map<std::string,
           std::shared_ptr<runtime_information_if::pending_await_s>> *
  get_pending_awaits() override final;
  std::mutex *get_pending_awaits_mutex() override final;
  std::chrono::seconds get_max_await_timeout() override final;
};

} // namespace runtime
