#include "runtime/fns/event.hpp"
#include "runtime/fns/helpers.hpp"
#include "runtime/processor.hpp"
#include "runtime/session/session.hpp"
#include <algorithm>
#include <any>

namespace runtime::fns {

function_group_s get_event_functions(runtime_information_if &runtime_info) {
  function_group_s group;
  group.group_name = "core/event";

  group.functions["pub"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["pub"].parameters = {{"channel", slp::slp_type_e::SYMBOL, true},
                                       {"topic_id", slp::slp_type_e::INTEGER, false},
                                       {"data", slp::slp_type_e::NONE, true}};
  group.functions["pub"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 4) {
          return SLP_ERROR(
              "core/event/pub requires channel, topic-id and data (use "
              "$CHANNEL_A through $CHANNEL_F)");
        }

        std::map<std::string, slp::slp_object_c> channel_context;
        channel_context["$CHANNEL_A"] = slp::parse("A").take();
        channel_context["$CHANNEL_B"] = slp::parse("B").take();
        channel_context["$CHANNEL_C"] = slp::parse("C").take();
        channel_context["$CHANNEL_D"] = slp::parse("D").take();
        channel_context["$CHANNEL_E"] = slp::parse("E").take();
        channel_context["$CHANNEL_F"] = slp::parse("F").take();

        auto channel_obj =
            runtime_info.eval_object(session, list.at(1), channel_context);
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

        auto data_result = runtime_info.eval_object(session, data_obj, context);
        std::string data_str = runtime_info.object_to_string(data_result);

        publish_result_e result =
            session.publish_event(category, topic_id, data_str);

        switch (result) {
        case publish_result_e::OK:
          break;
        case publish_result_e::RATE_LIMIT_EXCEEDED:
          return SLP_ERROR("core/event/pub failed (rate limit exceeded)");
        case publish_result_e::PERMISSION_DENIED:
          return SLP_ERROR("core/event/pub failed (permission denied)");
        case publish_result_e::NO_ENTITY:
          return SLP_ERROR("core/event/pub failed (no entity)");
        case publish_result_e::NO_EVENT_SYSTEM:
          return SLP_ERROR("core/event/pub failed (no event system)");
        case publish_result_e::NO_PRODUCER:
          return SLP_ERROR("core/event/pub failed (no producer)");
        case publish_result_e::NO_TOPIC_WRITER:
          return SLP_ERROR("core/event/pub failed (no topic writer)");
        default:
          return SLP_ERROR("core/event/pub failed (unknown error)");
        }

        logger->debug("[event] pub channel {} topic {} data {}", channel_sym,
                      topic_id, data_str);
        return SLP_BOOL(true);
      };

  group.functions["sub"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["sub"].parameters = {
      {"channel", slp::slp_type_e::SYMBOL, true},
      {"topic_id", slp::slp_type_e::INTEGER, false},
      {"handler_body", slp::slp_type_e::BRACE_LIST, false}};
  group.functions["sub"].handler_context_vars = {
      {"$data", slp::slp_type_e::SOME}};
  group.functions["sub"]
      .function = [&runtime_info](
                      session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
    auto logger = runtime_info.get_logger();
    auto subscription_handlers = runtime_info.get_subscription_handlers();
    auto subscription_handlers_mutex =
        runtime_info.get_subscription_handlers_mutex();
    auto list = args.as_list();
    if (list.size() < 4) {
      return SLP_ERROR(
          "core/event/sub requires channel, topic-id and handler body "
          "(use $CHANNEL_A through $CHANNEL_F)");
    }

    std::map<std::string, slp::slp_object_c> channel_context;
    channel_context["$CHANNEL_A"] = slp::parse("A").take();
    channel_context["$CHANNEL_B"] = slp::parse("B").take();
    channel_context["$CHANNEL_C"] = slp::parse("C").take();
    channel_context["$CHANNEL_D"] = slp::parse("D").take();
    channel_context["$CHANNEL_E"] = slp::parse("E").take();
    channel_context["$CHANNEL_F"] = slp::parse("F").take();

    auto channel_obj =
        runtime_info.eval_object(session, list.at(1), channel_context);
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

    runtime_information_if::subscription_handler_s handler;
    handler.session = &session;
    handler.category = category;
    handler.topic_id = topic_id;
    handler.handler_data = handler_obj.get_data();
    handler.handler_symbols = handler_obj.get_symbols();
    handler.handler_root_offset = handler_obj.get_root_offset();

    {
      std::lock_guard<std::mutex> lock(*subscription_handlers_mutex);
      subscription_handlers->push_back(handler);
    }

    bool success = session.subscribe_to_topic(
        category, topic_id,
        [&runtime_info, &session, category,
         topic_id](const events::event_s &event) {
          auto logger = runtime_info.get_logger();
          auto subscription_handlers = runtime_info.get_subscription_handlers();
          auto subscription_handlers_mutex =
              runtime_info.get_subscription_handlers_mutex();
          std::lock_guard<std::mutex> lock(*subscription_handlers_mutex);
          for (const auto &h : *subscription_handlers) {
            if (h.session == &session && h.category == category &&
                h.topic_id == topic_id) {
              std::map<std::string, slp::slp_object_c> handler_context;

              std::string event_data;
              try {
                event_data = std::any_cast<std::string>(event.payload);
              } catch (...) {
                event_data = "<event data>";
              }
              handler_context["$data"] = SLP_STRING(event_data);

              auto handler_obj = slp::slp_object_c::from_data(
                  h.handler_data, h.handler_symbols, h.handler_root_offset);

              auto list = handler_obj.as_list();
              for (size_t i = 0; i < list.size(); i++) {
                auto result = runtime_info.eval_object(session, list.at(i),
                                                       handler_context);
                if (result.type() == slp::slp_type_e::ERROR) {
                  logger->debug("[event] Handler encountered error, "
                                "stopping execution");
                  break;
                }
              }
              break;
            }
          }
        });

    if (!success) {
      std::lock_guard<std::mutex> lock(*subscription_handlers_mutex);
      subscription_handlers->erase(
          std::remove_if(
              subscription_handlers->begin(), subscription_handlers->end(),
              [&](const runtime_information_if::subscription_handler_s &sh) {
                return sh.session == &session && sh.category == category &&
                       sh.topic_id == topic_id &&
                       sh.handler_data == handler.handler_data;
              }),
          subscription_handlers->end());
      return SLP_ERROR("core/event/sub failed (check permissions)");
    }

    logger->debug("[event] sub channel {} topic {} with handler", channel_sym,
                  topic_id);
    return SLP_BOOL(true);
  };

  return group;
}

} // namespace runtime::fns
