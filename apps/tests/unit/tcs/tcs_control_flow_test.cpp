#include <core/tcs/tcs.hpp>
#include <fstream>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace {

std::string load_test_file(const std::string &filename) {
  std::string path = std::string(TEST_DATA_DIR) + "/" + filename;
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open test file: " + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

pkg::core::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

} // namespace

TEST_CASE("tcs control flow - parse and type check control structures",
          "[unit][tcs][control]") {
  auto source = load_test_file("test_control_flow.sxs");
  auto logger = create_test_logger();

  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test_control_flow.sxs"));
}

TEST_CASE("tcs control flow - if statement with matching types",
          "[unit][tcs][control][if]") {
  std::string source = R"([
    (def result (if 1 42 43))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs control flow - if statement with type mismatch",
          "[unit][tcs][control][if][error]") {
  std::string source = R"([
    (def result (if 1 42 "string"))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs control flow - if with non-integer condition",
          "[unit][tcs][control][if][error]") {
  std::string source = R"([
    (def result (if "not-int" 42 43))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs control flow - match with integer patterns",
          "[unit][tcs][control][match]") {
  std::string source = R"([
    (def value 5)
    (def result (match value
      (5 "five")
      (10 "ten")))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs control flow - match with string patterns",
          "[unit][tcs][control][match]") {
  std::string source = R"([
    (def value "hello")
    (def result (match value
      ("hello" 1)
      ("world" 2)))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs control flow - reflect with type handlers",
          "[unit][tcs][control][reflect]") {
  std::string source = R"([
    (def data 42)
    (def result (reflect data
      (:int "integer")
      (:str "string")))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs control flow - reflect with invalid type symbol",
          "[unit][tcs][control][reflect][error]") {
  std::string source = R"([
    (def data 42)
    (def result (reflect data
      (:invalid-type "bad")
      (:int "good")))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}
