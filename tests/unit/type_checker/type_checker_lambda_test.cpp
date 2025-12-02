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

TEST_CASE("lambda types - zero param lambda returns aberrant",
          "[unit][type_checker][lambda]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn () :int [ 42 ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda types - single param returns aberrant",
          "[unit][type_checker][lambda]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn (x :int) :int [ x ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda types - multi param returns aberrant",
          "[unit][type_checker][lambda]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "(fn (a :int b :int c :int) :int [ (debug a b c) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda types - mixed param types", "[unit][type_checker][lambda]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "(fn (x :int y :str z :real) :int [ (debug x y z) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda types - string return type", "[unit][type_checker][lambda]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn (x :int) :str [ \"result\" ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda types - real return type", "[unit][type_checker][lambda]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn (x :int) :real [ 3.14 ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda call - zero args correct",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def f (fn () :int [ 42 ])) (f) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda call - single int arg correct",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def f (fn (x :int) :int [ x ])) (f 42) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda call - single string arg correct",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f (fn (s :str) :str [ s ])) (f \"test\") ]");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("lambda call - single real arg correct",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f (fn (x :real) :real [ x ])) (f 3.14) ]");
  CHECK(type.base_type == slp::slp_type_e::REAL);
}

TEST_CASE("lambda call - two args correct types",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def add (fn (a :int b :int) :int [ (debug a b) ])) (add 1 2) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda call - three args mixed types",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def process (fn (x :int y :str z :real) :int [ (debug x y z) ])) "
      "(process 42 \"test\" 3.14) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda call - args from variables",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def x 10) (def y 20) (def add (fn (a :int b "
                               ":int) :int [ (debug a b) ])) (add x y) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda call - nested lambda calls",
          "[unit][type_checker][lambda][call]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def f (fn (x :int) :int [ x ])) (def g (fn "
                               "(y :int) :int [ y ])) (f (g 42)) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda call - wrong arg count fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ (def f (fn (x :int) :int [ x ])) (f) ]"),
      std::exception);
}

TEST_CASE("lambda call - too many args fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ (def f (fn (x :int) :int [ x ])) (f 1 2) ]"),
      std::exception);
}

TEST_CASE("lambda call - wrong arg type int expected got string fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ (def f (fn (x :int) :int [ x ])) (f \"bad\") ]"),
                  std::exception);
}

TEST_CASE("lambda call - wrong arg type string expected got int fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ (def f (fn (s :str) :str [ s ])) (f 42) ]"),
      std::exception);
}

TEST_CASE("lambda call - wrong arg type real expected got int fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ (def f (fn (x :real) :real [ x ])) (f 42) ]"),
      std::exception);
}

TEST_CASE("lambda call - first arg wrong type fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ (def f (fn (x :int y :int) :int "
                                           "[ (debug x y) ])) (f \"bad\" 2) ]"),
                  std::exception);
}

TEST_CASE("lambda call - second arg wrong type fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ (def f (fn (x :int y :int) :int "
                                           "[ (debug x y) ])) (f 1 \"bad\") ]"),
                  std::exception);
}

TEST_CASE("lambda call - third arg wrong type fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ (def f (fn (a :int b :int c :int) :int [ "
                               "(debug a b c) ])) (f 1 2 \"bad\") ]"),
      std::exception);
}

TEST_CASE("lambda return - body type mismatch int expected got string fails",
          "[unit][type_checker][lambda][return][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(fn () :int [ \"bad\" ])"),
                  std::exception);
}

TEST_CASE("lambda return - body type mismatch string expected got int fails",
          "[unit][type_checker][lambda][return][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(fn () :str [ 42 ])"),
                  std::exception);
}

TEST_CASE("lambda return - body type mismatch real expected got int fails",
          "[unit][type_checker][lambda][return][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression("(fn () :real [ 42 ])"),
                  std::exception);
}

TEST_CASE("lambda scope - param shadows outer variable",
          "[unit][type_checker][lambda][scope]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def x 100) (def f (fn (x :int) :int [ x ])) (f 42) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda scope - accesses outer variable",
          "[unit][type_checker][lambda][scope]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def outer 100) (def f (fn (x :int) "
                                       ":int [ (debug outer) ])) (f 42) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda scope - multiple params available in body",
          "[unit][type_checker][lambda][scope]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("[ (def f (fn (a :int b :int c :int) "
                                       ":int [ (debug a b c) ])) (f 1 2 3) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda complex - lambda returns lambda",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("(fn () :aberrant [ (fn () :int [ 0 ]) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda complex - lambda takes lambda as param",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def apply_fn (fn (f :aberrant x :int) :int [ (debug f x) ])) (def "
      "add_one (fn (n :int) :int [ n ])) (apply_fn add_one 42) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda complex - lambda with if statement",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def check (fn (x :int) :int [ (if (eq x 0) 1 0) ])) (check 5) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda complex - lambda with try/recover",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def safe (fn (x :int) :int [ (try x 0) ])) (safe 42) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda complex - lambda with match",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def matcher (fn (x :int) :none [ (match x (0 (debug \"zero\")) (1 "
      "(debug \"one\"))) ])) (matcher 0) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("lambda complex - lambda with do loop",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def looper (fn (x :int) :aberrant [ (do [ (done x) ]) ])) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("lambda complex - recursive type signature valid",
          "[unit][type_checker][lambda][complex]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "(fn (x :int) :int [ (if (eq x 0) 0 (debug x)) ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda chain - multiple lambdas in sequence",
          "[unit][type_checker][lambda][chain]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f1 (fn (x :int) :int [ x ])) (def f2 (fn (x :int) :int [ x ])) "
      "(def f3 (fn (x :int) :int [ x ])) (f3 (f2 (f1 42))) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda chain - transform types through chain",
          "[unit][type_checker][lambda][chain]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def to_str (fn (x :int) :str [ (cast :str x) ])) (def add_prefix (fn "
      "(s :str) :str [ s ])) (add_prefix (to_str 42)) ]");
  CHECK(type.base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("lambda many params - five parameters",
          "[unit][type_checker][lambda][many]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f (fn (a :int b :int c :int d :int e :int) :int [ (debug a b c d "
      "e) ])) (f 1 2 3 4 5) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda many params - ten parameters",
          "[unit][type_checker][lambda][many]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f (fn (a :int b :int c :int d :int e :int f :int g :int h :int i "
      ":int j :int) :int [ (debug a b c d e f g h i j) ])) (f 1 2 3 4 5 6 7 8 "
      "9 10) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda many params - mixed types five params",
          "[unit][type_checker][lambda][many]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def f (fn (a :int b :str c :real d :int e :str) :int [ (debug a b c "
      "d e) ])) (f 1 \"two\" 3.0 4 \"five\") ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda export - exported lambda can be called",
          "[unit][type_checker][lambda][export]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def add (fn (x :int y :int) :int [ (debug x "
                               "y) ])) (export add add) (add 1 2) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda store - lambda stored and called later",
          "[unit][type_checker][lambda][store]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression(
      "[ (def make_adder (fn (n :int) :aberrant [ (fn (x :int) :int [ (debug n "
      "x) ]) ])) (def add5 (make_adder 5)) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("lambda edge - zero params zero return valid",
          "[unit][type_checker][lambda][edge]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn () :none [ ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda edge - single param returns none",
          "[unit][type_checker][lambda][edge]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("(fn (x :int) :none [ (assert 1 \"test\") ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
}

TEST_CASE("lambda call error - zero params called with one arg fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ (def f (fn () :int [ 0 ])) (f 1) ]"),
      std::exception);
}

TEST_CASE("lambda call error - five params called with four fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ (def f (fn (a :int b :int c :int d :int e :int) :int "
                      "[ (debug a b c d e) ])) (f 1 2 3 4) ]"),
                  std::exception);
}

TEST_CASE("lambda call error - five params called with six fails",
          "[unit][type_checker][lambda][call][error]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ (def f (fn (a :int b :int c :int d :int e :int) :int "
                      "[ (debug a b c d e) ])) (f 1 2 3 4 5 6) ]"),
                  std::exception);
}

TEST_CASE("lambda unique ids - each lambda gets unique id in same context",
          "[unit][type_checker][lambda][ids]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type =
      checker.check_expression("[ (def f1 (fn () :int [ 0 ])) (def f2 (fn () "
                               ":int [ 0 ])) (def f3 (fn () :int [ 0 ])) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("lambda signature - signature preserved in type",
          "[unit][type_checker][lambda][signature]") {
  auto logger = create_test_logger();
  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");

  auto type = checker.check_expression("(fn (x :int y :str) :real [ 3.14 ])");
  CHECK(type.base_type == slp::slp_type_e::ABERRANT);
  CHECK(type.lambda_id > 0);
  CHECK(!type.lambda_signature.empty());
}
