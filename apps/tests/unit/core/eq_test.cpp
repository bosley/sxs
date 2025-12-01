#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <snitch/snitch.hpp>
#include <sxs/slp/slp.hpp>

TEST_CASE("eq - integers equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 42 42))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - integers not equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 42 43))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - reals equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 3.14 3.14))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - reals not equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 3.14 2.71))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - symbols equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 'hello 'hello))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - symbols not equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 'hello 'world))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - strings equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq "hello" "hello"))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - strings not equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq "hello" "world"))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - int vs real returns false", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 42 42.0))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - int vs string returns false", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 42 "42"))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - string vs symbol returns false", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq "hello" 'hello))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - empty lists equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq () ()))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - paren lists equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 2 3) '(1 2 3)))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - paren lists not equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 2 3) '(1 2 4)))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - bracket lists equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '[1 2 3] '[1 2 3]))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - brace lists equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '{1 2 3} '{1 2 3}))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - lists different sizes", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 2) '(1 2 3)))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - nested lists equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 (2 3) 4) '(1 (2 3) 4)))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - nested lists not equal at depth", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 (2 3) 4) '(1 (2 5) 4)))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - mixed type lists", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 "hello" 'world) '(1 "hello" 'world)))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - same lambda identity", "[unit][core][eq]") {
  std::string source = R"([
    (def add (fn (x :int) :int [x]))
    (def result (eq add add))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - different lambda identities", "[unit][core][eq]") {
  std::string source = R"([
    (def add (fn (x :int) :int [x]))
    (def mul (fn (x :int) :int [x]))
    (def result (eq add mul))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - error objects same message", "[unit][core][eq]") {
  std::string source = R"([
    (def err1 @(test error))
    (def err2 @(test error))
    (def result (eq err1 err2))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - error objects different messages", "[unit][core][eq]") {
  std::string source = R"([
    (def err1 @(error one))
    (def err2 @(error two))
    (def result (eq err1 err2))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - quoted values equal", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '42 '42))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - paren vs bracket list returns false", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 2 3) '[1 2 3]))
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
  CHECK(result_val.as_int() == 0);
}

TEST_CASE("eq - deeply nested equal lists", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq '(1 (2 (3 (4)))) '(1 (2 (3 (4))))))
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
  CHECK(result_val.as_int() == 1);
}

TEST_CASE("eq - requires exactly 2 arguments", "[unit][core][eq]") {
  std::string source = R"([
    (def result (eq 1))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}
