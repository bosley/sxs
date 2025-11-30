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

std::string get_kernel_path() {
  const char *sxs_home = std::getenv("SXS_HOME");
  if (sxs_home) {
    return std::string(sxs_home) + "/lib/kernels";
  }
  return "";
}

} // namespace

TEST_CASE("tcs kernel validation - load and call alu/add with correct types",
          "[unit][tcs][kernel][alu]") {
  std::string source = R"([
    #(load "alu")
    (alu/add 1 2)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - alu/add rejects string argument",
          "[unit][tcs][kernel][alu][error]") {
  std::string source = R"([
    #(load "alu")
    (alu/add "string" 2)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - alu/add rejects wrong arity",
          "[unit][tcs][kernel][alu][error]") {
  std::string source = R"([
    #(load "alu")
    (alu/add 1)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - alu real operations",
          "[unit][tcs][kernel][alu][real]") {
  std::string source = R"([
    #(load "alu")
    (alu/add_r 1.5 2.5)
    (alu/sub_r 10.0 3.0)
    (alu/mul_r 2.0 3.0)
    (alu/div_r 10.0 2.0)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - io/put with variadic arguments",
          "[unit][tcs][kernel][io][variadic]") {
  std::string source = R"([
    #(load "io")
    (io/put "Hello %s" "world")
    (io/put "Number: %d" 42)
    (io/put "Multiple: %d %s %f" 1 "test" 3.14)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - io/put rejects non-string format",
          "[unit][tcs][kernel][io][error]") {
  std::string source = R"([
    #(load "io")
    (io/put 123 "test")
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - kv operations with correct types",
          "[unit][tcs][kernel][kv]") {
  std::string source = R"([
    #(load "kv")
    (kv/open-memory store)
    (kv/set store:key 42)
    (kv/get store:key)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - random functions",
          "[unit][tcs][kernel][random]") {
  std::string source = R"([
    #(load "random")
    (random/int_range 1 100)
    (random/real_range 0.0 1.0)
    (random/string 10)
    (random/string_alpha 5)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - random/int_range type mismatch",
          "[unit][tcs][kernel][random][error]") {
  std::string source = R"([
    #(load "random")
    (random/int_range 1.5 100.5)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - multiple kernels in same file",
          "[unit][tcs][kernel][multiple]") {
  std::string source = R"([
    #(load "alu")
    #(load "random")
    #(load "io")
    
    (def x (alu/add 10 20))
    (def r (random/int_range 1 100))
    (io/put "Result: %d" x)
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs kernel validation - kernel function return used as argument",
          "[unit][tcs][kernel][composition]") {
  std::string source = R"([
    #(load "alu")
    (def result (alu/add (alu/mul 2 3) (alu/sub 10 5)))
  ])";

  auto logger = create_test_logger();
  auto kernel_path = get_kernel_path();
  std::vector<std::string> include_paths = {kernel_path};

  pkg::core::tcs::tcs_c tcs(logger, include_paths, ".");
  CHECK(tcs.check_source(source, "test"));
}
