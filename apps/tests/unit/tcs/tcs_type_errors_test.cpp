#include <core/tcs/tcs.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

namespace {

pkg::core::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

} // namespace

TEST_CASE("tcs type errors - undefined symbol", "[unit][tcs][errors][symbol]") {
  std::string source = R"([
    (def x undefined-symbol)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - invalid type annotation",
          "[unit][tcs][errors][type]") {
  std::string source = R"([
    (def func (fn (x :invalid-type) :int [
      (def result 42)
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - function wrong argument count",
          "[unit][tcs][errors][args]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - redefinition in same scope",
          "[unit][tcs][errors][redef]") {
  std::string source = R"([
    (def x 1)
    (def x 2)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - if branches different types",
          "[unit][tcs][errors][if]") {
  std::string source = R"([
    (def result (if 1 42 "string"))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - function body type mismatch",
          "[unit][tcs][errors][function]") {
  std::string source = R"([
    (def func (fn (x :int) :int [
      "string"
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - unknown command", "[unit][tcs][errors][command]") {
  std::string source = R"([
    (unknown-command arg1 arg2)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - apply with non-lambda",
          "[unit][tcs][errors][apply]") {
  std::string source = R"([
    (def result (apply 42 {1 2 3}))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs type errors - apply with non-brace-list",
          "[unit][tcs][errors][apply]") {
  std::string source = R"([
    (def func (fn (x :int) :int [
      42
    ]))
    (def result (apply func (1 2 3)))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}
