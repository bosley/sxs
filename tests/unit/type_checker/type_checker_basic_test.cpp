#include <core/type_checker/type_checker.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

namespace {

pkg::core::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

} // namespace

TEST_CASE("type_checker types - integer literal",
          "[unit][type_checker][types]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("42");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker types - real literal", "[unit][type_checker][types]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("3.14");
  CHECK(type.base_type == slp::slp_type_e::REAL);
}

TEST_CASE("type_checker types - string literal",
          "[unit][type_checker][types]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("\"hello\"");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("type_checker def - integer definition",
          "[unit][type_checker][def]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def x 42) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker def - real definition", "[unit][type_checker][def]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def y 3.14) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker def - string definition", "[unit][type_checker][def]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def name \"test\") ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker def - symbol reference evaluates",
          "[unit][type_checker][def]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def x 42) (def y x) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker def - redefinition fails",
          "[unit][type_checker][def][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ (def x 1) (def x 2) ]"),
                  std::exception);
}

TEST_CASE("type_checker def - undefined symbol returns symbol type",
          "[unit][type_checker][def]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("undefined_var");
  CHECK(type.base_type == slp::slp_type_e::SYMBOL);
}

TEST_CASE("type_checker fn - returns aberrant type",
          "[unit][type_checker][fn]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn () :int [ 0 ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("type_checker fn - with parameters", "[unit][type_checker][fn]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("(fn (a :int b :int) :int [ (debug a b) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("type_checker fn - return type mismatch fails",
          "[unit][type_checker][fn][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(fn () :str [ 42 ])"),
                  std::exception);
}

TEST_CASE("type_checker fn - body returns correct type",
          "[unit][type_checker][fn]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn () :int [ 42 ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
}

TEST_CASE("type_checker fn - string return type", "[unit][type_checker][fn]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn () :str [ \"test\" ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
}

TEST_CASE("type_checker if - returns integer type",
          "[unit][type_checker][if]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(if 1 10 20)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker if - returns string type", "[unit][type_checker][if]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(if 1 \"yes\" \"no\")");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("type_checker if - returns real type", "[unit][type_checker][if]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(if 0 3.14 2.71)");
  CHECK(type.base_type == slp::slp_type_e::REAL);
}

TEST_CASE("type_checker if - non-integer condition fails",
          "[unit][type_checker][if][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(if \"string\" 1 2)"),
                  std::exception);
}

TEST_CASE("type_checker if - branch type mismatch fails",
          "[unit][type_checker][if][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(if 1 42 \"string\")"),
                  std::exception);
}

TEST_CASE("type_checker match - returns none", "[unit][type_checker][match]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def x 42) (match x (42 (debug \"matched\")) (0 (debug \"zero\"))) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker match - with string", "[unit][type_checker][match]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def name \"test\") (match name (\"test\" (debug \"found\")) "
      "(\"other\" (debug \"not found\"))) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker reflect - returns none",
          "[unit][type_checker][reflect]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def x 42) (reflect x (:int (debug "
                               "\"integer\")) (:str (debug \"string\"))) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker reflect - multiple handlers",
          "[unit][type_checker][reflect]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def val 3.14) (reflect val (:int (debug \"int\")) (:real (debug "
      "\"real\")) (:str (debug \"string\"))) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker try - returns same type as body",
          "[unit][type_checker][try]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(try 42 0)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker try - with string", "[unit][type_checker][try]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(try \"success\" \"error\")");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("type_checker try - with block handler",
          "[unit][type_checker][try]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(try 42 [ 0 ])");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker try - type mismatch fails",
          "[unit][type_checker][try][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(try 42 \"error\")"),
                  std::exception);
}

TEST_CASE("type_checker recover - returns body type",
          "[unit][type_checker][recover]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(recover [ 42 ] [ 0 ])");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker recover - returns string type",
          "[unit][type_checker][recover]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("(recover [ \"success\" ] [ \"error\" ])");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("type_checker recover - with exception access",
          "[unit][type_checker][recover]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "(recover [ (debug \"main\") ] [ (debug $exception) ])");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker recover - type mismatch fails",
          "[unit][type_checker][recover][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(recover [ 42 ] [ \"string\" ])"),
                  std::exception);
}

TEST_CASE("type_checker assert - returns none",
          "[unit][type_checker][assert]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(assert 1 \"test passed\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker assert - multiple assertions return none",
          "[unit][type_checker][assert]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (assert 1 \"test 1\") (assert 0 \"test 2\") ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker assert - non-int condition fails",
          "[unit][type_checker][assert][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(assert \"bad\" \"message\")"),
                  std::exception);
}

TEST_CASE("type_checker assert - non-string message fails",
          "[unit][type_checker][assert][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(assert 1 42)"), std::exception);
}

TEST_CASE("type_checker eval - returns none", "[unit][type_checker][eval]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(eval \"(def x 42)\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker eval - non-string fails",
          "[unit][type_checker][eval][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(eval 42)"), std::exception);
}

TEST_CASE("type_checker apply - returns none", "[unit][type_checker][apply]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def add (fn (a :int b :int) :int [ (debug a "
                               "b) ])) (def args {1 2}) (apply add args) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker apply - non-lambda fails",
          "[unit][type_checker][apply][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ (def x 42) (apply x {1 2}) ]"),
                  std::exception);
}

TEST_CASE("type_checker apply - non-brace-list fails",
          "[unit][type_checker][apply][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ (def f (fn () :int [ 0 ])) (apply f (1 2)) ]"),
                  std::exception);
}

TEST_CASE("type_checker cast - returns target type int",
          "[unit][type_checker][cast]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(cast :int 3.14)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker cast - returns target type str",
          "[unit][type_checker][cast]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(cast :str 42)");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("type_checker cast - returns target type real",
          "[unit][type_checker][cast]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(cast :real 10)");
  CHECK(type.base_type == slp::slp_type_e::REAL);
}

TEST_CASE("type_checker cast - invalid type symbol fails",
          "[unit][type_checker][cast][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(cast :invalid_type 42)"),
                  std::exception);
}

TEST_CASE("type_checker do - returns aberrant", "[unit][type_checker][do]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("(do [ (debug $iterations) (done 42) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
}

TEST_CASE("type_checker do - nested loops return aberrant",
          "[unit][type_checker][do]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(do [ (do [ (done 1) ]) (done 0) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
}

TEST_CASE("type_checker done - returns none", "[unit][type_checker][done]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(do [ (done 42) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
}

TEST_CASE("type_checker done - outside loop fails",
          "[unit][type_checker][done][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(done 42)"), std::exception);
}

TEST_CASE("type_checker at - returns none", "[unit][type_checker][at]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(at 0 \"test\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker at - with brace list returns none",
          "[unit][type_checker][at]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def list {1 2 3}) (at 1 list) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker at - non-integer index fails",
          "[unit][type_checker][at][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(at \"bad\" \"test\")"),
                  std::exception);
}

TEST_CASE("type_checker eq - returns integer", "[unit][type_checker][eq]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(eq 1 2)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker eq - with strings returns integer",
          "[unit][type_checker][eq]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(eq \"hello\" \"world\")");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker eq - with reals returns integer",
          "[unit][type_checker][eq]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(eq 3.14 2.71)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker debug - returns integer",
          "[unit][type_checker][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(debug)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker debug - with args returns integer",
          "[unit][type_checker][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(debug 1 2 3)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker debug - variadic returns integer",
          "[unit][type_checker][debug]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(debug \"test\" 42 3.14)");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker complex - nested if returns correct type",
          "[unit][type_checker][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def x 5) (def y 10) (if (eq x y) (debug "
                               "\"equal\") (debug \"not equal\")) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker complex - lambda with try returns aberrant",
          "[unit][type_checker][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "(fn (a :int b :int) :int [ (try (debug a b) 0) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("type_checker scoping - shadowing works correctly",
          "[unit][type_checker][scoping]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def x 10) (def func (fn (x :int) :int [ (debug x) ])) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker scoping - outer scope access works",
          "[unit][type_checker][scoping]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def outer 100) (def func (fn (x :int) :int [ (debug outer) ])) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("type_checker bracket list - returns last expression type",
          "[unit][type_checker][bracket]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def x 1) (def y 2) 42 ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("type_checker bracket list - last is string",
          "[unit][type_checker][bracket]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def x 1) \"result\" ]");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}
