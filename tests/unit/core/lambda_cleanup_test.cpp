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
    (set persistent (fn (x :int) :int [
      (set r 1)
    ]))
    (set outer (fn () :int [
      (set scoped (fn (y :int) :int [
        (set r 2)
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
    (set outer (fn () :int [
      (set inner-fn (fn (x :int) :int [
        (set r 1)
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
    (set saved-fn none)
    (set outer (fn () :int [
      (set temp-fn (fn (x :int) :int [
        (set r 1)
      ]))
      (set saved-fn temp-fn)
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
    (set level1-fn (fn (x :int) :int [
      (set r 1)
    ]))
    (set fn-outer (fn () :int [
      (set level2-fn (fn (x :int) :int [
        (set r 2)
      ]))
      (set fn-inner (fn () :int [
        (set level3-fn (fn (x :int) :int [
          (set r 3)
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
    (set outer (fn () :int [
      (set fn1 (fn (x :int) :int [ (set r 1) ]))
      (set fn2 (fn (x :int) :int [ (set r 2) ]))
      (set fn3 (fn (x :int) :int [ (set r 3) ]))
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
    (set outer (fn (x :int) :int [
      (set inner (fn (y :int) :int [
        (set r 1)
      ]))
      (inner x)
      (set result 42)
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
