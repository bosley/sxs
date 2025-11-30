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

TEST_CASE("match - basic file test", "[unit][core][match]") {
  auto source = load_test_file("test_match.sxs");
  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("match - integer literal match", "[unit][core][match]") {
  std::string source = R"([
    (def x 42)
    (def result (match x
      (42 100)
      (3.14 200)
    ))
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
  CHECK(result_val.as_int() == 100);
}

TEST_CASE("match - real literal match", "[unit][core][match]") {
  std::string source = R"([
    (def x 3.14)
    (def result (match x
      (100 "int")
      (3.14 "real match")
    ))
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

  CHECK(result_val.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result_val.as_string().to_string() == "real match");
}

TEST_CASE("match - string literal match", "[unit][core][match]") {
  std::string source = R"([
    (def x "hello")
    (def result (match x
      ("world" 1)
      ("hello" 2)
    ))
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
  CHECK(result_val.as_int() == 2);
}

TEST_CASE("match - symbol type match", "[unit][core][match]") {
  std::string source = R"([
    (def x my-symbol)
    (def result (match x
      (my-symbol 999)
      (0 111)
    ))
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
  CHECK(result_val.as_int() == 999);
}

TEST_CASE("match - resolved symbol match", "[unit][core][match]") {
  std::string source = R"([
    (def target 100)
    (def pattern 100)
    (def result (match target
      (pattern 777)
      (200 888)
    ))
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

TEST_CASE("match - later handler matches", "[unit][core][match]") {
  std::string source = R"([
    (def x 50)
    (def result (match x
      (10 "first")
      (20 "second")
      (50 "third")
    ))
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

  CHECK(result_val.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result_val.as_string().to_string() == "third");
}

TEST_CASE("match - no handler matches returns error", "[unit][core][match]") {
  std::string source = R"([
    (def x 999)
    (def result (match x
      (1 "one")
      (2 "two")
    ))
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

  CHECK(result_val.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("match - type matches but value doesn't", "[unit][core][match]") {
  std::string source = R"([
    (def x 100)
    (def result (match x
      (200 "two hundred")
      (300 "three hundred")
    ))
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

  CHECK(result_val.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("match - wrong argument count", "[unit][core][match]") {
  std::string source = R"([
    (match 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("match - handler not paren list", "[unit][core][match]") {
  std::string source = R"([
    (match 42 [42 100])
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("match - handler wrong element count", "[unit][core][match]") {
  std::string source = R"([
    (match 42 (42))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("match - cannot match on aberrant", "[unit][core][match]") {
  std::string source = R"([
    (def my_fn (fn () :int [42]))
    (match my_fn
      (my_fn 100)
    )
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("match - multiple types same value different result",
          "[unit][core][match]") {
  std::string source = R"([
    (def x 7)
    (def result (match x
      ("test" "string")
      (7.0 "real")
      (7 "integer")
    ))
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

  CHECK(result_val.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result_val.as_string().to_string() == "integer");
}
