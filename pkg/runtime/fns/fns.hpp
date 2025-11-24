#pragma once
#include "runtime/events/events.hpp"
#include "runtime/runtime.hpp"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <slp/buffer.hpp>
#include <slp/slp.hpp>
#include <string>
#include <vector>

namespace runtime {
class session_c;
class processor_c;
} // namespace runtime

namespace runtime::fns {

struct function_group_s {
  const char *group_name;
  std::map<std::string, std::function<slp::slp_object_c(
                            session_c *, const slp::slp_object_c &,
                            const std::map<std::string, slp::slp_object_c> &)>>
      functions;
};

class function_provider_if {
public:
  virtual ~function_provider_if() = default;

  virtual logger_t get_logger() = 0;

  virtual slp::slp_object_c
  eval_object(session_c *session, const slp::slp_object_c &obj,
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

std::vector<function_group_s>
get_all_function_groups(function_provider_if *provider);

} // namespace runtime::fns