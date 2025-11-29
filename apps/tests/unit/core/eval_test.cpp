#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <fstream>
#include <snitch/snitch.hpp>
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

TEST_CASE("eval - basic test with file", "[unit][core][eval]") {
  auto source = load_test_file("test_eval.sxs");
  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("eval - evaluates simple expression", "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (def expr "(if 1 100 200)")
      (def result (eval expr))
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK(interpreter->has_symbol("result"));

  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);

  CHECK(result_val.type() == slp::slp_type_e::INTEGER);
  CHECK(result_val.as_int() == 100);
}

TEST_CASE("eval - definitions don't leak to outer scope",
          "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (def code "[(def x 42) x]")
      (def result (eval code))
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK(interpreter->has_symbol("result"));

  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);

  CHECK(result_val.type() == slp::slp_type_e::INTEGER);
  CHECK(result_val.as_int() == 42);

  CHECK_FALSE(interpreter->has_symbol("x"));
}

TEST_CASE("eval - can access outer scope symbols", "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (def outer-var 99)
      (def result (eval "outer-var"))
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);

  CHECK(result_val.type() == slp::slp_type_e::INTEGER);
  CHECK(result_val.as_int() == 99);
}

TEST_CASE("eval - requires exactly 1 argument", "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (eval "1" "2")
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("eval - requires string argument", "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (eval 42)
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("eval - throws on malformed code", "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (eval "(def x")
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("eval - evaluates bracket list with multiple statements",
          "[unit][core][eval]") {
  std::string source = R"sxs(
    [
      (def result (eval "[(def a 10) (def b 20) (if 1 a b)]"))
    ]
  )sxs";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);

  CHECK(result_val.type() == slp::slp_type_e::INTEGER);
  CHECK(result_val.as_int() == 10);

  CHECK_FALSE(interpreter->has_symbol("a"));
  CHECK_FALSE(interpreter->has_symbol("b"));
}
