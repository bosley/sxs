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

TEST_CASE("lambda types - parse and execute all lambda type tests",
          "[unit][core][lambda][types]") {
  auto source = load_test_file("test_lambda_types.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - match same lambda", "[unit][core][lambda][match]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def result (match add
      (add "matched!")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("result"));
}

TEST_CASE("lambda types - match different lambdas",
          "[unit][core][lambda][match]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def mul (fn (x :int y :int) :int [
      100
    ]))
    (def result (match add
      (mul "wrong")
      (add "correct")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - match no handler returns error",
          "[unit][core][lambda][match][error]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def mul (fn (x :int y :int) :int [
      100
    ]))
    (def result (match add
      (mul "wrong")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);

  CHECK(result_val.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("lambda types - match with mixed types",
          "[unit][core][lambda][match][mixed]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def result (match add
      (5 "integer")
      (add "lambda!")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - reflect with matching signature",
          "[unit][core][lambda][reflect]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def result (reflect add
      (:fn<int,int>int "matched!")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - reflect with multiple signatures",
          "[unit][core][lambda][reflect]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def concat (fn (s1 :str s2 :str) :str [
      "result"
    ]))
    (def result1 (reflect add
      (:fn<str,str>str "str function")
      (:fn<int,int>int "int function")))
    (def result2 (reflect concat
      (:fn<int,int>int "int function")
      (:fn<str,str>str "str function")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - reflect with no params signature",
          "[unit][core][lambda][reflect]") {
  std::string source = R"([
    (def get-const (fn () :int [
      42
    ]))
    (def result (reflect get-const
      (:fn<>int "no-param function")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - reflect no matching signature returns error",
          "[unit][core][lambda][reflect][error]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def result (reflect add
      (:fn<str,str>str "wrong signature")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);

  CHECK(result_val.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("lambda types - reflect with aberrant catch-all",
          "[unit][core][lambda][reflect]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def result (reflect add
      (:aberrant "any lambda!")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - reflect mixed with regular types",
          "[unit][core][lambda][reflect][mixed]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def result1 (reflect add
      (:int "integer")
      (:fn<int,int>int "lambda")))
    (def x 5)
    (def result2 (reflect x
      (:int "integer")
      (:fn<int,int>int "lambda")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - match lambda identity preserved through def",
          "[unit][core][lambda][match][identity]") {
  std::string source = R"([
    (def add (fn (a :int b :int) :int [
      42
    ]))
    (def same-add add)
    (def result (match same-add
      (add "matched through alias!")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("lambda types - reflect complex signature",
          "[unit][core][lambda][reflect][complex]") {
  std::string source = R"([
    (def complex-fn (fn (a :int b :real c :str d :symbol) :int [
      42
    ]))
    (def result (reflect complex-fn
      (:fn<int,real,str,symbol>int "complex signature!")))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}
