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
  slp::slp_object_c eval_object(session_c &, const slp::slp_object_c &,
                                const std::map<std::string, slp::slp_object_c> &) override {
    return slp::parse("nil").take();
  }
  std::string object_to_string(const slp::slp_object_c &) override { return ""; }
  logger_t get_logger() override { return nullptr; }
  std::vector<subscription_handler_s> *get_subscription_handlers() override { return nullptr; }
  std::mutex *get_subscription_handlers_mutex() override { return nullptr; }
  std::map<std::string, std::shared_ptr<pending_await_s>> *get_pending_awaits() override { return nullptr; }
  std::mutex *get_pending_awaits_mutex() override { return nullptr; }
  std::chrono::seconds get_max_await_timeout() override { return std::chrono::seconds(0); }
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
      }

      signatures[full_name] = sig;
    }
  }

  return signatures;
}

TEST_CASE("type system - simple set and get", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

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

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

  auto parse_result = slp::parse("((core/kv/get x))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - tainted value cannot be stored", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

  auto parse_result = slp::parse(
      "((core/kv/set x 42) (core/kv/set y (core/kv/get x)))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}

TEST_CASE("type system - detaint allows storing", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

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

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

  auto parse_result =
      slp::parse("((core/kv/snx counter 0) (core/insist (core/kv/get counter)))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - multiple variables", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

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

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

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

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

  auto parse_result =
      slp::parse(R"(((core/kv/set msg "hello") (core/insist (core/kv/get msg))))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE(result.success);
}

TEST_CASE("type system - detaint requires tainted input", "[ts]") {
  mock_runtime_info_c mock;
  auto groups = get_all_function_groups(mock);
  auto signatures = build_type_signatures(groups);

  function_signature_s insist_sig;
  insist_sig.return_type = slp::slp_type_e::NONE;
  insist_sig.can_return_error = false;
  insist_sig.is_detainter = true;
  insist_sig.parameters.push_back({slp::slp_type_e::NONE, true});
  signatures["core/insist"] = insist_sig;

  type_checker_c checker(signatures);

  auto parse_result = slp::parse("((core/insist 42))");
  REQUIRE_FALSE(parse_result.is_error());

  auto result = checker.check(parse_result.object());
  REQUIRE_FALSE(result.success);
}
