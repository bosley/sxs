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

std::string get_kernel_path() {
  const char *sxs_home = std::getenv("SXS_HOME");
  if (sxs_home) {
    return std::string(sxs_home) + "/lib/kernels";
  }
  return "";
}

} // namespace

TEST_CASE("tcs integration - local lambda calls kernel function",
          "[unit][tcs][integration][lambda-kernel]") {
  std::string source = R"([
    #(load "alu")
    
    (def my_add (fn (x :int y :int) :int [
        (alu/add x y)
    ]))
    
    (my_add 10 20)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - imported lambda uses kernel",
          "[unit][tcs][integration][import-kernel]") {
  std::string source = R"([
    #(import lib "test_integration_lib.sxs")
    
    (lib/compute 5 10)
    (lib/double_value 42)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {test_dir, kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - kernel result passed to lambda",
          "[unit][tcs][integration][kernel-to-lambda]") {
  std::string source = R"([
    #(load "alu")
    
    (def process (fn (val :int) :int [
        42
    ]))
    
    (def result (alu/add 10 20))
    (process result)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - kernel result passed to imported lambda",
          "[unit][tcs][integration][kernel-to-import]") {
  std::string source = R"([
    #(load "alu")
    #(import lib "test_integration_lib.sxs")
    
    (def x (alu/add 5 10))
    (lib/double_value x)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {test_dir, kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - complex type flow",
          "[unit][tcs][integration][complex]") {
  std::string source = R"([
    #(load "alu")
    #(load "random")
    #(import lib "test_integration_lib.sxs")
    
    (def local_compute (fn (a :int b :int) :int [
        (alu/add a b)
    ]))
    
    (def x (random/int_range 1 100))
    (def y (local_compute 10 20))
    (def z (lib/compute x y))
    (alu/mul z 2)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {test_dir, kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - nested lambda and kernel calls",
          "[unit][tcs][integration][nested]") {
  std::string source = R"([
    #(load "alu")
    
    (def outer (fn (x :int) :int [
        (def inner (fn (y :int) :int [
            (alu/add y 10)
        ]))
        (inner (alu/mul x 2))
    ]))
    
    (outer 5)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - variadic kernel in lambda",
          "[unit][tcs][integration][variadic]") {
  std::string source = R"([
    #(load "io")
    
    (def log_message (fn (msg :str num :int) :int [
        (io/put msg num)
    ]))
    
    (log_message "Value: %d" 42)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - type mismatch in complex flow",
          "[unit][tcs][integration][error]") {
  std::string source = R"([
    #(load "alu")
    #(import lib "test_integration_lib.sxs")
    
    (def x "string")
    (lib/compute x 10)
  ])";

  auto logger = create_test_logger();
  auto test_dir = get_test_data_dir();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {test_dir, kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, test_dir);
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - real number operations",
          "[unit][tcs][integration][real]") {
  std::string source = R"([
    #(load "alu")
    
    (def calculate (fn (x :real y :real) :real [
        (alu/add_r (alu/mul_r x 2.0) y)
    ]))
    
    (calculate 3.5 1.5)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs integration - mixed types with kv",
          "[unit][tcs][integration][kv]") {
  std::string source = R"([
    #(load "kv")
    #(load "alu")
    
    (def store_and_compute (fn (key :symbol value :int) :int [
        (kv/open-memory store)
        (kv/set key value)
        (alu/add value 10)
    ]))
    
    (store_and_compute mykey 42)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}
