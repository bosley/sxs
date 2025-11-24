#include "runtime/fns/runtime_fns.hpp"
#include "runtime/fns/helpers.hpp"
#include <any>
#include <sstream>

namespace runtime::fns {

function_group_s get_runtime_functions(
    logger_t logger,
    std::function<slp::slp_object_c(session_c*, const slp::slp_object_c&, const processor_c::eval_context_s&)> eval_fn,
    std::function<std::string(const slp::slp_object_c&)> to_string_fn,
    std::map<std::string, std::shared_ptr<processor_c::pending_await_s>> *pending_awaits,
    std::mutex *pending_awaits_mutex,
    std::chrono::seconds max_await_timeout
) {
    function_group_s group;
    group.group_name = "runtime";

    group.functions["log"] = [logger, eval_fn, to_string_fn](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
        auto list = args.as_list();
        if (list.size() < 2) {
            return SLP_ERROR("runtime/log requires message");
        }

        std::stringstream ss;
        for (size_t i = 1; i < list.size(); i++) {
            if (i > 1) {
                ss << " ";
            }
            auto msg_result = eval_fn(session, list.at(i), context);
            ss << to_string_fn(msg_result);
        }

        std::string message = ss.str();
        logger->info("[session:{}] {}", session->get_id(), message);
        return SLP_BOOL(true);
    };

    group.functions["eval"] = [eval_fn, to_string_fn](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
        auto list = args.as_list();
        if (list.size() < 2) {
            return SLP_ERROR("runtime/eval requires script text");
        }

        auto script_obj = eval_fn(session, list.at(1), context);
        std::string script_text = to_string_fn(script_obj);

        auto parse_result = slp::parse(script_text);
        if (parse_result.is_error()) {
            return SLP_ERROR("runtime/eval parse error");
        }

        return eval_fn(session, parse_result.object(), context);
    };

    group.functions["await"] = [eval_fn, pending_awaits, pending_awaits_mutex, max_await_timeout](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
        auto list = args.as_list();
        if (list.size() < 4) {
            return SLP_ERROR(
                "runtime/await requires body, response-channel and response-topic");
        }

        auto body_obj = list.at(1);
        auto resp_channel_obj = eval_fn(session, list.at(2), context);
        auto resp_topic_obj = list.at(3);

        if (resp_channel_obj.type() != slp::slp_type_e::SYMBOL) {
            return SLP_ERROR(
                "response channel must be $CHANNEL_A through $CHANNEL_F");
        }

        if (resp_topic_obj.type() != slp::slp_type_e::INTEGER) {
            return SLP_ERROR("response topic must be integer");
        }

        std::string channel_sym = resp_channel_obj.as_symbol();
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

        std::uint16_t topic_id =
            static_cast<std::uint16_t>(resp_topic_obj.as_int());

        std::string await_id =
            session->get_id() + "_" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count());

        auto pending = std::make_shared<processor_c::pending_await_s>();

        {
            std::lock_guard<std::mutex> lock(*pending_awaits_mutex);
            (*pending_awaits)[await_id] = pending;
        }

        bool sub_success = session->subscribe_to_topic(
            category, topic_id,
            [await_id, pending, pending_awaits_mutex](const events::event_s &event) {
                std::string event_data;
                try {
                    event_data = std::any_cast<std::string>(event.payload);
                } catch (...) {
                    event_data = "<event data>";
                }

                std::lock_guard<std::mutex> lock(pending->mutex);
                pending->result = SLP_STRING(event_data);
                pending->completed = true;
                pending->cv.notify_one();
            });

        if (!sub_success) {
            std::lock_guard<std::mutex> lock(*pending_awaits_mutex);
            pending_awaits->erase(await_id);
            return SLP_ERROR("runtime/await failed to subscribe");
        }

        auto body_result = eval_fn(session, body_obj, context);
        if (body_result.type() == slp::slp_type_e::ERROR) {
            session->unsubscribe_from_topic(category, topic_id);
            std::lock_guard<std::mutex> lock(*pending_awaits_mutex);
            pending_awaits->erase(await_id);
            return body_result;
        }

        {
            std::unique_lock<std::mutex> lock(pending->mutex);
            if (!pending->cv.wait_for(lock, max_await_timeout,
                                    [&pending] { return pending->completed; })) {
                session->unsubscribe_from_topic(category, topic_id);
                std::lock_guard<std::mutex> await_lock(*pending_awaits_mutex);
                pending_awaits->erase(await_id);
                return SLP_ERROR("runtime/await timeout waiting for response");
            }
        }

        session->unsubscribe_from_topic(category, topic_id);
        slp::slp_object_c result;
        {
            std::lock_guard<std::mutex> lock(pending->mutex);
            result = slp::slp_object_c::from_data(pending->result.get_data(),
                                                pending->result.get_symbols(),
                                                pending->result.get_root_offset());
        }

        {
            std::lock_guard<std::mutex> lock(*pending_awaits_mutex);
            pending_awaits->erase(await_id);
        }

        return result;
    };

    return group;
}

}

