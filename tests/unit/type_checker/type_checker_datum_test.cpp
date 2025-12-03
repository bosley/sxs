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
