#include "runtime/fns/kv.hpp"
#include "runtime/fns/helpers.hpp"

namespace runtime::fns {

function_group_s get_kv_functions(
    logger_t logger,
    std::function<slp::slp_object_c(session_c*, const slp::slp_object_c&, const processor_c::eval_context_s&)> eval_fn,
    std::function<std::string(const slp::slp_object_c&)> to_string_fn
) {
    function_group_s group;
    group.group_name = "kv";

    group.functions["kv/set"] = [logger, eval_fn, to_string_fn](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
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

        auto value_result = eval_fn(session, value_obj, context);
        std::string value = to_string_fn(value_result);

        auto *store = session->get_store();
        if (!store) {
            return SLP_ERROR("session store not available");
        }

        bool success = store->set(key, value);
        if (!success) {
            return SLP_ERROR("kv/set failed (check permissions)");
        }

        logger->debug("[kv] set {} = {}", key, value);
        return SLP_BOOL(true);
    };

    group.functions["kv/get"] = [logger, to_string_fn](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
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

        logger->debug("[kv] get {} = {}", key, value);
        return SLP_STRING(value);
    };

    group.functions["kv/del"] = [logger](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
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

        logger->debug("[kv] del {}", key);
        return SLP_BOOL(true);
    };

    group.functions["kv/exists"] = [logger](
        session_c *session,
        const slp::slp_object_c &args,
        const processor_c::eval_context_s &context
    ) {
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
        logger->debug("[kv] exists {} = {}", key, exists);
        return SLP_BOOL(exists);
    };

    return group;
}

}

