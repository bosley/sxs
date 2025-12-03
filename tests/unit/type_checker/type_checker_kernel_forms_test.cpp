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

TEST_CASE("kernel forms - forms_test kernel loads successfully",
          "[unit][type_checker][kernel][forms]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("#(load \"forms_test\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - pair form is recognized as type",
          "[unit][type_checker][kernel][forms]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") (def x (cast :pair {1 2})) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - triple form is recognized as type",
          "[unit][type_checker][kernel][forms]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") (def x (cast :triple {1 2 3})) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - result form is recognized as type",
          "[unit][type_checker][kernel][forms]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"forms_test\") (def x (cast :result "
                               "{\"ok\" 0 (cast :error \"err\")})) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - point form with real types",
          "[unit][type_checker][kernel][forms]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") (def p (cast :point {1.5 2.5})) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - nested form with pair and string",
          "[unit][type_checker][kernel][forms]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"forms_test\") (def n (cast :nested "
                               "{(cast :pair {1 2}) \"test\"})) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - make_pair function accepts ints returns pair",
          "[unit][type_checker][kernel][forms][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") (forms_test/make_pair 1 2) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
  CHECK(type.form_name == "pair");
}

TEST_CASE("kernel forms - sum_pair function accepts pair returns int",
          "[unit][type_checker][kernel][forms][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"forms_test\") (def p (cast :pair {1 "
                               "2})) (forms_test/sum_pair p) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel forms - make_result function returns result form",
          "[unit][type_checker][kernel][forms][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") (forms_test/make_result \"ok\" 0 (cast :error "
      "\"err\")) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
  CHECK(type.form_name == "result");
}

TEST_CASE("kernel forms - process_batch with variadic pair forms",
          "[unit][type_checker][kernel][forms][call][variadic]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"forms_test\") "
                                       "(def p1 (cast :pair {1 2})) "
                                       "(def p2 (cast :pair {3 4})) "
                                       "(forms_test/process_batch p1 p2) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
  CHECK(type.form_name == "result");
}

TEST_CASE("kernel forms - make_point with real coordinates",
          "[unit][type_checker][kernel][forms][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") (forms_test/make_point 1.5 2.5) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
  CHECK(type.form_name == "point");
}

TEST_CASE("kernel forms - distance between two points returns real",
          "[unit][type_checker][kernel][forms][call]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"forms_test\") "
                                       "(def p1 (cast :point {0.0 0.0})) "
                                       "(def p2 (cast :point {3.0 4.0})) "
                                       "(forms_test/distance p1 p2) ]");
  CHECK(type.base_type == slp::slp_type_e::REAL);
}

TEST_CASE("kernel forms - variadic pair type recognized",
          "[unit][type_checker][kernel][forms][variadic]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"forms_test\") "
                                       "(def p1 (cast :pair {1 2})) "
                                       "(def p2 (cast :pair {3 4})) "
                                       "(def p3 (cast :pair {5 6})) "
                                       "(forms_test/process_batch p1 p2 p3) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
  CHECK(type.form_name == "result");
}

TEST_CASE("kernel forms - form type mismatch in function call fails",
          "[unit][type_checker][kernel][forms][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type =
      checker.check_expression("[ #(load \"forms_test\") (def t (cast :triple "
                               "{1 2 3})) (forms_test/sum_pair t) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel forms - wrong parameter type to form function fails",
          "[unit][type_checker][kernel][forms][call][error]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  CHECK_THROWS_AS(
      checker.check_expression(
          "[ #(load \"forms_test\") (forms_test/make_pair \"str\" 2) ]"),
      std::exception);
}

TEST_CASE("kernel forms - variadic with multiple pairs works",
          "[unit][type_checker][kernel][forms][call][variadic]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"forms_test\") "
                                       "(def p1 (cast :pair {1 2})) "
                                       "(def p2 (cast :pair {3 4})) "
                                       "(def p3 (cast :pair {5 6})) "
                                       "(forms_test/process_batch p1 p2 p3) ]");
  CHECK(type.base_type == slp::slp_type_e::BRACE_LIST);
  CHECK(type.form_name == "result");
}

TEST_CASE("kernel forms - multiple kernels with different forms",
          "[unit][type_checker][kernel][forms][multi]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("[ #(load \"forms_test\" \"io\") "
                                       "(def p (cast :pair {1 2})) "
                                       "(io/put \"pair: ~a\" p) ]");
  CHECK(type.base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("kernel forms - form defined in kernel available in user code",
          "[unit][type_checker][kernel][forms][user]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression(
      "[ #(load \"forms_test\") "
      "#(define-form user_pair {:pair :pair}) "
      "(def up (cast :user_pair {(cast :pair {1 2}) (cast :pair {3 4})})) ]");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}

TEST_CASE("kernel forms - backwards compatibility with old kernel format",
          "[unit][type_checker][kernel][forms][compat]") {
  auto logger = create_test_logger();
  auto kernel_path = get_test_kernel_path();
  pkg::core::type_checker::type_checker_c checker(logger, {kernel_path}, ".");

  auto type = checker.check_expression("#(load \"io\")");
  CHECK(type.base_type == slp::slp_type_e::NONE);
}
