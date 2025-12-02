#include <core/type_checker/type_checker.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

namespace {

pkg::core::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

} // namespace

TEST_CASE("datum debug - returns integer",
          "[unit][type_checker][datum][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("#(debug 1 2 3)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("datum debug - empty args returns integer",
          "[unit][type_checker][datum][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("#(debug)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("datum debug - with strings and numbers",
          "[unit][type_checker][datum][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("#(debug \"test\" 42 3.14)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("datum debug - with variables",
          "[unit][type_checker][datum][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def x 42) #(debug x) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("datum debug - with expressions",
          "[unit][type_checker][datum][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ #(debug (if 1 10 20)) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("datum debug - nested in lambda",
          "[unit][type_checker][datum][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f (fn (x :int) :int [ #(debug x) ])) (f 42) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("datum import - invalid symbol argument fails",
          "[unit][type_checker][datum][import][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ #(import 123 \"file.sxs\") ]"),
                  std::exception);
}

TEST_CASE("datum import - invalid path argument fails",
          "[unit][type_checker][datum][import][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ #(import lib 123) ]"),
                  std::exception);
}

TEST_CASE("datum import - odd number of args fails",
          "[unit][type_checker][datum][import][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ #(import lib) ]"),
                  std::exception);
}

TEST_CASE("datum load - non-string argument fails",
          "[unit][type_checker][datum][load][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("#(load 123)"), std::exception);
}

TEST_CASE("datum load - no arguments fails",
          "[unit][type_checker][datum][load][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("#(load)"), std::exception);
}
