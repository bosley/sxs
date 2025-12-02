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

TEST_CASE("cast - basic test with file", "[unit][core][cast]") {
  auto source = load_test_file("test_cast.sxs");
  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - successful cast with matching types", "[unit][core][cast]") {
  std::string source = R"([
    (def x 42)
    (def y (cast :int x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - throws with mismatched types", "[unit][core][cast]") {
  std::string source = R"([
    (cast :int "not an int")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("cast - throws with string to int mismatch", "[unit][core][cast]") {
  std::string source = R"([
    (def str "hello")
    (cast :int str)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("cast - requires exactly 2 arguments", "[unit][core][cast]") {
  std::string source = R"([
    (cast :int)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("cast - first argument must be a type symbol", "[unit][core][cast]") {
  std::string source = R"([
    (cast 42 100)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("cast - invalid type symbol", "[unit][core][cast]") {
  std::string source = R"([
    (cast :invalid-type 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("cast - real type", "[unit][core][cast]") {
  std::string source = R"([
    (def x 3.14)
    (def y (cast :real x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - string type", "[unit][core][cast]") {
  std::string source = R"([
    (def x "hello")
    (def y (cast :str x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - symbol type", "[unit][core][cast]") {
  std::string source = R"([
    (def x test-symbol)
    (def y (cast :symbol x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - with recover to catch mismatch", "[unit][core][cast]") {
  std::string source = R"([
    (def result (recover [
      (cast :int "not an int")
      999
    ] [
      123
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert real to int", "[unit][core][cast]") {
  std::string source = R"([
    (def x 3.14)
    (def y (cast :int x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert int to real", "[unit][core][cast]") {
  std::string source = R"([
    (def x 42)
    (def y (cast :real x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert negative real to int", "[unit][core][cast]") {
  std::string source = R"([
    (def x -3.99)
    (def y (cast :int x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert paren list to brace list", "[unit][core][cast]") {
  std::string source = R"([
    (def x {1 2 3})
    (def y (cast :list-p x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert brace list to bracket list", "[unit][core][cast]") {
  std::string source = R"([
    (def x {1 2 3})
    (def y (cast :list-b x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert string to paren list", "[unit][core][cast]") {
  std::string source = R"([
    (def x "hello world")
    (def y (cast :list-p x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - convert string to bracket list", "[unit][core][cast]") {
  std::string source = R"([
    (def x "test data")
    (def y (cast :list-b x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("cast - verify int to real conversion value", "[unit][core][cast]") {
  std::string source = R"([
    (cast :real 42)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::REAL);
  CHECK(result.as_real() == 42.0);
}

TEST_CASE("cast - verify real to int conversion value", "[unit][core][cast]") {
  std::string source = R"([
    (cast :int 3.14)
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

TEST_CASE("cast - verify negative real to int conversion",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :int -5.99)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == -5);
}

TEST_CASE("cast - verify brace list to bracket list conversion",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :list-b {1 2 3})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::BRACKET_LIST);
  auto list = result.as_list();
  REQUIRE(list.size() == 3);
  CHECK(list.at(0).as_int() == 1);
  CHECK(list.at(1).as_int() == 2);
  CHECK(list.at(2).as_int() == 3);
}

TEST_CASE("cast - verify brace list to paren list conversion",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :list-p {10 20})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::PAREN_LIST);
  auto list = result.as_list();
  REQUIRE(list.size() == 2);
  CHECK(list.at(0).as_int() == 10);
  CHECK(list.at(1).as_int() == 20);
}

TEST_CASE("cast - verify string to paren list conversion",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :list-p "AB")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::PAREN_LIST);
  auto list = result.as_list();
  REQUIRE(list.size() == 2);
  CHECK(list.at(0).as_int() == 65);
  CHECK(list.at(1).as_int() == 66);
}

TEST_CASE("cast - verify paren list to string conversion",
          "[unit][core][cast]") {
  std::string source = R"([
    (def x {65 66 67})
    (cast :str x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result.as_string().to_string() == "ABC");
}

TEST_CASE("cast - verify identity cast preserves value", "[unit][core][cast]") {
  std::string source = R"([
    (cast :int 99)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 99);
}

TEST_CASE("cast - verify string identity cast", "[unit][core][cast]") {
  std::string source = R"([
    (cast :str "test string")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result.as_string().to_string() == "test string");
}

TEST_CASE("cast - int list to string via ASCII bytes", "[unit][core][cast]") {
  std::string source = R"([
    (cast :str {65 66 67})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result.as_string().to_string() == "ABC");
}

TEST_CASE("cast - int modulo 256 when converting to string",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :str {300 256 65})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  auto str = result.as_string().to_string();
  REQUIRE(str.size() == 3);
  CHECK(static_cast<unsigned char>(str[0]) == 44);
  CHECK(static_cast<unsigned char>(str[1]) == 0);
  CHECK(static_cast<unsigned char>(str[2]) == 65);
}

TEST_CASE("cast - string to byte list extracts RUNEs as integers",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :list-c "ABC")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::BRACE_LIST);
  auto list = result.as_list();
  REQUIRE(list.size() == 3);
  CHECK(list.at(0).as_int() == 65);
  CHECK(list.at(1).as_int() == 66);
  CHECK(list.at(2).as_int() == 67);
}

TEST_CASE("cast - roundtrip int list to string and back",
          "[unit][core][cast]") {
  std::string source = R"([
    (def ints {72 101 108 108 111})
    (def str (cast :str ints))
    (cast :list-b str)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::BRACKET_LIST);
  auto list = result.as_list();
  REQUIRE(list.size() == 5);
  CHECK(list.at(0).as_int() == 72);
  CHECK(list.at(1).as_int() == 101);
  CHECK(list.at(2).as_int() == 108);
  CHECK(list.at(3).as_int() == 108);
  CHECK(list.at(4).as_int() == 111);
}

TEST_CASE("cast - nested strings extracted when converting to string",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :str {65 "BC" 68})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result.as_string().to_string() == "ABCD");
}

TEST_CASE("cast - quoted list (SOME) unwraps and converts",
          "[unit][core][cast]") {
  std::string source = R"([
    (cast :str '(65 66 67))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result.as_string().to_string() == "ABC");
}

TEST_CASE("cast - quoted list with def", "[unit][core][cast]") {
  std::string source = R"([
    (def quoted '(72 101 108 108 111))
    (cast :str quoted)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  REQUIRE(result.type() == slp::slp_type_e::DQ_LIST);
  CHECK(result.as_string().to_string() == "Hello");
}
