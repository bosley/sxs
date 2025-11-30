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

TEST_CASE("if - parse and execute", "[unit][core][if][parse]") {
  auto source = load_test_file("test_if.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("if - condition true returns true branch", "[unit][core][if][true]") {
  std::string source = R"([
    (def result (if 1 42 99))
  ])";

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
}

TEST_CASE("if - condition false returns false branch",
          "[unit][core][if][false]") {
  std::string source = R"([
    (def result (if 0 42 99))
  ])";

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
  CHECK(result_val.as_int() == 99);
}

TEST_CASE("if - non-zero integer is true", "[unit][core][if][nonzero]") {
  std::string source = R"([
    (def result (if 5 100 200))
  ])";

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
  CHECK(result_val.as_int() == 100);
}

TEST_CASE("if - negative integer is true", "[unit][core][if][negative]") {
  std::string source = R"([
    (def result (if -1 100 200))
  ])";

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
  CHECK(result_val.as_int() == 100);
}

TEST_CASE("if - non-integer type is true", "[unit][core][if][nonint]") {
  std::string source = R"([
    (def result (if some-symbol 111 222))
  ])";

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
  CHECK(result_val.as_int() == 111);
}

TEST_CASE("if - real type is true", "[unit][core][if][real]") {
  std::string source = R"([
    (def result (if 3.14 111 222))
  ])";

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
  CHECK(result_val.as_int() == 111);
}

TEST_CASE("if - evaluates condition", "[unit][core][if][eval]") {
  std::string source = R"([
    (def x 10)
    (def result (if x 333 444))
  ])";

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
  CHECK(result_val.as_int() == 333);
}

TEST_CASE("if - bracket list returns last value true",
          "[unit][core][if][bracket-true]") {
  std::string source = R"([
    (def result (if 1 [
      (def a 1)
      (def b 2)
      555
    ] [
      (def c 4)
      666
    ]))
  ])";

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
  CHECK(result_val.as_int() == 555);
}

TEST_CASE("if - bracket list returns last value false",
          "[unit][core][if][bracket-false]") {
  std::string source = R"([
    (def result (if 0 [
      (def a 1)
      555
    ] [
      (def b 2)
      (def c 3)
      777
    ]))
  ])";

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
  CHECK(result_val.as_int() == 777);
}

TEST_CASE("if - works inside function", "[unit][core][if][function]") {
  std::string source = R"([
    (def check-fn (fn (val :int) :int [
      (if val 777 888)
    ]))
    (def result-true (check-fn 1))
    (def result-false (check-fn 0))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  auto result_true_parsed = slp::parse("result-true");
  REQUIRE(result_true_parsed.is_success());
  auto result_true_obj = result_true_parsed.take();
  auto result_true_val = interpreter->eval(result_true_obj);
  CHECK(result_true_val.as_int() == 777);

  auto result_false_parsed = slp::parse("result-false");
  REQUIRE(result_false_parsed.is_success());
  auto result_false_obj = result_false_parsed.take();
  auto result_false_val = interpreter->eval(result_false_obj);
  CHECK(result_false_val.as_int() == 888);
}

TEST_CASE("if - requires exactly 3 arguments", "[unit][core][if][args]") {
  std::string source = R"([
    (def result (if 1 42))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}
