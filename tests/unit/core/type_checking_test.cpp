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

TEST_CASE("type checking - parse and execute all correct types",
          "[unit][core][types][parse]") {
  auto source = load_test_file("test_type_checking.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("type checking - integer parameter validation",
          "[unit][core][types][int]") {
  std::string source = R"([
    (def int-fn (fn (x :int) :int [
      (def r 1)
    ]))
    (int-fn 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("type checking - real parameter validation",
          "[unit][core][types][real]") {
  std::string source = R"([
    (def real-fn (fn (x :real) :real [
      (def r 1.0)
    ]))
    (real-fn 3.14)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("type checking - symbol parameter validation",
          "[unit][core][types][symbol]") {
  std::string source = R"([
    (def sym-fn (fn (s :symbol) :symbol [
      (def r test)
    ]))
    (sym-fn my-symbol)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("type checking - string parameter validation",
          "[unit][core][types][string]") {
  std::string source = R"([
    (def str-fn (fn (s :str) :str [
      (def r "ok")
    ]))
    (str-fn "hello")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("type checking - wrong type throws error",
          "[unit][core][types][error]") {
  std::string source = R"([
    (def int-fn (fn (x :int) :int [
      (def r 1)
    ]))
    (int-fn 3.14)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::exception);
}

TEST_CASE("type checking - multiple parameters with mixed types",
          "[unit][core][types][mixed]") {
  std::string source = R"([
    (def mixed-fn (fn (i :int r :real s :symbol st :str) :int [
      (def done 1)
    ]))
    (mixed-fn 42 2.718 test "string")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("type checking - parameter type enforced at call time",
          "[unit][core][types][enforce]") {
  std::string source = R"([
    (def typed-fn (fn (x :int) :int [
      (def r 1)
    ]))
    (def wrong-val "not an int")
    (typed-fn wrong-val)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::exception);
}
