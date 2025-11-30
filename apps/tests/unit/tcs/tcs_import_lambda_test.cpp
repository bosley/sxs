#include <core/tcs/tcs.hpp>
#include <filesystem>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

namespace {

pkg::core::logger_t create_test_logger() {
  auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  return std::make_shared<spdlog::logger>("test", sink);
}

std::string get_test_data_dir() { return TEST_DATA_DIR; }

} // namespace

TEST_CASE("tcs import lambda - import and call with correct types",
          "[unit][tcs][import][lambda]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    (math/add_numbers 10 20)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - reject wrong argument type",
          "[unit][tcs][import][lambda][error]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    (math/add_numbers "string" 20)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - reject wrong arity",
          "[unit][tcs][import][lambda][error]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    (math/add_numbers 10)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - multiple functions from same import",
          "[unit][tcs][import][lambda][multiple]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    (math/add_numbers 10 20)
    (math/multiply 5 6)
    (math/greet "world")
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - multiple imports",
          "[unit][tcs][import][lambda][multiple-imports]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    #(import utils "test_import_lambda_b.sxs")
    
    (math/add_numbers 10 20)
    (utils/process_value 42)
    (utils/format_string "test" 100)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - nested imports",
          "[unit][tcs][import][lambda][nested]") {
  std::string source = R"([
    #(import lib_a "test_import_nested_a.sxs")
    (lib_a/wrapper 100)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - call result used in local lambda",
          "[unit][tcs][import][lambda][composition]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    
    (def process (fn (x :int) :int [
        (math/add_numbers x 10)
    ]))
    
    (process 5)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - string parameter validation",
          "[unit][tcs][import][lambda][string]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    (math/greet "Alice")
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - string parameter type mismatch",
          "[unit][tcs][import][lambda][string][error]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    (math/greet 123)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs import lambda - pass imported lambda result to another",
          "[unit][tcs][import][lambda][chaining]") {
  std::string source = R"([
    #(import math "test_import_lambda_a.sxs")
    #(import utils "test_import_lambda_b.sxs")
    
    (def result (math/add_numbers 10 20))
    (utils/process_value result)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  std::vector<std::string> include_paths = {test_dir};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}
