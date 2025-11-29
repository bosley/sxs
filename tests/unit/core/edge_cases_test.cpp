#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <fstream>
#include <sxs/slp/slp.hpp>
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

TEST_CASE("edge cases - parse and execute", "[unit][core][edge][parse]") {
  auto source = load_test_file("test_edge_cases.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("edge cases - undefined symbol as function call",
          "[unit][core][edge][undefined]") {
  std::string source = R"([
    (undefined-function 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::exception);
}

TEST_CASE("edge cases - wrong argument count", "[unit][core][edge][argcount]") {
  std::string source = R"([
    (def fn-two-args (fn (a :int b :int) :int [
      (def r 1)
    ]))
    (fn-two-args 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::exception);
}

TEST_CASE("edge cases - too many arguments", "[unit][core][edge][toomany]") {
  std::string source = R"([
    (def fn-one-arg (fn (x :int) :int [
      (def r 1)
    ]))
    (fn-one-arg 1 2 3)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::exception);
}

TEST_CASE("edge cases - accessing undefined symbol",
          "[unit][core][edge][undef-symbol]") {
  std::string source = R"([
    (debug some-undefined-symbol)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("edge cases - empty function body", "[unit][core][edge][empty]") {
  std::string source = R"([
    (def empty (fn () :int [
      (def dummy 0)
    ]))
    (empty)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("edge cases - function parameter shadowing",
          "[unit][core][edge][param-shadow]") {
  std::string source = R"([
    (def outer-x 100)
    (def shadow-fn (fn (x :int) :int [
      (def inner-x x)
    ]))
    (shadow-fn 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  auto outer_x_parsed = slp::parse("outer-x");
  REQUIRE(outer_x_parsed.is_success());
  auto outer_x_obj = outer_x_parsed.take();
  auto outer_x_val = interpreter->eval(outer_x_obj);
  CHECK(outer_x_val.as_int() == 100);
}
