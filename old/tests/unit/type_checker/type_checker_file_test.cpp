#include <core/type_checker/type_checker.hpp>
#include <fstream>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
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

TEST_CASE("type_checker file test - test_basic_types.sxs",
          "[unit][type_checker][file]") {
  auto source = load_test_file("test_basic_types.sxs");
  auto logger = create_test_logger();

  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");
  CHECK(checker.check_source(source, "test_basic_types.sxs"));
}

TEST_CASE("type_checker file test - test_functions.sxs",
          "[unit][type_checker][file]") {
  auto source = load_test_file("test_functions.sxs");
  auto logger = create_test_logger();

  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");
  CHECK(checker.check_source(source, "test_functions.sxs"));
}

TEST_CASE("type_checker file test - test_control_flow.sxs",
          "[unit][type_checker][file]") {
  auto source = load_test_file("test_control_flow.sxs");
  auto logger = create_test_logger();

  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");
  CHECK(checker.check_source(source, "test_control_flow.sxs"));
}

TEST_CASE("type_checker file test - test_error_handling.sxs",
          "[unit][type_checker][file]") {
  auto source = load_test_file("test_error_handling.sxs");
  auto logger = create_test_logger();

  pkg::core::type_checker::type_checker_c checker(logger, {}, ".");
  CHECK(checker.check_source(source, "test_error_handling.sxs"));
}
