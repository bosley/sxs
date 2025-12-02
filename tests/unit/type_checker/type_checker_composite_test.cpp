#include "core/context.hpp"
#include "core/instructions/datum.hpp"
#include "core/instructions/instructions.hpp"
#include <snitch/snitch.hpp>

using namespace pkg::core;

TEST_CASE("define-form basic", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  REQUIRE(context->has_form("pair"));

  auto form_def = context->get_form_definition("pair");
  REQUIRE(form_def.size() == 2);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::INTEGER);
  REQUIRE(form_def[1].base_type == slp::slp_type_e::INTEGER);

  type_info_s pair_type;
  REQUIRE(context->is_type_symbol(":pair", pair_type));
  REQUIRE(pair_type.base_type == slp::slp_type_e::BRACE_LIST);
  REQUIRE(pair_type.form_name == "pair");

  type_info_s pair_variadic_type;
  REQUIRE(context->is_type_symbol(":pair..", pair_variadic_type));
  REQUIRE(pair_variadic_type.base_type == slp::slp_type_e::BRACE_LIST);
  REQUIRE(pair_variadic_type.is_variadic == true);
}

TEST_CASE("define-form nested", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    #(define-form two {:pair :pair :str})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  REQUIRE(context->has_form("pair"));
  REQUIRE(context->has_form("two"));

  auto two_def = context->get_form_definition("two");
  REQUIRE(two_def.size() == 3);
  REQUIRE(two_def[0].base_type == slp::slp_type_e::BRACE_LIST);
  REQUIRE(two_def[0].form_name == "pair");
  REQUIRE(two_def[1].base_type == slp::slp_type_e::BRACE_LIST);
  REQUIRE(two_def[1].form_name == "pair");
  REQUIRE(two_def[2].base_type == slp::slp_type_e::DQ_LIST);
}

TEST_CASE("define-form in cast", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"(
    [
      #(define-form pair {:int :int})
      (def x 3)
      (def a (cast :pair {1 x}))
    ]
  )";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("define-form in function signature", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"(
    [
      #(define-form pair {:int :int})
      (def process (fn (p :pair) :list-c [
        p
      ]))
    ]
  )";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("define-form invalid type", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"([
    #(define-form bad {:invalid :int})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  CHECK_THROWS_AS(context->eval_type(obj), std::exception);
}

TEST_CASE("define-form not brace list", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"([
    #(define-form bad (:int :int))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  CHECK_THROWS_AS(context->eval_type(obj), std::exception);
}

TEST_CASE("define-form variadic type", "[type_checker][composite]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context =
      create_compiler_context(logger, include_paths, working_directory,
                              instructions_map, nullptr, nullptr);

  std::string source = R"(
    [
      #(define-form pair {:int :int})
      (def process (fn (pairs :pair..) :int [
        42
      ]))
    ]
  )";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}
