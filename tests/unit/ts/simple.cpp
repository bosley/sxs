#include "runtime/fns/fns.hpp"
#include "runtime/processor.hpp"
#include "runtime/runtime.hpp"
#include "ts/ts.hpp"
#include <snitch/snitch.hpp>

using namespace pkg::ts;
using namespace runtime;
using namespace runtime::fns;

class mock_runtime_info_c : public runtime_information_if {
public:
  slp::slp_object_c
  eval_object(session_c &, const slp::slp_object_c &,
              const std::map<std::string, slp::slp_object_c> &) override {
    return slp::parse("nil").take();
  }
  std::string object_to_string(const slp::slp_object_c &) override {
    return "";
  }
  logger_t get_logger() override { return nullptr; }
  std::vector<subscription_handler_s> *get_subscription_handlers() override {
    return nullptr;
  }
  std::mutex *get_subscription_handlers_mutex() override { return nullptr; }
};

std::map<std::string, function_signature_s>
build_type_signatures(const std::vector<function_group_s> &groups) {
  std::map<std::string, function_signature_s> signatures;

  for (const auto &group : groups) {
    for (const auto &[name, info] : group.functions) {
      std::string full_name = std::string(group.group_name) + "/" + name;

      function_signature_s sig;
      sig.return_type = info.return_type;
      sig.can_return_error = info.can_return_error;
      sig.is_variadic = info.is_variadic;
      sig.handler_context_vars = info.handler_context_vars;

      for (const auto &param : info.parameters) {
        function_parameter_info_s param_info;
        param_info.type = param.type;
        param_info.is_evaluated = param.is_evaluated;
        sig.parameters.push_back(param_info);
      }

      if (full_name == "core/kv/set") {
        sig.is_setter = true;
      } else if (full_name == "core/kv/snx") {
        sig.is_setter = true;
      } else if (full_name == "core/kv/get") {
        sig.is_getter = true;
      } else if (full_name == "core/kv/load") {
        sig.is_loader = true;
      }

      signatures[full_name] = sig;
    }
  }

  return signatures;
}

std::map<std::string, slp::slp_type_e> extract_dollar_vars_from_signatures(
    const std::map<std::string, function_signature_s> &signatures) {
  std::map<std::string, slp::slp_type_e> dollar_vars;

  for (const auto &[fn_name, sig] : signatures) {
    for (const auto &[var_name, var_type] : sig.handler_context_vars) {
      dollar_vars[var_name] = var_type;
    }
  }

  dollar_vars["$CHANNEL_A"] = slp::slp_type_e::SYMBOL;
  dollar_vars["$CHANNEL_B"] = slp::slp_type_e::SYMBOL;
  dollar_vars["$CHANNEL_C"] = slp::slp_type_e::SYMBOL;
  dollar_vars["$CHANNEL_D"] = slp::slp_type_e::SYMBOL;
  dollar_vars["$CHANNEL_E"] = slp::slp_type_e::SYMBOL;
  dollar_vars["$CHANNEL_F"] = slp::slp_type_e::SYMBOL;

  return dollar_vars;
}

TEST_CASE("type system - simple set and get", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result =
      slp::parse("((core/kv/set x 42) (core/insist (core/kv/get x)))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - get before set fails", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/kv/get x))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - tainted value cannot be stored", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result =
      slp::parse("((core/kv/set x 42) (core/kv/set y (core/kv/get x)))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - detaint allows storing", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(
      "((core/kv/set x 42) (core/kv/set y (core/insist (core/kv/get x))))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - snx sets if not exists", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(
      "((core/kv/snx counter 0) (core/insist (core/kv/get counter)))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - multiple variables", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(R"(
    (
      (core/kv/set name "Alice")
      (core/kv/set age 30)
      (core/kv/set active true)
      (core/util/log (core/insist (core/kv/get name)))
      (core/util/log (core/insist (core/kv/get age)))
      (core/util/log (core/insist (core/kv/get active)))
    )
  )");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - get wrong variable fails", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(R"(
    (
      (core/kv/set name "Alice")
      (core/util/log (core/insist (core/kv/get age)))
    )
  )");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - string type tracking", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(
      R"(((core/kv/set msg "hello") (core/insist (core/kv/get msg))))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - detaint requires tainted input", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/insist 42))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - get rejects dollar vars", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/kv/get $key))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - load requires dollar vars", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/kv/set x 42) (core/kv/load x))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - load returns pure SOME", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/kv/load $key))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - iterate with load", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(R"(
    (
      (core/kv/set user:1 "alice")
      (core/kv/iterate user: 0 10 {
        (core/util/log (core/kv/load $key))
      })
    )
  )");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - event sub with $data", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(R"(
    (core/event/sub $CHANNEL_A 100 :str {
      (core/util/log $data)
    })
  )");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - channel vars in pub", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/event/pub $CHANNEL_A 100 \"msg\"))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - insist rejects symbol", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("(((core/kv/set x 42) (core/insist x)))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - insist rejects literal integer", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse("((core/insist 42))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - insist rejects string", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);
  auto dollar_vars = extract_dollar_vars_from_signatures(signatures);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::PAREN_LIST, false});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures, dollar_vars);

  auto parse_result = slp::parse(R"(((core/insist "hello")))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}
