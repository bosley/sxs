#include <core/core.hpp>
#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <filesystem>
#include <fstream>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <sxs/slp/slp.hpp>

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

TEST_CASE("basic scoping - parse and execute", "[unit][core][scoping][parse]") {
  auto source = load_test_file("test_basic_scoping.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  CHECK(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 100);
}

TEST_CASE("basic scoping - symbol visibility after scope exit",
          "[unit][core][scoping][visibility]") {
  std::string source = R"([
    (def outer 100)
    (def test-fn (fn () :int [
      (def inner 42)
    ]))
    (test-fn)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK(interpreter->has_symbol("outer"));
  CHECK_FALSE(interpreter->has_symbol("inner"));
}

TEST_CASE("basic scoping - symbol shadowing",
          "[unit][core][scoping][shadowing]") {
  std::string source = R"([
    (def x 100)
    (def shadow-fn (fn () :int [
      (def x 200)
      (def inner-x x)
    ]))
    (shadow-fn)
    (def outer-x x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK(interpreter->has_symbol("outer-x"));

  auto outer_x_parsed = slp::parse("outer-x");
  REQUIRE(outer_x_parsed.is_success());
  auto outer_x_obj = outer_x_parsed.take();
  auto outer_x_val = interpreter->eval(outer_x_obj);
  CHECK(outer_x_val.type() == slp::slp_type_e::INTEGER);
  CHECK(outer_x_val.as_int() == 100);
}

TEST_CASE("basic scoping - nested scope access to outer",
          "[unit][core][scoping][nested]") {
  std::string source = R"([
    (def outer 999)
    (def capture-fn (fn () :int [
      (def captured outer)
    ]))
    (capture-fn)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK_FALSE(interpreter->has_symbol("captured"));
}
