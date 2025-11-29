#include <core/core.hpp>
#include <core/imports/imports.hpp>
#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <core/kernels/kernels.hpp>
#include <filesystem>
#include <fstream>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <sxs/slp/slp.hpp>

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

std::unique_ptr<pkg::core::callable_context_if> create_test_interpreter(
    pkg::core::imports::imports_manager_c &imports_manager,
    pkg::core::kernels::kernel_manager_c &kernel_manager,
    std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
        *import_interpreters,
    std::map<std::string, std::shared_mutex> *import_interpreter_locks) {
  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(
      symbols, &imports_manager.get_import_context(),
      &kernel_manager.get_kernel_context(), import_interpreters,
      import_interpreter_locks);
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

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_basic.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("import - lambda export", "[unit][core][import][lambda]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_lambda.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  auto result = interpreter->eval(obj);

  CHECK(result.type() == slp::slp_type_e::INTEGER);
  CHECK(result.as_int() == 8);
}

TEST_CASE("import - value exports", "[unit][core][import][values]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_values.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("import - multiple exports", "[unit][core][import][multiple]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_lambda.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  interpreter->eval(obj);

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

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  std::string source = R"([
    (def x 42)
    #(import testlib "test_import_exported_basic.sxs")
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  CHECK_THROWS_AS(interpreter->eval(obj), std::runtime_error);
}

TEST_CASE("import - direct circular import", "[unit][core][import][circular]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_circular_a.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  bool exception_caught = false;
  std::string exception_message;
  try {
    interpreter->eval(obj);
  } catch (const std::runtime_error &e) {
    exception_caught = true;
    exception_message = e.what();
  }

  REQUIRE(exception_caught);
  CHECK(exception_message.find("failed to import") != std::string::npos);
}

TEST_CASE("import - indirect circular import (3-way)",
          "[unit][core][import][circular]") {
  auto logger = std::make_shared<spdlog::logger>(
      "test", std::make_shared<spdlog::sinks::null_sink_mt>());

  std::vector<std::string> include_paths = {TEST_DATA_DIR};
  std::string working_dir = TEST_DATA_DIR;

  std::map<std::string, std::unique_ptr<pkg::core::callable_context_if>>
      import_interpreters;
  std::map<std::string, std::shared_mutex> import_interpreter_locks;
  pkg::core::imports::imports_manager_c imports_manager(
      logger, include_paths, working_dir, &import_interpreters,
      &import_interpreter_locks);
  pkg::core::kernels::kernel_manager_c kernel_manager(logger, include_paths,
                                                      working_dir);

  auto source = load_test_file("test_import_circular_3way_a.sxs");
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());

  auto interpreter =
      create_test_interpreter(imports_manager, kernel_manager,
                              &import_interpreters, &import_interpreter_locks);

  auto obj = parse_result.take();
  bool exception_caught = false;
  std::string exception_message;
  try {
    interpreter->eval(obj);
  } catch (const std::runtime_error &e) {
    exception_caught = true;
    exception_message = e.what();
  }

  REQUIRE(exception_caught);
  CHECK(exception_message.find("failed to import") != std::string::npos);
}
