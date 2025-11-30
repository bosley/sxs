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

TEST_CASE("tcs error handling - parse and type check error handlers",
          "[unit][tcs][error]") {
  auto source = load_test_file("test_error_handling.sxs");
  auto logger = create_test_logger();

  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test_error_handling.sxs"));
}

TEST_CASE("tcs error handling - assert with integer condition",
          "[unit][tcs][error][assert]") {
  std::string source = R"([
    (assert 1 "test message")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - assert with non-integer condition",
          "[unit][tcs][error][assert][error]") {
  std::string source = R"([
    (assert "not-int" "test message")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - assert with non-string message",
          "[unit][tcs][error][assert][error]") {
  std::string source = R"([
    (assert 1 42)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - recover with bracket lists",
          "[unit][tcs][error][recover]") {
  std::string source = R"([
    (def result (recover
      [(def x 42)]
      [(def y $exception)]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - recover with matching types",
          "[unit][tcs][error][recover]") {
  std::string source = R"([
    (def result (recover
      [42]
      [0]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - recover with type mismatch",
          "[unit][tcs][error][recover][error]") {
  std::string source = R"([
    (def result (recover
      [42]
      ["string"]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - try with matching types",
          "[unit][tcs][error][try]") {
  std::string source = R"([
    (def result (try 42 0))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - try with type mismatch",
          "[unit][tcs][error][try][error]") {
  std::string source = R"([
    (def result (try 42 "string"))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - eval with string argument",
          "[unit][tcs][error][eval]") {
  std::string source = "[  (def code \"test\")  (def result (eval code))]";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs error handling - eval with non-string argument",
          "[unit][tcs][error][eval][error]") {
  std::string source = R"([
    (def result (eval 42))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}
