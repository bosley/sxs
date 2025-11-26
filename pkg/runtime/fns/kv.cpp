#include "runtime/fns/kv.hpp"
#include "runtime/fns/helpers.hpp"
#include "runtime/processor.hpp"
#include "runtime/session/session.hpp"

namespace runtime::fns {

static std::string object_to_storage_string(const slp::slp_object_c &obj) {
  switch (obj.type()) {
  case slp::slp_type_e::SYMBOL:
    return obj.as_symbol();
  case slp::slp_type_e::INTEGER:
    return std::to_string(obj.as_int());
  case slp::slp_type_e::REAL:
    return std::to_string(obj.as_real());
  case slp::slp_type_e::DATUM:
    [[fallthrough]];
  case slp::slp_type_e::ABERRANT:
    [[fallthrough]];
  case slp::slp_type_e::DQ_LIST:
    [[fallthrough]];
  case slp::slp_type_e::ERROR:
    [[fallthrough]];
  case slp::slp_type_e::NONE:
    [[fallthrough]];
  case slp::slp_type_e::SOME: {
    return obj.as_string().to_string();
  default:
    return slp::parse("@(unknown object type when attempting to store)")
        .take()
        .as_string()
        .to_string();
  }
  } // EOS
} // EOF

function_group_s get_kv_functions(runtime_information_if &runtime_info) {
  auto logger = runtime_info.get_logger();

  function_group_s group;
  group.group_name = "core/kv";

  group.functions["set"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["set"].parameters = {{"key", slp::slp_type_e::SYMBOL, false},
                                       {"value", slp::slp_type_e::NONE, true}};
  group.functions["set"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 3) {
          return SLP_ERROR("core/kv/set requires key and value");
        }

        auto key_obj = list.at(1);
        auto value_obj = list.at(2);

        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key = key_obj.as_symbol();

        auto value_result =
            runtime_info.eval_object(session, value_obj, context);
        std::string value = object_to_storage_string(value_result);

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool success = store->set(key, value);
        if (!success) {
          return SLP_ERROR("core/kv/set failed (check permissions)");
        }

        logger->debug("[kv] set {} = {}", key, value);
        return SLP_BOOL(true);
      };

  group.functions["get"].return_type = slp::slp_type_e::DQ_LIST;
  group.functions["get"].parameters = {{"key", slp::slp_type_e::SYMBOL, false}};
  group.functions["get"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/kv/get requires key");
        }

        auto key_obj = list.at(1);
        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key = key_obj.as_symbol();

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        std::string value;
        bool success = store->get(key, value);
        if (!success) {
          return SLP_ERROR(
              "core/kv/get failed (key not found or no permission)");
        }

        logger->debug("[kv] get {} = {}", key, value);
        return SLP_STRING(value);
      };

  group.functions["del"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["del"].parameters = {{"key", slp::slp_type_e::SYMBOL, false}};
  group.functions["del"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/kv/del requires key");
        }

        auto key_obj = list.at(1);
        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key_symbol = key_obj.as_symbol();

        std::string key;
        if (!key_symbol.empty() && key_symbol[0] == '$') {
          auto context_it = context.find(key_symbol);
          if (context_it == context.end()) {
            return SLP_ERROR("context variable not available");
          }
          key = object_to_storage_string(context_it->second);
        } else {
          key = key_symbol;
        }

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool success = store->del(key);
        if (!success) {
          return SLP_ERROR("core/kv/del failed (check permissions)");
        }

        logger->debug("[kv] del {}", key);
        return SLP_BOOL(true);
      };

  group.functions["exists"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["exists"].parameters = {
      {"key", slp::slp_type_e::SYMBOL, false}};
  group.functions["exists"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/kv/exists requires key");
        }

        auto key_obj = list.at(1);
        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key_symbol = key_obj.as_symbol();

        std::string key;
        if (!key_symbol.empty() && key_symbol[0] == '$') {
          auto context_it = context.find(key_symbol);
          if (context_it == context.end()) {
            return SLP_ERROR("context variable not available");
          }
          key = object_to_storage_string(context_it->second);
        } else {
          key = key_symbol;
        }

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool exists = store->exists(key);
        logger->debug("[kv] exists {} = {}", key, exists);
        return SLP_BOOL(exists);
      };

  group.functions["snx"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["snx"].parameters = {{"key", slp::slp_type_e::SYMBOL, false},
                                       {"value", slp::slp_type_e::NONE, true}};
  group.functions["snx"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 3) {
          return SLP_ERROR("core/kv/snx requires key and value");
        }

        auto key_obj = list.at(1);
        auto value_obj = list.at(2);

        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key = key_obj.as_symbol();

        auto value_result =
            runtime_info.eval_object(session, value_obj, context);
        std::string value = object_to_storage_string(value_result);

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool success = store->set_nx(key, value);
        if (!success) {
          logger->debug("[kv] snx {} failed (key exists)", key);
          return SLP_BOOL(false);
        }

        logger->debug("[kv] snx {} = {}", key, value);
        return SLP_BOOL(true);
      };

  group.functions["cas"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["cas"].parameters = {
      {"key", slp::slp_type_e::SYMBOL, false},
      {"expected_value", slp::slp_type_e::NONE, true},
      {"new_value", slp::slp_type_e::NONE, true}};
  group.functions["cas"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 4) {
          return SLP_ERROR("core/kv/cas requires key, expected value, and new "
                           "value");
        }

        auto key_obj = list.at(1);
        auto expected_obj = list.at(2);
        auto new_value_obj = list.at(3);

        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key = key_obj.as_symbol();

        auto expected_result =
            runtime_info.eval_object(session, expected_obj, context);
        std::string expected_value = object_to_storage_string(expected_result);

        auto new_value_result =
            runtime_info.eval_object(session, new_value_obj, context);
        std::string new_value = object_to_storage_string(new_value_result);

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        bool success = store->compare_and_swap(key, expected_value, new_value);
        if (!success) {
          logger->debug("[kv] cas {} failed (expected: {}, new: {})", key,
                        expected_value, new_value);
          return SLP_BOOL(false);
        }

        logger->debug("[kv] cas {} from {} to {}", key, expected_value,
                      new_value);
        return SLP_BOOL(true);
      };

  group.functions["iterate"].return_type = slp::slp_type_e::SYMBOL;
  group.functions["iterate"].parameters = {
      {"prefix", slp::slp_type_e::SYMBOL, false},
      {"offset", slp::slp_type_e::INTEGER, false},
      {"limit", slp::slp_type_e::INTEGER, false},
      {"handler_body", slp::slp_type_e::BRACE_LIST, false}};
  group.functions["iterate"].handler_context_vars = {
      {"$key", slp::slp_type_e::DQ_LIST}};
  group.functions["iterate"]
      .function = [&runtime_info](
                      session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
    auto logger = runtime_info.get_logger();
    auto list = args.as_list();
    if (list.size() < 5) {
      return SLP_ERROR("core/kv/iterate requires prefix, offset, limit, and "
                       "handler body");
    }

    auto prefix_obj = list.at(1);
    auto offset_obj = list.at(2);
    auto limit_obj = list.at(3);
    auto handler_obj = list.at(4);

    if (prefix_obj.type() != slp::slp_type_e::SYMBOL) {
      return SLP_ERROR("prefix must be symbol");
    }
    std::string prefix = prefix_obj.as_symbol();

    if (offset_obj.type() != slp::slp_type_e::INTEGER) {
      return SLP_ERROR("offset must be integer");
    }

    if (limit_obj.type() != slp::slp_type_e::INTEGER) {
      return SLP_ERROR("limit must be integer");
    }

    if (handler_obj.type() != slp::slp_type_e::BRACE_LIST) {
      return SLP_ERROR("handler must be a brace list {}");
    }

    std::int64_t offset = offset_obj.as_int();
    std::int64_t limit = limit_obj.as_int();

    if (offset < 0) {
      return SLP_ERROR("offset must be non-negative");
    }

    if (limit < 0) {
      return SLP_ERROR("limit must be non-negative");
    }

    auto *store = session.get_store();
    if (!store) {
      return SLP_ERROR("session store not available");
    }

    std::int64_t current_index = 0;
    std::int64_t processed_count = 0;

    store->iterate(
        prefix, [&](const std::string &key, const std::string &value) -> bool {
          if (current_index < offset) {
            current_index++;
            return true;
          }

          if (processed_count >= limit) {
            return false;
          }

          std::map<std::string, slp::slp_object_c> handler_context;
          for (const auto &[k, v] : context) {
            handler_context[k] = slp::slp_object_c::from_data(
                v.get_data(), v.get_symbols(), v.get_root_offset());
          }
          handler_context["$key"] = SLP_STRING(key);

          auto handler_list = handler_obj.as_list();
          for (size_t i = 0; i < handler_list.size(); i++) {
            auto result = runtime_info.eval_object(session, handler_list.at(i),
                                                   handler_context);
            if (result.type() == slp::slp_type_e::ERROR) {
              logger->debug("[kv] iterate handler encountered error, stopping");
              return false;
            }
          }

          current_index++;
          processed_count++;
          return true;
        });

    logger->debug("[kv] iterate prefix {} offset {} limit {} processed {}",
                  prefix, offset, limit, processed_count);
    return SLP_BOOL(true);
  };

  group.functions["load"].return_type = slp::slp_type_e::SOME;
  group.functions["load"].parameters = {
      {"key", slp::slp_type_e::SYMBOL, false}};
  group.functions["load"].can_return_error = false;
  group.functions["load"].function =
      [&runtime_info](session_c &session, const slp::slp_object_c &args,
                      const std::map<std::string, slp::slp_object_c> &context) {
        auto logger = runtime_info.get_logger();
        auto list = args.as_list();
        if (list.size() < 2) {
          return SLP_ERROR("core/kv/load requires key");
        }

        auto key_obj = list.at(1);
        if (key_obj.type() != slp::slp_type_e::SYMBOL) {
          return SLP_ERROR("key must be a symbol");
        }
        std::string key_symbol = key_obj.as_symbol();

        if (key_symbol != "$key") {
          return SLP_ERROR("core/kv/load requires $key context variable");
        }

        auto context_it = context.find("$key");
        if (context_it == context.end()) {
          return SLP_ERROR("$key not available in context");
        }

        std::string key = object_to_storage_string(context_it->second);

        auto *store = session.get_store();
        if (!store) {
          return SLP_ERROR("session store not available");
        }

        std::string value;
        bool success = store->get(key, value);
        if (!success) {
          return SLP_ERROR(
              "core/kv/load failed (key not found or no permission)");
        }

        logger->debug("[kv] load {} = {}", key, value);
        auto result = slp::parse("'" + value);
        if (result.is_error()) {
          return SLP_ERROR("core/kv/load failed to quote value");
        }
        return result.take();
      };

  return group;
}

} // namespace runtime::fns
