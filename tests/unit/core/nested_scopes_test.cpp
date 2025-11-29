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

TEST_CASE("nested scopes - parse and execute", "[unit][core][nested][parse]") {
  auto source = load_test_file("test_nested_scopes.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("nested scopes - deep nesting visibility",
          "[unit][core][nested][visibility]") {
  std::string source = R"([
    (def level1 100)
    (def fn1 (fn () :int [
      (def level2 200)
      (def fn2 (fn () :int [
        (def level3 300)
        (def fn3 (fn () :int [
          (def level4 400)
          (def all-levels level1)
        ]))
        (fn3)
      ]))
      (fn2)
    ]))
    (fn1)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("level1"));
  CHECK_FALSE(interpreter->has_symbol("level2"));
  CHECK_FALSE(interpreter->has_symbol("level3"));
  CHECK_FALSE(interpreter->has_symbol("level4"));
  CHECK_FALSE(interpreter->has_symbol("all-levels"));
}

TEST_CASE("nested scopes - shadowing at multiple levels",
          "[unit][core][nested][shadow]") {
  std::string source = R"([
    (def x 1)
    (def fn1 (fn () :int [
      (def x 2)
      (def fn2 (fn () :int [
        (def x 3)
        (def fn3 (fn () :int [
          (def x 4)
          (def deepest x)
        ]))
        (fn3)
        (def level3-x x)
      ]))
      (fn2)
      (def level2-x x)
    ]))
    (fn1)
    (def level1-x x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  auto level1_x = slp::parse("level1-x");
  REQUIRE(level1_x.is_success());
  auto obj1 = level1_x.take();
  auto val1 = interpreter->eval(obj1);
  CHECK(val1.as_int() == 1);

  CHECK_FALSE(interpreter->has_symbol("deepest"));
}

TEST_CASE("nested scopes - nested functions",
          "[unit][core][nested][functions]") {
  std::string source = R"([
    (def outer-fn (fn (x :int) :int [
      (def inner-fn (fn (y :int) :int [
        (def sum 42)
      ]))
      (inner-fn 20)
      (def result 1)
    ]))
    (outer-fn 10)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("outer-fn"));
  CHECK_FALSE(interpreter->has_symbol("inner-fn"));
}

TEST_CASE("nested scopes - function accessing outer scope",
          "[unit][core][nested][capture]") {
  std::string source = R"([
    (def outer-var 999)
    (def fn-captures (fn (x :int) :int [
      (def captured outer-var)
    ]))
    (fn-captures 1)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("nested scopes - multiple nested bracket scopes",
          "[unit][core][nested][brackets]") {
  std::string source = R"([
    (def a 1)
    (def fn1 (fn () :int [
      (def b 2)
      (def fn2 (fn () :int [
        (def c 3)
      ]))
      (fn2)
      (def after-c a)
    ]))
    (fn1)
    (def final a)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("a"));
  CHECK_FALSE(interpreter->has_symbol("after-c"));
  CHECK(interpreter->has_symbol("final"));
  CHECK_FALSE(interpreter->has_symbol("b"));
  CHECK_FALSE(interpreter->has_symbol("c"));
}

TEST_CASE("nested scopes - scope isolation between siblings",
          "[unit][core][nested][sibling]") {
  std::string source = R"([
    (def fn1 (fn () :int [
      (def first-scope 1)
    ]))
    (fn1)
    (def fn2 (fn () :int [
      (def second-scope 2)
    ]))
    (fn2)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK_FALSE(interpreter->has_symbol("first-scope"));
  CHECK_FALSE(interpreter->has_symbol("second-scope"));
}
