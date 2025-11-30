#include <core/tcs/tcs.hpp>
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

} // namespace

TEST_CASE("tcs imports - export function", "[unit][tcs][imports][export]") {
  auto source = load_test_file("test_import_exported.sxs");
  auto logger = create_test_logger();

  std::string test_data_dir = TEST_DATA_DIR;
  pkg::core::tcs::tcs_c tcs(logger, {test_data_dir}, test_data_dir);
  CHECK(tcs.check_source(source, "test_import_exported.sxs"));
}

TEST_CASE("tcs imports - simple export", "[unit][tcs][imports][export]") {
  std::string source = R"([
    (export add (fn (a :int b :int) :int [
      42
    ]))
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs imports - export with invalid symbol",
          "[unit][tcs][imports][export][error]") {
  std::string source = R"([
    (export 123 42)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs imports - import missing file",
          "[unit][tcs][imports][import][error]") {
  std::string source = R"([
    #(import missing "nonexistent.sxs")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs imports - import with invalid arguments",
          "[unit][tcs][imports][import][error]") {
  std::string source = R"([
    #(import)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs imports - import with non-symbol name",
          "[unit][tcs][imports][import][error]") {
  std::string source = R"([
    #(import "not-symbol" "file.sxs")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs imports - import with non-string path",
          "[unit][tcs][imports][import][error]") {
  std::string source = R"([
    #(import myimport 123)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}
