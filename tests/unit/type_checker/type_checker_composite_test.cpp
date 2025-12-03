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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

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

TEST_CASE("form mixed primitive types",
          "[type_checker][composite][validation]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form mixed {:int :str :real :symbol})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("mixed");
  REQUIRE(form_def.size() == 4);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::INTEGER);
  REQUIRE(form_def[1].base_type == slp::slp_type_e::DQ_LIST);
  REQUIRE(form_def[2].base_type == slp::slp_type_e::REAL);
  REQUIRE(form_def[3].base_type == slp::slp_type_e::SYMBOL);
}

TEST_CASE("form with list types", "[type_checker][composite][validation]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form container {:list-p :list-b :list-c})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("container");
  REQUIRE(form_def.size() == 3);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::PAREN_LIST);
  REQUIRE(form_def[1].base_type == slp::slp_type_e::BRACKET_LIST);
  REQUIRE(form_def[2].base_type == slp::slp_type_e::BRACE_LIST);
}

TEST_CASE("form with complex types", "[type_checker][composite][validation]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form complex {:some :error :datum})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("complex");
  REQUIRE(form_def.size() == 3);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::SOME);
  REQUIRE(form_def[1].base_type == slp::slp_type_e::ERROR);
  REQUIRE(form_def[2].base_type == slp::slp_type_e::DATUM);
}

TEST_CASE("form assignment to list-c compatible",
          "[type_checker][composite][validation]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2}))
    (def y (cast :list-c x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form multiple parameters", "[type_checker][composite][function]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    #(define-form triple {:int :int :int})
    (def combine (fn (p :pair t :triple) :int [
      42
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form return type", "[type_checker][composite][function]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def make_pair (fn (a :int b :int) :pair [
      (cast :pair {a b})
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form variadic parameters", "[type_checker][composite][function]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form point {:int :int})
    (def process_points (fn (points :point..) :int [
      42
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form with lambda type", "[type_checker][composite][lambda]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form callback {:aberrant :int})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("callback");
  REQUIRE(form_def.size() == 2);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::ABERRANT);
  REQUIRE(form_def[1].base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("lambda returning form", "[type_checker][composite][lambda]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def factory (fn (x :int) :pair [
      (cast :pair {x x})
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("lambda taking form", "[type_checker][composite][lambda]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def process (fn (p :pair) :int [
      42
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form in if branches", "[type_checker][composite][control_flow]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (if 1 
      (cast :pair {1 2})
      (cast :pair {3 4})
    ))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form in try handler", "[type_checker][composite][control_flow]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (try 
      (cast :pair {1 2})
      (cast :pair {0 0})
    ))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("empty form", "[type_checker][composite][edge_case]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form empty {})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("empty");
  REQUIRE(form_def.size() == 0);
}

TEST_CASE("single element form", "[type_checker][composite][edge_case]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form single {:int})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("single");
  REQUIRE(form_def.size() == 1);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("large form", "[type_checker][composite][edge_case]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form large {:int :str :real :int :str :real :int :str :real :int :str :real})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("large");
  REQUIRE(form_def.size() == 12);
}

TEST_CASE("deeply nested forms", "[type_checker][composite][edge_case]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form level1 {:int :int})
    #(define-form level2 {:level1 :str})
    #(define-form level3 {:level2 :real})
    #(define-form level4 {:level3 :symbol})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  REQUIRE(context->has_form("level1"));
  REQUIRE(context->has_form("level2"));
  REQUIRE(context->has_form("level3"));
  REQUIRE(context->has_form("level4"));

  auto level4_def = context->get_form_definition("level4");
  REQUIRE(level4_def.size() == 2);
  REQUIRE(level4_def[0].base_type == slp::slp_type_e::BRACE_LIST);
  REQUIRE(level4_def[0].form_name == "level3");
}

TEST_CASE("form with any type", "[type_checker][composite][edge_case]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form flexible {:any :int})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("flexible");
  REQUIRE(form_def.size() == 2);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::NONE);
  REQUIRE(form_def[1].base_type == slp::slp_type_e::INTEGER);
}

TEST_CASE("form forward reference fails", "[type_checker][composite][scope]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form uses_undefined {:undefined :int})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  CHECK_THROWS_AS(context->eval_type(obj), std::exception);
}

TEST_CASE("form visibility in nested scopes",
          "[type_checker][composite][scope]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def outer (fn () :pair [
      (def inner (fn () :pair [
        (cast :pair {1 2})
      ]))
      (inner)
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("cast list-c to form validates", "[type_checker][composite][cast]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x {1 2})
    (def y (cast :pair x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("cast form to list-c is noop", "[type_checker][composite][cast]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2}))
    (def y (cast :list-c x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("cast form to different form", "[type_checker][composite][cast]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    #(define-form point {:int :int})
    (def x (cast :pair {1 2}))
    (def y (cast :point x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("cast nested forms", "[type_checker][composite][cast]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form inner {:int :int})
    #(define-form outer {:inner :str})
    (def x (cast :inner {1 2}))
    (def y (cast :outer {x "test"}))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("cast with wrong element count", "[type_checker][composite][cast]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2 3}))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form in match pattern", "[type_checker][composite][control_flow]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (cast :pair {1 2}))
    (def result (match x
      ((cast :pair {1 2}) 100)
      ((cast :pair {3 4}) 200)
    ))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form in recover", "[type_checker][composite][control_flow]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def x (recover 
      [(cast :pair {1 2})]
      [(cast :pair {0 0})]
    ))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("nested function calls with forms",
          "[type_checker][composite][function]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def make (fn (x :int) :pair [
      (cast :pair {x x})
    ]))
    (def process (fn (p :pair) :int [
      42
    ]))
    (def result (process (make 5)))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form parameter accepts list-c",
          "[type_checker][composite][function]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def process (fn (p :pair) :int [42]))
    (def x {1 2})
    (def result (process x))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}

TEST_CASE("form with variadic list type",
          "[type_checker][composite][validation]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form container {:int.. :str})
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));

  auto form_def = context->get_form_definition("container");
  REQUIRE(form_def.size() == 2);
  REQUIRE(form_def[0].base_type == slp::slp_type_e::INTEGER);
  REQUIRE(form_def[0].is_variadic == true);
}

TEST_CASE("form in do loop", "[type_checker][composite][control_flow]") {
  auto logger = spdlog::default_logger();
  std::vector<std::string> include_paths;
  std::string working_directory = ".";

  auto instructions_map = instructions::get_standard_callable_symbols();
  auto datum_map = datum::get_standard_callable_symbols();
  instructions_map.insert(datum_map.begin(), datum_map.end());

  auto context = create_compiler_context(
      logger, include_paths, working_directory, instructions_map, nullptr);

  std::string source = R"([
    #(define-form pair {:int :int})
    (def result (do [
      (def x (cast :pair {1 2}))
      (done x)
    ]))
  ])";

  auto parse_result = slp::parse(source);
  REQUIRE(!parse_result.is_error());

  auto obj = parse_result.take();
  REQUIRE_NOTHROW(context->eval_type(obj));
}
