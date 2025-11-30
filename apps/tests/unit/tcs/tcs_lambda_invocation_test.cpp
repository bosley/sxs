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

TEST_CASE("tcs lambda invocation - call lambda with correct types",
          "[unit][tcs][lambda][invocation]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (add 1 2)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with wrong type for first arg",
          "[unit][tcs][lambda][invocation][error]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (add "string" 2)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with wrong type for second arg",
          "[unit][tcs][lambda][invocation][error]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (add 1 "string")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with too few arguments",
          "[unit][tcs][lambda][invocation][arity-error]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (add 1)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with too many arguments",
          "[unit][tcs][lambda][invocation][arity-error]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (add 1 2 3)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with no parameters",
          "[unit][tcs][lambda][invocation][no-params]") {
  std::string source = R"([
    (def get-value (fn () :int [
      42
    ]))
    (get-value)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with real parameters",
          "[unit][tcs][lambda][invocation][real]") {
  std::string source = R"([
    (def calculate (fn (x :real y :real) :real [
      3.14
    ]))
    (calculate 1.5 2.5)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with string parameter",
          "[unit][tcs][lambda][invocation][string]") {
  std::string source = R"([
    (def greet (fn (name :str) :str [
      "hello"
    ]))
    (greet "world")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - call lambda with mixed types",
          "[unit][tcs][lambda][invocation][mixed]") {
  std::string source = R"([
    (def mixed (fn (i :int r :real s :str) :int [
      42
    ]))
    (mixed 10 3.14 "test")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - multiple lambda calls",
          "[unit][tcs][lambda][invocation][multiple]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def multiply (fn (x :int y :int) :int [
      10
    ]))
    (add 1 2)
    (multiply 3 4)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - nested lambda calls",
          "[unit][tcs][lambda][invocation][nested]") {
  std::string source = R"([
    (def inner (fn (x :int) :int [
      42
    ]))
    (def outer (fn (y :int) :int [
      (inner y)
    ]))
    (outer 10)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs lambda invocation - lambda call with expression argument",
          "[unit][tcs][lambda][invocation][expression]") {
  std::string source = R"([
    (def process (fn (value :int) :int [
      42
    ]))
    (def get-number (fn () :int [
      10
    ]))
    (process (get-number))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}
