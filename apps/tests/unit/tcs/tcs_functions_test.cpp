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

TEST_CASE("tcs functions - parse and type check function definitions",
          "[unit][tcs][functions]") {
  auto source = load_test_file("test_functions.sxs");
  auto logger = create_test_logger();

  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test_functions.sxs"));
}

TEST_CASE("tcs functions - simple function definition",
          "[unit][tcs][functions][basic]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - function with real parameters",
          "[unit][tcs][functions][real]") {
  std::string source = R"([
    (def calculate (fn (x :real y :real) :real [
      3.14
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - function with string return",
          "[unit][tcs][functions][string]") {
  std::string source = R"([
    (def to-string (fn (val :int) :str [
      "converted"
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - function with no parameters",
          "[unit][tcs][functions][no-params]") {
  std::string source = R"([
    (def get-constant (fn () :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - function return type mismatch",
          "[unit][tcs][functions][error]") {
  std::string source = R"([
    (def bad-func (fn (x :int) :int [
      "string-not-int"
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - function with multiple parameters",
          "[unit][tcs][functions][multi-params]") {
  std::string source = R"([
    (def complex-func (fn (a :int b :real c :str d :symbol) :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - invalid parameter type",
          "[unit][tcs][functions][invalid-param]") {
  std::string source = R"([
    (def bad-func (fn (x :invalid-type) :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs functions - invalid return type",
          "[unit][tcs][functions][invalid-return]") {
  std::string source = R"([
    (def bad-func (fn (x :int) :invalid-type [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}
