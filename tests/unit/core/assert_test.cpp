#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <fstream>
#include <slp/slp.hpp>
#include <snitch/snitch.hpp>
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
} // namespace

TEST_CASE("assert - basic test with file", "[unit][core][assert]") {
  auto source = load_test_file("test_assert.sxs");
  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("assert - passes with truthy condition", "[unit][core][assert]") {
  std::string source = R"([
    (assert 1 "this should pass")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("assert - throws with falsy condition", "[unit][core][assert]") {
  std::string source = R"([
    (assert 0 "this should fail")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("assert - throws with custom message", "[unit][core][assert]") {
  std::string source = R"([
    (assert 0 "custom error message")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();

  try {
    interpreter->eval(obj);
    FAIL("assert should have thrown");
  } catch (const std::runtime_error &e) {
    std::string msg = e.what();
    CHECK(msg == "custom error message");
  }
}

TEST_CASE("assert - requires exactly 2 arguments", "[unit][core][assert]") {
  std::string source = R"([
    (assert 1)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("assert - condition must be integer", "[unit][core][assert]") {
  std::string source = R"([
    (assert "not an int" "message")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}
