#include "runtime/processor.hpp"
#include "runtime/fns/fns.hpp"
#include <any>
#include <chrono>
#include <spdlog/spdlog.h>

constexpr std::chrono::seconds MAX_AWAIT_TIMEOUT = std::chrono::seconds(5);

namespace runtime {

std::string
processor_c::slp_object_to_string(const slp::slp_object_c &obj) const {
  auto type = obj.type();
  if (type == slp::slp_type_e::INTEGER) {
    return std::to_string(obj.as_int());
  } else if (type == slp::slp_type_e::REAL) {
    return std::to_string(obj.as_real());
  } else if (type == slp::slp_type_e::SYMBOL) {
    return obj.as_symbol();
  } else if (type == slp::slp_type_e::DQ_LIST) {
    return obj.as_string().to_string();
  } else if (type == slp::slp_type_e::ERROR) {
    return obj.as_string().to_string();
  }
  return "nil";
}

processor_c::processor_c(logger_t logger, events::event_system_c *event_system)
    : logger_(logger), event_system_(event_system) {
  logger_->info("[processor_c] Initializing processor");

  global_context_.bindings["$CHANNEL_A"] = slp::parse("A").take();
  global_context_.bindings["$CHANNEL_B"] = slp::parse("B").take();
  global_context_.bindings["$CHANNEL_C"] = slp::parse("C").take();
  global_context_.bindings["$CHANNEL_D"] = slp::parse("D").take();
  global_context_.bindings["$CHANNEL_E"] = slp::parse("E").take();
  global_context_.bindings["$CHANNEL_F"] = slp::parse("F").take();

  register_builtin_functions();
  logger_->info("[processor_c] Registered {} builtin functions",
                function_registry_.size());
}

void processor_c::consume_event(const events::event_s &event) {
  if (event.category != events::event_category_e::RUNTIME_EXECUTION_REQUEST) {
    logger_->debug("[processor_c] Ignoring event with category {} on topic {}",
                   static_cast<int>(event.category), event.topic_identifier);
    return;
  }

  logger_->debug("[processor_c] Received execution request event for topic {}",
                 event.topic_identifier);

  try {
    const auto &request = std::any_cast<execution_request_s>(event.payload);

    if (!request.session) {
      logger_->error("[processor_c] Received event with null session pointer");
      return;
    }

    logger_->info("[processor_c] Executing script for session {} request {}",
                  request.session->get_id(), request.request_id);

    auto result = execute_script(request.session, request.script_text,
                                 request.request_id);

    send_result_to_session(request.session, result);

  } catch (const std::bad_any_cast &e) {
    logger_->error("[processor_c] Failed to cast event payload: {}", e.what());
  } catch (const std::exception &e) {
    logger_->error("[processor_c] Exception during event processing: {}",
                   e.what());
  }
}

execution_result_s processor_c::execute_script(session_c *session,
                                               const std::string &script_text,
                                               const std::string &request_id) {
  execution_result_s result;
  result.request_id = request_id;
  result.success = false;

  auto parse_result = slp::parse(script_text);

  if (parse_result.is_error()) {
    result.error_message = parse_result.error().message;
    logger_->error("[processor_c] Parse error: {} at byte {}",
                   result.error_message, parse_result.error().byte_position);
    return result;
  }

  try {
    auto eval_result = eval_object(session, parse_result.object());
    if (eval_result.type() == slp::slp_type_e::ERROR) {
      result.error_message = eval_result.as_string().to_string();
      logger_->error("[processor_c] Script execution returned error: {}",
                     result.error_message);
    } else {
      result.result_data = slp_object_to_string(eval_result);
      result.success = true;
      logger_->debug("[processor_c] Script executed successfully");
    }
  } catch (const std::exception &e) {
    result.error_message = e.what();
    logger_->error("[processor_c] Execution error: {}", e.what());
  }

  return result;
}

slp::slp_object_c processor_c::eval_object(session_c *session,
                                           const slp::slp_object_c &obj) {
  eval_context_s empty_context;
  return eval_object_with_context(session, obj, empty_context);
}

slp::slp_object_c
processor_c::eval_object_with_context(session_c *session,
                                      const slp::slp_object_c &obj,
                                      const eval_context_s &context) {
  auto type = obj.type();

  if (type == slp::slp_type_e::INTEGER || type == slp::slp_type_e::REAL ||
      type == slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::from_data(obj.get_data(), obj.get_symbols(),
                                        obj.get_root_offset());
  }

  if (type == slp::slp_type_e::SYMBOL) {
    std::string sym = obj.as_symbol();

    auto it = context.bindings.find(sym);
    if (it != context.bindings.end()) {
      const auto &bound_obj = it->second;
      return slp::slp_object_c::from_data(bound_obj.get_data(),
                                          bound_obj.get_symbols(),
                                          bound_obj.get_root_offset());
    }

    auto global_it = global_context_.bindings.find(sym);
    if (global_it != global_context_.bindings.end()) {
      const auto &bound_obj = global_it->second;
      return slp::slp_object_c::from_data(bound_obj.get_data(),
                                          bound_obj.get_symbols(),
                                          bound_obj.get_root_offset());
    }

    return slp::slp_object_c::from_data(obj.get_data(), obj.get_symbols(),
                                        obj.get_root_offset());
  }

  if (type == slp::slp_type_e::PAREN_LIST) {
    auto list = obj.as_list();
    if (list.empty()) {
      return slp::parse("0").take();
    }
    auto first = list.at(0);
    if (first.type() == slp::slp_type_e::SYMBOL) {
      std::string function_name = first.as_symbol();
      return call_function(session, function_name, obj, context);
    }
    return slp::parse("0").take();
  }

  if (type == slp::slp_type_e::BRACKET_LIST) {
    auto list = obj.as_list();
    slp::slp_object_c last_result = slp::parse("0").take();
    for (size_t i = 0; i < list.size(); i++) {
      last_result = eval_object_with_context(session, list.at(i), context);
      if (last_result.type() == slp::slp_type_e::ERROR) {
        break;
      }
    }
    return last_result;
  }

  return slp::parse("0").take();
}

slp::slp_object_c processor_c::call_function(session_c *session,
                                             const std::string &name,
                                             const slp::slp_object_c &args,
                                             const eval_context_s &context) {
  auto it = function_registry_.find(name);
  if (it == function_registry_.end()) {
    logger_->warn("[processor_c] Unknown function: {}", name);
    return slp::parse("@\"unknown function '" + name + "'\"").take();
  }

  try {
    return it->second(session, args, context);
  } catch (const std::exception &e) {
    logger_->error("[processor_c] Function {} threw exception: {}", name,
                   e.what());
    return slp::parse("@\"" + std::string(e.what()) + "\"").take();
  }
}

void processor_c::send_result_to_session(session_c *session,
                                         const execution_result_s &result) {
  logger_->debug("[processor_c] Sending result to session {}",
                 session->get_id());
}

void processor_c::register_function(const std::string &name,
                                    function_handler_t handler) {
  function_registry_[name] = handler;
  logger_->debug("[processor_c] Registered function: {}", name);
}

void processor_c::register_builtin_functions() {
  fns::function_dependencies_s deps;
  deps.logger = logger_;
  deps.eval_fn = [this](session_c *session, const slp::slp_object_c &obj,
                       const eval_context_s &context) {
    return eval_object_with_context(session, obj, context);
  };
  deps.to_string_fn = [this](const slp::slp_object_c &obj) {
    return slp_object_to_string(obj);
  };
  deps.subscription_handlers = &subscription_handlers_;
  deps.subscription_handlers_mutex = &subscription_handlers_mutex_;
  deps.pending_awaits = &pending_awaits_;
  deps.pending_awaits_mutex = &pending_awaits_mutex_;
  deps.max_await_timeout = MAX_AWAIT_TIMEOUT;

  auto groups = fns::get_all_function_groups(deps);
  
  for (const auto &group : groups) {
    for (const auto &[name, handler] : group.functions) {
      register_function(name, handler);
      logger_->debug("[processor_c] Registered function: {} (group: {})", 
                     name, group.group_name);
    }
  }
}

} // namespace runtime
