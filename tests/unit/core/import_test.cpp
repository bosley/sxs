#include <core/core.hpp>
#include <core/imports/imports.hpp>
#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <core/kernels/kernels.hpp>
#include <filesystem>
#include <fstream>
#include <slp/slp.hpp>
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

std::unique_ptr<pkg::core::callable_context_if>
create_test_interpreter(pkg::core::imports::imports_manager_c &imports_manager,
                        pkg::core::kernels::kernel_manager_c &kernel_manager) {
  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(
      symbols, &imports_manager.get_import_context(),
      &kernel_manager.get_kernel_context());
  imports_manager.set_parent_context(interpreter.get());
  return interpreter;
}

} // namespace

TEST_CASE("import - basic import and function call",
          "[unit][core][import][basic]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  pkg::core::imports::imports_manager_c imports_manager(logger, include_paths,
                                                        working_dir);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_basic.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter = create_test_interpreter(imports_manager, kernel_manager);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("testlib/greet"));
}

TEST_CASE("import - lambda export", "[unit][core][import][lambda]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  pkg::core::imports::imports_manager_c imports_manager(logger, include_paths,
                                                        working_dir);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_lambda.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter = create_test_interpreter(imports_manager, kernel_manager);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  CHECK(interpreter->has_symbol("mathlib/add"));
  CHECK(interpreter->has_symbol("mathlib/multiply"));
  CHECK(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 8);
}

TEST_CASE("import - value exports", "[unit][core][import][values]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  pkg::core::imports::imports_manager_c imports_manager(logger, include_paths,
                                                        working_dir);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_values.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter = create_test_interpreter(imports_manager, kernel_manager);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));

  CHECK(interpreter->has_symbol("config/version"));
  CHECK(interpreter->has_symbol("config/max_count"));

  auto version_parsed = slp::parse("config/version");
  REQUIRE(version_parsed.is_success());
  auto version_obj = version_parsed.take();
  auto version_val = interpreter->eval(version_obj);
  CHECK(version_val.type() == slp::slp_type_e::INTEGER);
  CHECK(version_val.as_int() == 100);
}

TEST_CASE("import - multiple exports", "[unit][core][import][multiple]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  pkg::core::imports::imports_manager_c imports_manager(logger, include_paths,
                                                        working_dir);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_lambda.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter = create_test_interpreter(imports_manager, kernel_manager);

  auto obj = parse_result.take();
  interpreter->eval(obj);

  CHECK(interpreter->has_symbol("mathlib/add"));
  CHECK(interpreter->has_symbol("mathlib/multiply"));

  auto call_source = R"([(mathlib/add 5 3)])";
  auto call_parsed = slp::parse(call_source);
  REQUIRE(call_parsed.is_success());
  auto call_obj = call_parsed.take();
  auto result = interpreter->eval(call_obj);
  CHECK(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 8);
}

TEST_CASE("import - locked after first instruction",
          "[unit][core][import][locking]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  pkg::core::imports::imports_manager_c imports_manager(logger, include_paths,
                                                        working_dir);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  std::string source = R"([
    (def x 42)
    #(import testlib "test_import_exported_basic.sxs")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter = create_test_interpreter(imports_manager, kernel_manager);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}
