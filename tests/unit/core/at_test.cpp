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

TEST_CASE("at - comprehensive test with file", "[unit][core][at]") {
  auto source = load_test_file("test_at.sxs");
  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("at - index paren list", "[unit][core][at]") {
  std::string source = R"([
    (def list '(1 2 3 4 5))
    (def first (at 0 list))
    (def last (at 4 list))
    (def middle (at 2 list))
    middle
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 3);
}

TEST_CASE("at - index bracket list", "[unit][core][at]") {
  std::string source = R"([
    (def list (cast :list-b '(10 20 30)))
    (def elem (at 1 list))
    elem
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 20);
}

TEST_CASE("at - index brace list", "[unit][core][at]") {
  std::string source = R"([
    (def list {100 200 300})
    (def elem (at 2 list))
    elem
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 300);
}

TEST_CASE("at - index string returns byte value", "[unit][core][at]") {
  std::string source = R"([
    (def str "ABC")
    (def char_code (at 0 str))
    char_code
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 65);
}

TEST_CASE("at - verify byte values from string", "[unit][core][at]") {
  std::string source = R"([
    (def result (at 0 "A"))
    result
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 65);
}

TEST_CASE("at - multiple byte values from string", "[unit][core][at]") {
  std::string source_a = R"([(at 0 "ABC")])";
  std::string source_b = R"([(at 1 "ABC")])";
  std::string source_c = R"([(at 2 "ABC")])";

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();

  auto parse_a = slp::parse(source_a);
  REQUIRE(parse_a.is_success());
  auto interp_a = pkg::core::create_interpreter(symbols);
  auto obj_a = parse_a.take();
  auto result_a = interp_a->eval(obj_a);
  REQUIRE(result_a.type() == slp::slp_type_e::INTEGER);
  CHECK(result_a.as_int() == 65);

  auto parse_b = slp::parse(source_b);
  REQUIRE(parse_b.is_success());
  auto interp_b = pkg::core::create_interpreter(symbols);
  auto obj_b = parse_b.take();
  auto result_b = interp_b->eval(obj_b);
  REQUIRE(result_b.type() == slp::slp_type_e::INTEGER);
  CHECK(result_b.as_int() == 66);

  auto parse_c = slp::parse(source_c);
  REQUIRE(parse_c.is_success());
  auto interp_c = pkg::core::create_interpreter(symbols);
  auto obj_c = parse_c.take();
  auto result_c = interp_c->eval(obj_c);
  REQUIRE(result_c.type() == slp::slp_type_e::INTEGER);
  CHECK(result_c.as_int() == 67);
}

TEST_CASE("at - out of bounds returns error object", "[unit][core][at]") {
  std::string source = R"([
    (def list {1 2 3})
    (def result (at 10 list))
    result
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  CHECK(result.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("at - negative index returns error object", "[unit][core][at]") {
  std::string source = R"([
    (def list '(1 2 3))
    (def result (at -1 list))
    result
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  CHECK(result.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("at - string out of bounds returns error", "[unit][core][at]") {
  std::string source = R"([
    (def result (at 100 "test"))
    result
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  CHECK(result.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("at - empty list access returns error", "[unit][core][at]") {
  std::string source = R"([
    (def empty '())
    (def result (at 0 empty))
    result
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  CHECK(result.type() == slp::slp_type_e::ERROR);
}

TEST_CASE("at - non-integer index throws", "[unit][core][at]") {
  std::string source = R"([
    (def list '(1 2 3))
    (def result (at "not-an-int" list))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("at - non-list collection throws", "[unit][core][at]") {
  std::string source = R"([
    (def result (at 0 42))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("at - wrong number of arguments throws", "[unit][core][at]") {
  std::string source = R"([
    (def result (at 0))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("at - access nested lists", "[unit][core][at]") {
  std::string source = R"([
    (def nested '((1 2) (3 4) (5 6)))
    (def inner (at 1 nested))
    inner
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::PAREN_LIST);
  auto inner_list = result.as_list();
  CHECK(inner_list.size() == 2);
  auto first_elem = inner_list.at(0);
  REQUIRE(first_elem.type() == slp::slp_type_e::INTEGER);
  CHECK(first_elem.as_int() == 3);
}

TEST_CASE("at - access mixed type list", "[unit][core][at]") {
  std::string source = R"([
    (def mixed '(42 "hello" 3.14))
    (def int_val (at 0 mixed))
    (def str_val (at 1 mixed))
    (def real_val (at 2 mixed))
    real_val
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::REAL);
  CHECK(result.as_real() == 3.14);
}

TEST_CASE("at - access quoted list", "[unit][core][at]") {
  std::string source = R"([
    (def quoted '(7 8 9))
    (def elem (at 1 quoted))
    elem
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 8);
}

TEST_CASE("at - single element list", "[unit][core][at]") {
  std::string source = R"([
    (def single {999})
    (def elem (at 0 single))
    elem
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 999);
}

TEST_CASE("at - zero value element", "[unit][core][at]") {
  std::string source = R"([
    (def list {0 1 2})
    (def zero (at 0 list))
    zero
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);
  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 0);
}
