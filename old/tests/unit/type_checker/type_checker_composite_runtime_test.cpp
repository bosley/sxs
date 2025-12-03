#include "core/context.hpp"
#include "core/core.hpp"
#include "core/instructions/datum.hpp"
#include "core/instructions/instructions.hpp"
#include "core/interpreter.hpp"
#include <snitch/snitch.hpp>

using namespace pkg::core;

TEST_CASE("runtime cast validates form element types",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2}))
    (debug x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime cast with wrong element count succeeds",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2 3}))
    (debug x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime cast with mixed types succeeds",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 "string"}))
    (debug x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form to list-c cast is noop",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2}))
    (def y (cast :list-c x))
    (debug y)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime nested form validation",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form inner {:int :int})
    #(define-form outer {:inner :str})
    (def i (cast :inner {1 2}))
    (def o (cast :outer {i "test"}))
    (debug o)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form with symbol resolution",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def a 10)
    (def b 20)
    (def p (cast :pair {a b}))
    (debug p)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form in function call",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def process (fn (p :pair) :int [
      42
    ]))
    (def x (cast :pair {1 2}))
    (def result (process x))
    (debug result)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form return from function",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def make_pair (fn (a :int b :int) :pair [
      (cast :pair {a b})
    ]))
    (def p (make_pair 5 10))
    (debug p)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime empty form", "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form empty {})
    (def e (cast :empty {}))
    (debug e)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime large form", "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form large {:int :str :real :int :str :real :int :str :real :int})
    (def l (cast :large {1 "a" 1.0 2 "b" 2.0 3 "c" 3.0 4}))
    (debug l)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime deeply nested forms", "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form level1 {:int :int})
    #(define-form level2 {:level1 :str})
    #(define-form level3 {:level2 :real})
    (def l1 (cast :level1 {1 2}))
    (def l2 (cast :level2 {l1 "test"}))
    (def l3 (cast :level3 {l2 3.14}))
    (debug l3)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form in if branches", "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (if 1 
      (cast :pair {1 2})
      (cast :pair {3 4})
    ))
    (debug x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form in try catch", "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (try 
      (cast :pair {1 2})
      (cast :pair {0 0})
    ))
    (debug x)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form in do loop", "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    (def result (do [
      (def x (cast :pair {1 2}))
      (done x)
    ]))
    (debug result)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime form with variadic elements",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form flexible {:int.. :str})
    (def f (cast :flexible {{1 2 3} "test"}))
    (debug f)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("runtime cast between compatible forms",
          "[type_checker][composite][runtime]") {
  auto logger = spdlog::default_logger();

  std::string source = R"([
    #(define-form pair {:int :int})
    #(define-form point {:int :int})
    (def p1 (cast :pair {1 2}))
    (def p2 (cast :point p1))
    (debug p2)
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto interpreter = create_interpreter(instructions_map);
  auto obj = parse_result.take();
  REQUIRE_NOTHROW(interpreter->eval(obj));
}
