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

TEST_CASE("tcs basic types - parse and type check literals",
          "[unit][tcs][basic]") {
  auto source = load_test_file("test_basic_types.sxs");
  auto logger = create_test_logger();

  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test_basic_types.sxs"));
}

TEST_CASE("tcs basic types - integer definition", "[unit][tcs][basic][int]") {
  std::string source = R"([
    (def x 42)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs basic types - real definition", "[unit][tcs][basic][real]") {
  std::string source = R"([
    (def pi 3.14159)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs basic types - string definition", "[unit][tcs][basic][string]") {
  std::string source = R"([
    (def greeting "hello world")
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs basic types - symbol definition", "[unit][tcs][basic][symbol]") {
  std::string source = R"([
    (def my-sym test-symbol)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs basic types - multiple definitions",
          "[unit][tcs][basic][multi]") {
  std::string source = R"([
    (def a 1)
    (def b 2.0)
    (def c "three")
    (def d four)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}

TEST_CASE("tcs basic types - redefinition in same scope fails",
          "[unit][tcs][basic][error]") {
  std::string source = R"([
    (def x 1)
    (def x 2)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK_FALSE(tcs.check_source(source, "test"));
}

TEST_CASE("tcs basic types - debug with multiple types",
          "[unit][tcs][basic][debug]") {
  std::string source = R"([
    (def x 42)
    (def y 3.14)
    (def s "test")
    (debug x y s)
  ])";

  auto logger = create_test_logger();
  pkg::core::tcs::tcs_c tcs(logger, {}, ".");
  CHECK(tcs.check_source(source, "test"));
}
