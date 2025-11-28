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
    (set fn-two-args (fn (a :int b :int) :int [
      (set r 1)
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
    (set fn-one-arg (fn (x :int) :int [
      (set r 1)
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

TEST_CASE("edge cases - symbol shadowing function name",
          "[unit][core][edge][shadow]") {
  std::string source = R"([
    (set my-fn (fn (x :int) :int [
      (set r 1)
    ]))
    (my-fn 42)
    (set my-fn 999)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  auto my_fn_parsed = slp::parse("my-fn");
  REQUIRE(my_fn_parsed.is_success());
  auto my_fn_obj = my_fn_parsed.take();
  auto my_fn_val = interpreter->eval(my_fn_obj);
  CHECK(my_fn_val.type() == slp::slp_type_e::INTEGER);
  CHECK(my_fn_val.as_int() == 999);
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
    (set empty (fn () :int [
      (set dummy 0)
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
    (set outer-x 100)
    (set shadow-fn (fn (x :int) :int [
      (set inner-x x)
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
