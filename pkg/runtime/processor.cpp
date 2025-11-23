#include "runtime/processor.hpp"
#include <algorithm>
#include <any>
#include <spdlog/spdlog.h>
#include <sstream>

#define SLP_ERROR(msg) ([]() { \
  auto _r = slp::parse("@\"" msg "\""); \
  return _r.take(); \
}())

#define SLP_BOOL(val) ([](bool _v) { \
  auto _r = slp::parse(_v ? "true" : "false"); \
  return _r.take(); \
}(val))

#define SLP_STRING(val) ([](const std::string& _v) { \
  auto _r = slp::parse("\"" + _v + "\""); \
  return _r.take(); \
}(val))

namespace runtime {

std::string processor_c::slp_object_to_string(const slp::slp_object_c &obj) const {
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
    if (eval_result.type() == slp::slp_type_e::ERROR) {
      result.error_message = eval_result.as_string().to_string();
      logger_->error("[processor_c] Script execution returned error: {}", result.error_message);
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
    return slp::slp_object_c::from_data(
      obj.get_data(),
      obj.get_symbols(),
      obj.get_root_offset()
    );
  }
  
  if (type == slp::slp_type_e::SYMBOL) {
    std::string sym = obj.as_symbol();
    
    auto it = context.bindings.find(sym);
    if (it != context.bindings.end()) {
      const auto& bound_obj = it->second;
      return slp::slp_object_c::from_data(
        bound_obj.get_data(),
        bound_obj.get_symbols(),
        bound_obj.get_root_offset()
      );
    }
    
    auto global_it = global_context_.bindings.find(sym);
    if (global_it != global_context_.bindings.end()) {
      const auto& bound_obj = global_it->second;
      return slp::slp_object_c::from_data(
        bound_obj.get_data(),
        bound_obj.get_symbols(),
        bound_obj.get_root_offset()
      );
    }
    
    return slp::slp_object_c::from_data(
      obj.get_data(),
      obj.get_symbols(),
      obj.get_root_offset()
    );
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
  register_function(
      "kv/set", [this](session_c *session, const slp::slp_object_c &args,
                       const eval_context_s &context) {
        auto list = args.as_list();
        if (list.size() < 3) {
          return SLP_ERROR("kv/set requires key and value");
        }

        auto key_obj = list.at(1);
        auto value_obj = list.at(2);

        std::string key;
        if (key_obj.type() == slp::slp_type_e::SYMBOL) {
          key = key_obj.as_symbol();
        } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
          key = key_obj.as_string().to_string();
        } else {
          return SLP_ERROR("key must be symbol or string");
        }

        auto value_result = eval_object_with_context(session, value_obj, context);
        std::string value = slp_object_to_string(value_result);

        auto *store = session->get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool success = store->set(key, value);
        if (!success) {
          return SLP_ERROR("kv/set failed (check permissions)");
        }

        logger_->debug("[processor_c] kv/set {} = {}", key, value);
        return SLP_BOOL(true);
      });

  register_function("kv/get", [this](session_c *session,
                                     const slp::slp_object_c &args,
                                     const eval_context_s &context) {
    auto list = args.as_list();
    if (list.size() < 2) {
      return SLP_ERROR("kv/get requires key");
    }

    auto key_obj = list.at(1);
    std::string key;
    if (key_obj.type() == slp::slp_type_e::SYMBOL) {
      key = key_obj.as_symbol();
    } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
      key = key_obj.as_string().to_string();
    } else {
      return SLP_ERROR("key must be symbol or string");
    }

    auto *store = session->get_store();
    if (!store) {
      return SLP_ERROR("session store not available");
    }

    std::string value;
    bool success = store->get(key, value);
    if (!success) {
      return SLP_ERROR("kv/get failed (key not found or no permission)");
    }

    logger_->debug("[processor_c] kv/get {} = {}", key, value);
    return SLP_STRING(value);
  });

  register_function(
      "kv/del", [this](session_c *session, const slp::slp_object_c &args,
                       const eval_context_s &context) {
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("kv/del requires key");
        }

        auto key_obj = list.at(1);
        std::string key;
        if (key_obj.type() == slp::slp_type_e::SYMBOL) {
          key = key_obj.as_symbol();
        } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
          key = key_obj.as_string().to_string();
        } else {
          return SLP_ERROR("key must be symbol or string");
        }

        auto *store = session->get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool success = store->del(key);
        if (!success) {
          return SLP_ERROR("kv/del failed (check permissions)");
        }

        logger_->debug("[processor_c] kv/del {}", key);
        return SLP_BOOL(true);
      });

  register_function("kv/exists", [this](session_c *session,
                                        const slp::slp_object_c &args,
                                        const eval_context_s &context) {
    auto list = args.as_list();
    if (list.size() < 2) {
      return SLP_ERROR("kv/exists requires key");
    }

    auto key_obj = list.at(1);
    std::string key;
    if (key_obj.type() == slp::slp_type_e::SYMBOL) {
      key = key_obj.as_symbol();
    } else if (key_obj.type() == slp::slp_type_e::DQ_LIST) {
      key = key_obj.as_string().to_string();
    } else {
      return SLP_ERROR("key must be symbol or string");
    }

    auto *store = session->get_store();
    if (!store) {
      return SLP_ERROR("session store not available");
    }

    bool exists = store->exists(key);
    logger_->debug("[processor_c] kv/exists {} = {}", key, exists);
    return SLP_BOOL(exists);
  });

  register_function("event/pub", [this](session_c *session,
                                        const slp::slp_object_c &args,
                                        const eval_context_s &context) {
    auto list = args.as_list();
    if (list.size() < 4) {
      return SLP_ERROR("event/pub requires channel, topic-id and data (use $CHANNEL_A through $CHANNEL_F)");
    }

    // Evaluate the channel argument to resolve symbols like $CHANNEL_A
    auto channel_obj = eval_object_with_context(session, list.at(1), context);
    auto topic_obj = list.at(2);
    auto data_obj = list.at(3);

    if (channel_obj.type() != slp::slp_type_e::SYMBOL) {
      return SLP_ERROR("channel must be $CHANNEL_A through $CHANNEL_F");
    }

    std::string channel_sym = channel_obj.as_symbol();
    events::event_category_e category;
    
    if (channel_sym == "A") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_A;
    } else if (channel_sym == "B") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_B;
    } else if (channel_sym == "C") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_C;
    } else if (channel_sym == "D") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_D;
    } else if (channel_sym == "E") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_E;
    } else if (channel_sym == "F") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_F;
    } else {
      return SLP_ERROR("invalid channel (must be A, B, C, D, E, or F)");
    }

    if (topic_obj.type() != slp::slp_type_e::INTEGER) {
      return SLP_ERROR("topic-id must be integer");
    }

    std::uint16_t topic_id = static_cast<std::uint16_t>(topic_obj.as_int());

    auto data_result = eval_object_with_context(session, data_obj, context);
    std::string data_str = slp_object_to_string(data_result);

    bool success = session->publish_event(category, topic_id, data_str);

    if (!success) {
      return SLP_ERROR("event/pub failed (check permissions)");
    }

    logger_->debug("[processor_c] event/pub channel {} topic {} data {}", 
                   channel_sym, topic_id, data_str);
    return SLP_BOOL(true);
  });

  register_function("event/sub", [this](session_c *session,
                                        const slp::slp_object_c &args,
                                        const eval_context_s &context) {
    auto list = args.as_list();
    if (list.size() < 4) {
      return SLP_ERROR("event/sub requires channel, topic-id and handler body (use $CHANNEL_A through $CHANNEL_F)");
    }

    // Evaluate the channel argument to resolve symbols like $CHANNEL_A
    auto channel_obj = eval_object_with_context(session, list.at(1), context);
    auto topic_obj = list.at(2);
    auto handler_obj = list.at(3);

    if (channel_obj.type() != slp::slp_type_e::SYMBOL) {
      return SLP_ERROR("channel must be $CHANNEL_A through $CHANNEL_F");
    }

    std::string channel_sym = channel_obj.as_symbol();
    events::event_category_e category;
    
    if (channel_sym == "A") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_A;
    } else if (channel_sym == "B") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_B;
    } else if (channel_sym == "C") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_C;
    } else if (channel_sym == "D") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_D;
    } else if (channel_sym == "E") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_E;
    } else if (channel_sym == "F") {
      category = events::event_category_e::RUNTIME_BACKCHANNEL_F;
    } else {
      return SLP_ERROR("invalid channel (must be A, B, C, D, E, or F)");
    }

    if (topic_obj.type() != slp::slp_type_e::INTEGER) {
      return SLP_ERROR("topic-id must be integer");
    }

    if (handler_obj.type() != slp::slp_type_e::BRACE_LIST) {
      return SLP_ERROR("handler must be a brace list {}");
    }

    std::uint16_t topic_id = static_cast<std::uint16_t>(topic_obj.as_int());

    subscription_handler_s handler;
    handler.session = session;
    handler.category = category;
    handler.topic_id = topic_id;
    handler.handler_data = handler_obj.get_data();
    handler.handler_symbols = handler_obj.get_symbols();
    handler.handler_root_offset = handler_obj.get_root_offset();

    {
      std::lock_guard<std::mutex> lock(subscription_handlers_mutex_);
      subscription_handlers_.push_back(handler);
    }

    bool success = session->subscribe_to_topic(
        category, topic_id, [this, session, category, topic_id](const events::event_s &event) {
          std::lock_guard<std::mutex> lock(subscription_handlers_mutex_);
          for (const auto &h : subscription_handlers_) {
            if (h.session == session && h.category == category && h.topic_id == topic_id) {
              eval_context_s context;

              std::string event_data;
              try {
                event_data = std::any_cast<std::string>(event.payload);
              } catch (...) {
                event_data = "<event data>";
              }
              context.bindings["$data"] = SLP_STRING(event_data);

              auto handler_obj = slp::slp_object_c::from_data(
                  h.handler_data, h.handler_symbols, h.handler_root_offset);

              auto list = handler_obj.as_list();
              for (size_t i = 0; i < list.size(); i++) {
                auto result = eval_object_with_context(session, list.at(i), context);
                if (result.type() == slp::slp_type_e::ERROR) {
                  logger_->debug("[processor_c] Handler encountered error, stopping execution");
                  break;
                }
              }
              break;
            }
          }
        });

    if (!success) {
      std::lock_guard<std::mutex> lock(subscription_handlers_mutex_);
      subscription_handlers_.erase(
          std::remove_if(subscription_handlers_.begin(),
                         subscription_handlers_.end(),
                         [&](const subscription_handler_s &sh) {
                           return sh.session == session && sh.category == category && 
                                  sh.topic_id == topic_id &&
                                  sh.handler_data == handler.handler_data;
                         }),
          subscription_handlers_.end());
      return SLP_ERROR("event/sub failed (check permissions)");
    }

    logger_->debug("[processor_c] event/sub channel {} topic {} with handler", 
                   channel_sym, topic_id);
    return SLP_BOOL(true);
  });

  register_function(
      "runtime/log", [this](session_c *session, const slp::slp_object_c &args,
                            const eval_context_s &context) {
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("runtime/log requires message");
        }

        std::stringstream ss;
        for (size_t i = 1; i < list.size(); i++) {
          if (i > 1) {
            ss << " ";
          }
          auto msg_result = eval_object_with_context(session, list.at(i), context);
          ss << slp_object_to_string(msg_result);
        }

        std::string message = ss.str();
        logger_->info("[session:{}] {}", session->get_id(), message);
        return SLP_BOOL(true);
      });
}

} // namespace runtime
