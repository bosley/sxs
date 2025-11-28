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

TEST_CASE("functions - parse and execute", "[unit][core][functions][parse]") {
  auto source = load_test_file("test_functions.sxs");

  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("functions - fn returns aberrant",
          "[unit][core][functions][aberrant]") {
  std::string source = R"([
    (set my-fn (fn (x :int) :int [
      (set r 1)
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK(interpreter->has_symbol("my-fn"));

  auto fn_parsed = slp::parse("my-fn");
  REQUIRE(fn_parsed.is_success());
  auto fn_obj = fn_parsed.take();
  auto fn_val = interpreter->eval(fn_obj);
  CHECK(fn_val.type() == slp::slp_type_e::ABERRANT);
}

TEST_CASE("functions - call with correct arguments",
          "[unit][core][functions][call]") {
  std::string source = R"([
    (set add (fn (a :int b :int) :int [
      (set result 42)
    ]))
    (set call-result (add 10 20))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("call-result"));
}

TEST_CASE("functions - parameters bound in function scope",
          "[unit][core][functions][params]") {
  std::string source = R"([
    (set check-param (fn (x :int) :int [
      (set captured-x x)
    ]))
    (check-param 777)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK_FALSE(interpreter->has_symbol("x"));
  CHECK_FALSE(interpreter->has_symbol("captured-x"));
}

TEST_CASE("functions - no parameters function",
          "[unit][core][functions][no-params]") {
  std::string source = R"([
    (set no-args (fn () :int [
      (set internal 99)
    ]))
    (no-args)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK_FALSE(interpreter->has_symbol("internal"));
}

TEST_CASE("functions - multiple parameters different types",
          "[unit][core][functions][multi-param]") {
  std::string source = R"([
    (set multi (fn (i :int r :real s :symbol) :int [
      (set done 1)
    ]))
    (multi 42 3.14 test)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("functions - function call returns last expression",
          "[unit][core][functions][return]") {
  std::string source = R"([
    (set ret-fn (fn (x :int) :int [
      (set result x)
    ]))
    (set captured-return (ret-fn 123))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("captured-return"));
}
