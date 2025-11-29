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

TEST_CASE("lambda cleanup - parse and execute", "[unit][core][lambda][parse]") {
  auto source = load_test_file("test_lambda_cleanup.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda cleanup - persistent function remains callable",
          "[unit][core][lambda][persistent]") {
  std::string source = R"([
    (def persistent (fn (x :int) :int [
      (def r 1)
    ]))
    (def outer (fn () :int [
      (def scoped (fn (y :int) :int [
        (def r 2)
      ]))
      (scoped 100)
    ]))
    (outer)
    (persistent 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("persistent"));
  CHECK_FALSE(interpreter->has_symbol("scoped"));
}

TEST_CASE("lambda cleanup - scoped function symbol removed",
          "[unit][core][lambda][removed]") {
  std::string source = R"([
    (def outer (fn () :int [
      (def inner-fn (fn (x :int) :int [
        (def r 1)
      ]))
      (inner-fn 10)
    ]))
    (outer)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK_FALSE(interpreter->has_symbol("inner-fn"));
}

TEST_CASE("lambda cleanup - calling removed lambda fails",
          "[unit][core][lambda][call-removed]") {
  std::string source = R"([
    (def saved-fn none)
    (def outer (fn () :int [
      (def temp-fn (fn (x :int) :int [
        (def r 1)
      ]))
      (def saved-fn temp-fn)
    ]))
    (outer)
    (saved-fn 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::exception);
}

TEST_CASE("lambda cleanup - nested scope lambda cleanup",
          "[unit][core][lambda][nested]") {
  std::string source = R"([
    (def level1-fn (fn (x :int) :int [
      (def r 1)
    ]))
    (def fn-outer (fn () :int [
      (def level2-fn (fn (x :int) :int [
        (def r 2)
      ]))
      (def fn-inner (fn () :int [
        (def level3-fn (fn (x :int) :int [
          (def r 3)
        ]))
      ]))
      (fn-inner)
    ]))
    (fn-outer)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("level1-fn"));
  CHECK_FALSE(interpreter->has_symbol("level2-fn"));
  CHECK_FALSE(interpreter->has_symbol("level3-fn"));
}

TEST_CASE("lambda cleanup - multiple lambdas in same scope",
          "[unit][core][lambda][multiple]") {
  std::string source = R"([
    (def outer (fn () :int [
      (def fn1 (fn (x :int) :int [ (def r 1) ]))
      (def fn2 (fn (x :int) :int [ (def r 2) ]))
      (def fn3 (fn (x :int) :int [ (def r 3) ]))
      (fn1 1)
      (fn2 2)
      (fn3 3)
    ]))
    (outer)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK_FALSE(interpreter->has_symbol("fn1"));
  CHECK_FALSE(interpreter->has_symbol("fn2"));
  CHECK_FALSE(interpreter->has_symbol("fn3"));
}

TEST_CASE("lambda cleanup - lambda in function scope",
          "[unit][core][lambda][function-scope]") {
  std::string source = R"([
    (def outer (fn (x :int) :int [
      (def inner (fn (y :int) :int [
        (def r 1)
      ]))
      (inner x)
      (def result 42)
    ]))
    (outer 10)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("outer"));
  CHECK_FALSE(interpreter->has_symbol("inner"));
}
