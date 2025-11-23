#include "runtime/processor.hpp"
#include <any>
#include <spdlog/spdlog.h>
#include <sstream>

namespace runtime {

processor_c::processor_c(logger_t logger, events::event_system_c *event_system)
    : logger_(logger), event_system_(event_system), current_context_(nullptr) {
  logger_->info("[processor_c] Initializing processor");
  register_builtin_functions();
  logger_->info("[processor_c] Registered {} builtin functions",
                function_registry_.size());
}

void processor_c::consume_event(const events::event_s &event) {
  logger_->debug("[processor_c] Received event for topic {}",
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

    if (std::holds_alternative<std::int64_t>(eval_result)) {
      result.result_data = std::to_string(std::get<std::int64_t>(eval_result));
    } else if (std::holds_alternative<double>(eval_result)) {
      result.result_data = std::to_string(std::get<double>(eval_result));
    } else if (std::holds_alternative<std::string>(eval_result)) {
      result.result_data = std::get<std::string>(eval_result);
    } else if (std::holds_alternative<bool>(eval_result)) {
      result.result_data = std::get<bool>(eval_result) ? "true" : "false";
    }

    result.success = true;
    logger_->debug("[processor_c] Script executed successfully");
  } catch (const std::exception &e) {
    result.error_message = e.what();
    logger_->error("[processor_c] Execution error: {}", e.what());
  }

  return result;
}

function_result_t processor_c::eval_object(session_c *session,
                                           const slp::slp_object_c &obj) {
  if (current_context_) {
    return eval_object_with_context(session, obj, *current_context_);
  }
  eval_context_s empty_context;
  return eval_object_with_context(session, obj, empty_context);
}

function_result_t
processor_c::eval_object_with_context(session_c *session,
                                      const slp::slp_object_c &obj,
                                      const eval_context_s &context) {
  const eval_context_s *prev_context = current_context_;
  current_context_ = &context;

  auto type = obj.type();
  function_result_t result;

  if (type == slp::slp_type_e::INTEGER) {
    result = obj.as_int();
  } else if (type == slp::slp_type_e::REAL) {
    result = obj.as_real();
  } else if (type == slp::slp_type_e::SYMBOL) {
    std::string sym = obj.as_symbol();
    auto it = context.bindings.find(sym);
    if (it != context.bindings.end()) {
      result = it->second;
    } else {
      result = sym;
    }
  } else if (type == slp::slp_type_e::DQ_LIST) {
    result = obj.as_string().to_string();
  } else if (type == slp::slp_type_e::PAREN_LIST) {
    auto list = obj.as_list();
    if (list.empty()) {
      result = std::int64_t(0);
    } else {
      auto first = list.at(0);
      if (first.type() == slp::slp_type_e::SYMBOL) {
        std::string function_name = first.as_symbol();
        result = call_function(session, function_name, obj);
      } else {
        result = std::int64_t(0);
      }
    }
  } else if (type == slp::slp_type_e::BRACKET_LIST) {
    auto list = obj.as_list();
    function_result_t last_result = std::int64_t(0);
    for (size_t i = 0; i < list.size(); i++) {
      last_result = eval_object_with_context(session, list.at(i), context);
    }
    result = last_result;
  } else {
    result = std::int64_t(0);
  }

  current_context_ = prev_context;
  return result;
}

function_result_t processor_c::call_function(session_c *session,
                                             const std::string &name,
                                             const slp::slp_object_c &args) {
  auto it = function_registry_.find(name);
  if (it == function_registry_.end()) {
    logger_->warn("[processor_c] Unknown function: {}", name);
    return std::string("error: unknown function '" + name + "'");
  }

  try {
    return it->second(session, args);
  } catch (const std::exception &e) {
    logger_->error("[processor_c] Function {} threw exception: {}", name,
                   e.what());
    return std::string("error: " + std::string(e.what()));
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
  register_function(
      "kv/set", [this](session_c *session, const slp::slp_object_c &args) {
        auto list = args.as_list();
        if (list.size() < 3) {
          return function_result_t(
              std::string("error: kv/set requires key and value"));
        }

        auto key_obj = list.at(1);
        auto value_obj = list.at(2);

        std::string key;
        if (key_obj.type() == slp::slp_type_e::SYMBOL) {
          key = key_obj.as_symbol();
        } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
          key = key_obj.as_string().to_string();
        } else {
          return function_result_t(
              std::string("error: key must be symbol or string"));
        }

        auto value_result = eval_object(session, value_obj);
        std::string value;
        if (std::holds_alternative<std::int64_t>(value_result)) {
          value = std::to_string(std::get<std::int64_t>(value_result));
        } else if (std::holds_alternative<double>(value_result)) {
          value = std::to_string(std::get<double>(value_result));
        } else if (std::holds_alternative<std::string>(value_result)) {
          value = std::get<std::string>(value_result);
        } else if (std::holds_alternative<bool>(value_result)) {
          value = std::get<bool>(value_result) ? "true" : "false";
        }

        auto *store = session->get_store();
        if (!store) {
          return function_result_t(
              std::string("error: session store not available"));
        }

        bool success = store->set(key, value);
        if (!success) {
          return function_result_t(
              std::string("error: kv/set failed (check permissions)"));
        }

        logger_->debug("[processor_c] kv/set {} = {}", key, value);
        return function_result_t(true);
      });

  register_function("kv/get", [this](session_c *session,
                                     const slp::slp_object_c &args) {
    auto list = args.as_list();
    if (list.size() < 2) {
      return function_result_t(std::string("error: kv/get requires key"));
    }

    auto key_obj = list.at(1);
    std::string key;
    if (key_obj.type() == slp::slp_type_e::SYMBOL) {
      key = key_obj.as_symbol();
    } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
      key = key_obj.as_string().to_string();
    } else {
      return function_result_t(
          std::string("error: key must be symbol or string"));
    }

    auto *store = session->get_store();
    if (!store) {
      return function_result_t(
          std::string("error: session store not available"));
    }

    std::string value;
    bool success = store->get(key, value);
    if (!success) {
      return function_result_t(
          std::string("error: kv/get failed (key not found or no permission)"));
    }

    logger_->debug("[processor_c] kv/get {} = {}", key, value);
    return function_result_t(value);
  });

  register_function(
      "kv/del", [this](session_c *session, const slp::slp_object_c &args) {
        auto list = args.as_list();
        if (list.size() < 2) {
          return function_result_t(std::string("error: kv/del requires key"));
        }

        auto key_obj = list.at(1);
        std::string key;
        if (key_obj.type() == slp::slp_type_e::SYMBOL) {
          key = key_obj.as_symbol();
        } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
          key = key_obj.as_string().to_string();
        } else {
          return function_result_t(
              std::string("error: key must be symbol or string"));
        }

        auto *store = session->get_store();
        if (!store) {
          return function_result_t(
              std::string("error: session store not available"));
        }

        bool success = store->del(key);
        if (!success) {
          return function_result_t(
              std::string("error: kv/del failed (check permissions)"));
        }

        logger_->debug("[processor_c] kv/del {}", key);
        return function_result_t(true);
      });

  register_function("kv/exists", [this](session_c *session,
                                        const slp::slp_object_c &args) {
    auto list = args.as_list();
    if (list.size() < 2) {
      return function_result_t(std::string("error: kv/exists requires key"));
    }

    auto key_obj = list.at(1);
    std::string key;
    if (key_obj.type() == slp::slp_type_e::SYMBOL) {
      key = key_obj.as_symbol();
    } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
      key = key_obj.as_string().to_string();
    } else {
      return function_result_t(
          std::string("error: key must be symbol or string"));
    }

    auto *store = session->get_store();
    if (!store) {
      return function_result_t(
          std::string("error: session store not available"));
    }

    bool exists = store->exists(key);
    logger_->debug("[processor_c] kv/exists {} = {}", key, exists);
    return function_result_t(exists);
  });

  register_function("event/pub", [this](session_c *session,
                                        const slp::slp_object_c &args) {
    auto list = args.as_list();
    if (list.size() < 3) {
      return function_result_t(
          std::string("error: event/pub requires topic-id and data"));
    }

    auto topic_obj = list.at(1);
    auto data_obj = list.at(2);

    if (topic_obj.type() != slp::slp_type_e::INTEGER) {
      return function_result_t(std::string("error: topic-id must be integer"));
    }

    std::uint16_t topic_id = static_cast<std::uint16_t>(topic_obj.as_int());

    auto data_result = eval_object(session, data_obj);
    std::string data_str;
    if (std::holds_alternative<std::int64_t>(data_result)) {
      data_str = std::to_string(std::get<std::int64_t>(data_result));
    } else if (std::holds_alternative<double>(data_result)) {
      data_str = std::to_string(std::get<double>(data_result));
    } else if (std::holds_alternative<std::string>(data_result)) {
      data_str = std::get<std::string>(data_result);
    } else if (std::holds_alternative<bool>(data_result)) {
      data_str = std::get<bool>(data_result) ? "true" : "false";
    }

    bool success = session->publish_event(
        events::event_category_e::RUNTIME_BACKCHANNEL_A, topic_id, data_str);

    if (!success) {
      return function_result_t(
          std::string("error: event/pub failed (check permissions)"));
    }

    logger_->debug("[processor_c] event/pub topic {} data {}", topic_id,
                   data_str);
    return function_result_t(true);
  });

  register_function("event/sub", [this](session_c *session,
                                        const slp::slp_object_c &args) {
    auto list = args.as_list();
    if (list.size() < 3) {
      return function_result_t(
          std::string("error: event/sub requires topic-id and handler body"));
    }

    auto topic_obj = list.at(1);
    auto handler_obj = list.at(2);

    if (topic_obj.type() != slp::slp_type_e::INTEGER) {
      return function_result_t(std::string("error: topic-id must be integer"));
    }

    if (handler_obj.type() != slp::slp_type_e::BRACE_LIST) {
      return function_result_t(
          std::string("error: handler must be a brace list {}"));
    }

    std::uint16_t topic_id = static_cast<std::uint16_t>(topic_obj.as_int());

    subscription_handler_s handler;
    handler.session = session;
    handler.topic_id = topic_id;
    handler.handler_data = handler_obj.get_data();
    handler.handler_symbols = handler_obj.get_symbols();
    handler.handler_root_offset = handler_obj.get_root_offset();

    bool success = session->subscribe_to_topic(
        topic_id, [this, session, topic_id](const events::event_s &event) {
          for (const auto &h : subscription_handlers_) {
            if (h.session == session && h.topic_id == topic_id) {
              eval_context_s context;

              std::string event_data;
              try {
                event_data = std::any_cast<std::string>(event.payload);
              } catch (...) {
                event_data = "<event data>";
              }
              context.bindings["$data"] = function_result_t(event_data);

              auto handler_obj = slp::slp_object_c::from_data(
                  h.handler_data, h.handler_symbols, h.handler_root_offset);

              auto list = handler_obj.as_list();
              for (size_t i = 0; i < list.size(); i++) {
                eval_object_with_context(session, list.at(i), context);
              }
              break;
            }
          }
        });

    if (!success) {
      return function_result_t(
          std::string("error: event/sub failed (check permissions)"));
    }

    subscription_handlers_.push_back(handler);
    logger_->debug("[processor_c] event/sub topic {} with handler", topic_id);
    return function_result_t(true);
  });

  register_function(
      "runtime/log", [this](session_c *session, const slp::slp_object_c &args) {
        auto list = args.as_list();
        if (list.size() < 2) {
          return function_result_t(
              std::string("error: runtime/log requires message"));
        }

        std::stringstream ss;
        for (size_t i = 1; i < list.size(); i++) {
          auto msg_result = eval_object(session, list.at(i));
          if (i > 1) {
            ss << " ";
          }
          if (std::holds_alternative<std::int64_t>(msg_result)) {
            ss << std::get<std::int64_t>(msg_result);
          } else if (std::holds_alternative<double>(msg_result)) {
            ss << std::get<double>(msg_result);
          } else if (std::holds_alternative<std::string>(msg_result)) {
            ss << std::get<std::string>(msg_result);
          } else if (std::holds_alternative<bool>(msg_result)) {
            ss << (std::get<bool>(msg_result) ? "true" : "false");
          }
        }

        std::string message = ss.str();
        logger_->info("[session:{}] {}", session->get_id(), message);
        return function_result_t(true);
      });
}

} // namespace runtime
