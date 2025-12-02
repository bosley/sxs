#include <core/type_checker/type_checker.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

namespace {

pkg::core::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

std::string get_test_kernel_path() { return std::string(TEST_KERNEL_DIR); }

} // namespace

TEST_CASE("kernel load - io kernel loads and returns none",
          "[unit][type_checker][kernel][io]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("#(load \"io\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel load - forge kernel loads and returns none",
          "[unit][type_checker][kernel][forge]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("#(load \"forge\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel load - math kernel loads and returns none",
          "[unit][type_checker][kernel][math]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("#(load \"math\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel load - multiple kernels",
          "[unit][type_checker][kernel][multi]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("#(load \"io\" \"forge\" \"math\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - io/put with string format returns int",
          "[unit][type_checker][kernel][io][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"io\") (io/put \"format\" 1) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - io/put variadic with multiple objects",
          "[unit][type_checker][kernel][io][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"io\") (io/put \"format\" 1 2 3 \"test\") ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - io/put without format fails",
          "[unit][type_checker][kernel][io][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ #(load \"io\") (io/put) ]"),
                  std::exception);
}

TEST_CASE("kernel call - io/put with non-string format fails",
          "[unit][type_checker][kernel][io][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"io\") (io/put 123 \"test\") ]"),
      std::exception);
}

TEST_CASE("kernel call - math/add with two ints returns int",
          "[unit][type_checker][kernel][math][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"math\") (math/add 10 20) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - math/sub returns int",
          "[unit][type_checker][kernel][math][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"math\") (math/sub 100 25) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - math/mul returns int",
          "[unit][type_checker][kernel][math][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"math\") (math/mul 5 6) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - math/div returns int",
          "[unit][type_checker][kernel][math][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"math\") (math/div 20 4) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - math/add with wrong arg count fails",
          "[unit][type_checker][kernel][math][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"math\") (math/add 10) ]"),
      std::exception);
}

TEST_CASE("kernel call - math/add with too many args fails",
          "[unit][type_checker][kernel][math][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"math\") (math/add 1 2 3) ]"),
      std::exception);
}

TEST_CASE("kernel call - math/add with non-int first arg fails",
          "[unit][type_checker][kernel][math][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"math\") (math/add \"bad\" 2) ]"),
      std::exception);
}

TEST_CASE("kernel call - math/add with non-int second arg fails",
          "[unit][type_checker][kernel][math][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"math\") (math/add 1 \"bad\") ]"),
      std::exception);
}

TEST_CASE("kernel call - forge/count returns int",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/count list) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - forge/pf returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/pf list 4) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/pb returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/pb list 4) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/rf returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/rf list) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/rb returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/rb list) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/lsh with int count returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/lsh list 2) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/rsh with int count returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/rsh list 2) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/rotr returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/rotr list 1) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/rotl returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/rotl list 1) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/rev returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/rev list) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/concat returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def a {1 2}) (def b {3 4}) (forge/concat a b) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/replace returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (forge/replace {1 2 3} 2 5) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/drop_match returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (forge/drop_match {1 2 3 2} 2) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/drop_period returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (forge/drop_period {1 2 3 4 5} 0 2) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/to_bits returns list-c",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"forge\") (forge/to_bits 42) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
}

TEST_CASE("kernel call - forge/from_bits returns int",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def bits {0 1 0 1}) (forge/from_bits bits) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel call - forge/to_bits_r returns list-c",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"forge\") (forge/to_bits_r 3.14) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
}

TEST_CASE("kernel call - forge/from_bits_r returns real",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def bits {0 1 0 1}) (forge/from_bits_r bits) ]");
  CHECK(type.base_type == slp::slp_type_e::REAL);
}

TEST_CASE("kernel call - forge/resize with correct types returns any",
          "[unit][type_checker][kernel][forge][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forge\") (def list {1 2 3}) (forge/resize list 5 0) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel call - forge/resize with non-int size fails",
          "[unit][type_checker][kernel][forge][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ #(load \"forge\") (forge/resize {1 2} \"bad\" 0) ]"),
                  std::exception);
}

TEST_CASE("kernel call - forge/lsh with non-int count fails",
          "[unit][type_checker][kernel][forge][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ #(load \"forge\") (forge/lsh {1 2} \"bad\") ]"),
                  std::exception);
}

TEST_CASE("kernel call - forge/rsh with non-int count fails",
          "[unit][type_checker][kernel][forge][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"forge\") (forge/rsh {1 2} 3.14) ]"),
      std::exception);
}

TEST_CASE("kernel call - forge/from_bits with non-list fails",
          "[unit][type_checker][kernel][forge][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"forge\") (forge/from_bits 42) ]"),
      std::exception);
}

TEST_CASE("kernel call - forge/to_bits with non-int fails",
          "[unit][type_checker][kernel][forge][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression("[ #(load \"forge\") (forge/to_bits \"bad\") ]"),
      std::exception);
}

TEST_CASE("kernel call - forge/to_bits_r with non-real fails",
          "[unit][type_checker][kernel][forge][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(checker.check_expression(
                      "[ #(load \"forge\") (forge/to_bits_r \"bad\") ]"),
                  std::exception);
}

TEST_CASE("kernel complex - multiple kernel functions in sequence",
          "[unit][type_checker][kernel][complex]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"math\" \"forge\") (def a (math/add 5 10)) (def b (math/mul "
      "a 2)) (def list {1 2 3}) (def len (forge/count list)) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel complex - kernel function with variables",
          "[unit][type_checker][kernel][complex]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"math\") (def x 10) (def y 20) (def sum (math/add x y)) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel complex - kernel function in lambda",
          "[unit][type_checker][kernel][complex]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"math\") (def adder (fn (x :int y :int) :int [ (math/add x "
      "y) ])) (adder 5 10) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel complex - kernel function in if",
          "[unit][type_checker][kernel][complex]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"math\") (if 1 (math/add 1 2) (math/sub 5 3)) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel complex - nested kernel calls",
          "[unit][type_checker][kernel][complex]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"math\") (math/add (math/mul 2 3) (math/sub 10 5)) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel error - function not loaded fails",
          "[unit][type_checker][kernel][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(checker.check_expression("[ (math/add 1 2) ]"),
                  std::exception);
}

TEST_CASE("kernel error - nonexistent kernel fails",
          "[unit][type_checker][kernel][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(checker.check_expression("#(load \"nonexistent_kernel\")"),
                  std::exception);
}
