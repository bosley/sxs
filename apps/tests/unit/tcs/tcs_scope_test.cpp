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

TEST_CASE("tcs scope - nested scopes", "[unit][tcs][scope]") {
  std::string source = R"([
    (def x 1)
    (def outer (fn () :int [
      (def y 2)
      (def inner (fn () :int [
        (def z 3)
        42
      ]))
      10
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs scope - parameter shadowing", "[unit][tcs][scope][shadow]") {
  std::string source = R"([
    (def x 1)
    (def func (fn (x :int) :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs scope - local variable in function",
          "[unit][tcs][scope][local]") {
  std::string source = R"([
    (def func (fn (x :int) :int [
      (def temp 10)
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs scope - bracket list scoping", "[unit][tcs][scope][bracket]") {
  std::string source = R"([
    (def x 1)
    (def y 2)
    (def z 3)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}
