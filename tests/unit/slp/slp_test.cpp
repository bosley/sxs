#include <slp/buffer.hpp>
#include <slp/slp.hpp>
#include <snitch/snitch.hpp>

class slp_test_accessor {
public:
  static const slp::slp_buffer_c &get_data(const slp::slp_object_c &obj) {
    return obj.data_;
  }

  static const std::map<std::uint64_t, std::string> &
  get_symbols(const slp::slp_object_c &obj) {
    return obj.symbols_;
  }

  static const slp::slp_unit_of_store_t *
  get_view(const slp::slp_object_c &obj) {
    return obj.view_;
  }

  static size_t get_root_offset(const slp::slp_object_c &obj) {
    return obj.root_offset_;
  }
};

TEST_CASE("slp parse integers", "[unit][slp][integer]") {
  auto result = slp::parse("42");
  CHECK(result.is_success());
  CHECK(result.object().type() == slp::slp_type_e::INTEGER);
  CHECK(result.object().as_int() == 42);
}

TEST_CASE("slp parse negative integers", "[unit][slp][integer]") {
  auto result = slp::parse("-123");
  CHECK(result.is_success());
  CHECK(result.object().type() == slp::slp_type_e::INTEGER);
  CHECK(result.object().as_int() == -123);
}

TEST_CASE("slp parse reals", "[unit][slp][real]") {
  SECTION("decimal notation") {
    auto result = slp::parse("3.14");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::REAL);
    CHECK(result.object().as_real() == 3.14);
  }

  SECTION("scientific notation") {
    auto result = slp::parse("1.23e10");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::REAL);
    CHECK(result.object().as_real() == 1.23e10);
  }

  SECTION("negative scientific notation") {
    auto result = slp::parse("-5.67e-3");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::REAL);
    CHECK(result.object().as_real() == -5.67e-3);
  }
}

TEST_CASE("slp parse symbols", "[unit][slp][symbol]") {
  SECTION("simple symbol") {
    auto result = slp::parse("hello");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SYMBOL);
    CHECK(std::string(result.object().as_symbol()) == "hello");
  }

  SECTION("symbol with special chars") {
    auto result = slp::parse("my-symbol");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SYMBOL);
    CHECK(std::string(result.object().as_symbol()) == "my-symbol");
  }
}

TEST_CASE("slp parse paren lists", "[unit][slp][list]") {
  SECTION("empty list") {
    auto result = slp::parse("()");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::PAREN_LIST);
  }

  SECTION("list with integers") {
    auto result = slp::parse("(1 2 3)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::PAREN_LIST);
  }

  SECTION("nested lists") {
    auto result = slp::parse("(1 (2 3) 4)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::PAREN_LIST);
  }

  SECTION("list with mixed types") {
    auto result = slp::parse("(1 hello 3.14)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::PAREN_LIST);
  }
}

TEST_CASE("slp parse environments", "[unit][slp][list]") {
  SECTION("empty environment") {
    auto result = slp::parse("[]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
  }

  SECTION("environment with content") {
    auto result = slp::parse("[1 2 3]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
  }
}

TEST_CASE("slp parse brace lists", "[unit][slp][list]") {
  SECTION("empty brace list") {
    auto result = slp::parse("{}");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACE_LIST);
  }

  SECTION("brace list with content") {
    auto result = slp::parse("{a b}");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACE_LIST);
  }
}

TEST_CASE("slp parse strings", "[unit][slp][string]") {
  SECTION("empty string") {
    auto result = slp::parse("\"\"");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::DQ_LIST);
  }

  SECTION("simple string") {
    auto result = slp::parse("\"hello world\"");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::DQ_LIST);
  }
}

TEST_CASE("slp parse comments", "[unit][slp][comment]") {
  SECTION("comment only") {
    auto result = slp::parse("; just a comment\n42");
    CHECK(result.is_success());
  }

  SECTION("comment with code") {
    auto result = slp::parse("(1 ; middle comment\n 2)");
    CHECK(result.is_success());
  }

  SECTION("multiple comments") {
    auto result = slp::parse("; first\n; second\n(42)");
    CHECK(result.is_success());
  }
}

TEST_CASE("slp parse quoted objects", "[unit][slp][quote]") {
  SECTION("quoted list") {
    auto result = slp::parse("'(1 2 3)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }

  SECTION("quoted symbol") {
    auto result = slp::parse("'hello");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }
}

TEST_CASE("slp parse error objects", "[unit][slp][error]") {
  SECTION("error with integer") {
    auto result = slp::parse("@42");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("error with symbol") {
    auto result = slp::parse("@not-found");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("error with list") {
    auto result = slp::parse("@(division by zero)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("error with string") {
    auto result = slp::parse("@\"file not found\"");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("multiple errors") {
    auto result = slp::parse("@@nested-error");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }
}

TEST_CASE("slp parse environment", "[unit][slp][env]") {
  SECTION("simple environment") {
    auto result = slp::parse("[my-program (let a 3)]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
  }

  SECTION("environment with multiple expressions") {
    auto result = slp::parse("[test (let a 3) (putln a)]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
  }

  SECTION("empty environment") {
    auto result = slp::parse("[]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
  }
}

TEST_CASE("slp parse complex nested structures", "[unit][slp][complex]") {
  SECTION("deeply nested") {
    auto result = slp::parse("(1 (2 (3 (4 5))))");
    CHECK(result.is_success());
  }

  SECTION("mixed list types") {
    auto result = slp::parse("(a [b {c d}] e)");
    CHECK(result.is_success());
  }
}

TEST_CASE("slp parse whitespace handling", "[unit][slp][whitespace]") {
  SECTION("multiple spaces") {
    auto result = slp::parse("(1    2    3)");
    CHECK(result.is_success());
  }

  SECTION("newlines") {
    auto result = slp::parse("(1\n2\n3)");
    CHECK(result.is_success());
  }

  SECTION("mixed whitespace") {
    auto result = slp::parse("(1 \n\t 2   \n 3)");
    CHECK(result.is_success());
  }
}

TEST_CASE("slp parse errors - unclosed lists", "[unit][slp][error]") {
  SECTION("unclosed paren") {
    auto result = slp::parse("(1 2 3");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_PAREN_LIST);
  }

  SECTION("unclosed environment") {
    auto result = slp::parse("[1 2 3");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_BRACKET_LIST);
  }

  SECTION("unclosed brace") {
    auto result = slp::parse("{1 2 3");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_BRACE_LIST);
  }

  SECTION("unclosed string") {
    auto result = slp::parse("\"hello");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_DQ_LIST);
  }
}

TEST_CASE("slp parse example.slp structure", "[unit][slp][example]") {
  std::string example = R"(
[my-program
    (let a 3)
    (putln a)
]
)";
  auto result = slp::parse(example);
  CHECK(result.is_success());
}

TEST_CASE("slp parse complex example", "[unit][slp][example]") {
  std::string example = R"(
[my-program
    (let a [my-sub-env
        (let my_data 0)
    ])
    (let something {a my_data})
    (let a '(1 2 3))
]
)";
  auto result = slp::parse(example);
  CHECK(result.is_success());
}

TEST_CASE("slp list operations - basic", "[unit][slp][list][interface]") {
  SECTION("empty list") {
    auto result = slp::parse("()");
    CHECK(result.is_success());
    auto list = result.object().as_list();
    CHECK(list.empty());
    CHECK(list.size() == 0);
  }

  SECTION("list with integers") {
    auto result = slp::parse("(1 2 3)");
    CHECK(result.is_success());
    auto list = result.object().as_list();
    CHECK(!list.empty());
    CHECK(list.size() == 3);

    auto first = list.at(0);
    CHECK(first.type() == slp::slp_type_e::INTEGER);
    CHECK(first.as_int() == 1);

    auto second = list.at(1);
    CHECK(second.type() == slp::slp_type_e::INTEGER);
    CHECK(second.as_int() == 2);

    auto third = list.at(2);
    CHECK(third.type() == slp::slp_type_e::INTEGER);
    CHECK(third.as_int() == 3);
  }

  SECTION("list with mixed types") {
    auto result = slp::parse("(42 hello 3.14)");
    CHECK(result.is_success());
    auto list = result.object().as_list();
    CHECK(list.size() == 3);

    auto elem0 = list.at(0);
    CHECK(elem0.type() == slp::slp_type_e::INTEGER);
    CHECK(elem0.as_int() == 42);

    auto elem1 = list.at(1);
    CHECK(elem1.type() == slp::slp_type_e::SYMBOL);
    CHECK(std::string(elem1.as_symbol()) == "hello");

    auto elem2 = list.at(2);
    CHECK(elem2.type() == slp::slp_type_e::REAL);
    CHECK(elem2.as_real() == 3.14);
  }
}

TEST_CASE("slp list operations - nested", "[unit][slp][list][interface]") {
  auto result = slp::parse("(1 (2 3) 4)");
  CHECK(result.is_success());

  auto outer_list = result.object().as_list();
  CHECK(outer_list.size() == 3);

  auto first = outer_list.at(0);
  CHECK(first.type() == slp::slp_type_e::INTEGER);
  CHECK(first.as_int() == 1);

  auto nested = outer_list.at(1);
  CHECK(nested.type() == slp::slp_type_e::PAREN_LIST);
  auto inner_list = nested.as_list();
  CHECK(inner_list.size() == 2);
  CHECK(inner_list.at(0).as_int() == 2);
  CHECK(inner_list.at(1).as_int() == 3);

  auto third = outer_list.at(2);
  CHECK(third.type() == slp::slp_type_e::INTEGER);
  CHECK(third.as_int() == 4);
}

TEST_CASE("slp list operations - bracket and brace",
          "[unit][slp][list][interface]") {
  SECTION("environment") {
    auto result = slp::parse("[1 2 3]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
    auto list = result.object().as_list();
    CHECK(list.size() == 3);
    CHECK(list.at(0).as_int() == 1);
  }

  SECTION("brace list") {
    auto result = slp::parse("{a b c}");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACE_LIST);
    auto list = result.object().as_list();
    CHECK(list.size() == 3);
    CHECK(std::string(list.at(0).as_symbol()) == "a");
  }

  SECTION("environment is a list") {
    auto result = slp::parse("[test (let a 1) (let b 2)]");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
    auto list = result.object().as_list();
    CHECK(list.size() == 3);
    CHECK(list.at(0).type() == slp::slp_type_e::SYMBOL);
    CHECK(std::string(list.at(0).as_symbol()) == "test");
  }
}

TEST_CASE("slp list operations - invalid type",
          "[unit][slp][list][interface]") {
  SECTION("integer is not a list") {
    auto result = slp::parse("42");
    CHECK(result.is_success());
    auto list = result.object().as_list();
    CHECK(list.empty());
    CHECK(list.size() == 0);
  }

  SECTION("symbol is not a list") {
    auto result = slp::parse("hello");
    CHECK(result.is_success());
    auto list = result.object().as_list();
    CHECK(list.empty());
  }
}

TEST_CASE("slp string operations - basic", "[unit][slp][string][interface]") {
  SECTION("empty string") {
    auto result = slp::parse("\"\"");
    CHECK(result.is_success());
    auto str = result.object().as_string();
    CHECK(str.empty());
    CHECK(str.size() == 0);
    CHECK(str.to_string() == "");
  }

  SECTION("simple string") {
    auto result = slp::parse("\"hello\"");
    CHECK(result.is_success());
    auto str = result.object().as_string();
    CHECK(!str.empty());
    CHECK(str.size() == 5);
    CHECK(str.to_string() == "hello");
    CHECK(str.at(0) == 'h');
    CHECK(str.at(1) == 'e');
    CHECK(str.at(2) == 'l');
    CHECK(str.at(3) == 'l');
    CHECK(str.at(4) == 'o');
  }

  SECTION("string with spaces") {
    auto result = slp::parse("\"hello world\"");
    CHECK(result.is_success());
    auto str = result.object().as_string();
    CHECK(str.size() == 11);
    CHECK(str.to_string() == "hello world");
    CHECK(str.at(5) == ' ');
  }
}

TEST_CASE("slp string operations - invalid type",
          "[unit][slp][string][interface]") {
  SECTION("integer is not a string") {
    auto result = slp::parse("42");
    CHECK(result.is_success());
    auto str = result.object().as_string();
    CHECK(str.empty());
    CHECK(str.size() == 0);
    CHECK(str.to_string() == "");
  }

  SECTION("list is not a string") {
    auto result = slp::parse("(1 2 3)");
    CHECK(result.is_success());
    auto str = result.object().as_string();
    CHECK(str.empty());
  }
}

TEST_CASE("slp edge cases - numbers", "[unit][slp][edge]") {
  SECTION("zero") {
    auto result = slp::parse("0");
    CHECK(result.is_success());
    CHECK(result.object().as_int() == 0);
  }

  SECTION("negative zero") {
    auto result = slp::parse("-0");
    CHECK(result.is_success());
    CHECK(result.object().as_int() == 0);
  }

  SECTION("large positive integer") {
    auto result = slp::parse("9223372036854775807");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::INTEGER);
    CHECK(result.object().as_int() == 9223372036854775807LL);
  }

  SECTION("large negative integer") {
    auto result = slp::parse("-9223372036854775808");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::INTEGER);
  }

  SECTION("zero real") {
    auto result = slp::parse("0.0");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::REAL);
    CHECK(result.object().as_real() == 0.0);
  }

  SECTION("negative zero real") {
    auto result = slp::parse("-0.0");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::REAL);
  }

  SECTION("scientific notation zero") {
    auto result = slp::parse("0e0");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::REAL);
    CHECK(result.object().as_real() == 0.0);
  }
}

TEST_CASE("slp edge cases - symbols", "[unit][slp][edge]") {
  SECTION("single character symbol") {
    auto result = slp::parse("x");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SYMBOL);
    CHECK(std::string(result.object().as_symbol()) == "x");
  }

  SECTION("symbol with dashes") {
    auto result = slp::parse("my-long-symbol-name");
    CHECK(result.is_success());
    CHECK(std::string(result.object().as_symbol()) == "my-long-symbol-name");
  }

  SECTION("symbol with underscores") {
    auto result = slp::parse("my_variable_name");
    CHECK(result.is_success());
    CHECK(std::string(result.object().as_symbol()) == "my_variable_name");
  }

  SECTION("symbol with mixed case") {
    auto result = slp::parse("MySymbol");
    CHECK(result.is_success());
    CHECK(std::string(result.object().as_symbol()) == "MySymbol");
  }
}

TEST_CASE("slp edge cases - empty and whitespace", "[unit][slp][edge]") {
  SECTION("only whitespace") {
    auto result = slp::parse("   \n\t  ");
    CHECK(result.is_error());
  }

  SECTION("only comments") {
    auto result = slp::parse("; just a comment\n; another comment");
    CHECK(result.is_error());
  }

  SECTION("whitespace before object") {
    auto result = slp::parse("  \n\t  42");
    CHECK(result.is_success());
    CHECK(result.object().as_int() == 42);
  }
}

TEST_CASE("slp error validation - byte positions", "[unit][slp][error]") {
  SECTION("unclosed paren position") {
    auto result = slp::parse("(1 2 3");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_PAREN_LIST);
    CHECK(result.error().byte_position == 0);
    CHECK(result.error().message.length() > 0);
  }

  SECTION("unclosed bracket position") {
    auto result = slp::parse("[1 2 3");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_BRACKET_LIST);
    CHECK(result.error().byte_position == 0);
  }

  SECTION("unclosed string position") {
    auto result = slp::parse("\"hello world");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_DQ_LIST);
    CHECK(result.error().byte_position == 0);
  }

  SECTION("nested unclosed paren") {
    auto result = slp::parse("(1 (2 3)");
    CHECK(result.is_error());
    CHECK(result.error().error_code ==
          slp::slp_parse_error_e::UNCLOSED_PAREN_LIST);
  }
}

TEST_CASE("slp move semantics", "[unit][slp][move]") {
  SECTION("move parse result") {
    auto result1 = slp::parse("42");
    CHECK(result1.is_success());

    auto result2 = std::move(result1);
    CHECK(result2.is_success());
    CHECK(result2.object().as_int() == 42);
  }

  SECTION("object from list at() can be moved") {
    auto result = slp::parse("(1 2 3)");
    CHECK(result.is_success());

    auto list = result.object().as_list();
    auto elem = list.at(0);
    CHECK(elem.type() == slp::slp_type_e::INTEGER);
    CHECK(elem.as_int() == 1);
    CHECK(elem.has_data());
  }

  SECTION("parse result contains movable object") {
    auto result = slp::parse("(hello world)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::PAREN_LIST);

    const auto &symbols = slp_test_accessor::get_symbols(result.object());
    CHECK(symbols.size() == 2);
  }
}

TEST_CASE("slp internal structure validation", "[unit][slp][internal]") {
  SECTION("integer internal structure") {
    auto result = slp::parse("42");
    CHECK(result.is_success());

    const auto &data = slp_test_accessor::get_data(result.object());
    CHECK(!data.empty());
    CHECK(data.size() >= sizeof(slp::slp_unit_of_store_t));

    const auto *view = slp_test_accessor::get_view(result.object());
    CHECK(view != nullptr);
    CHECK((view->header & 0xFF) ==
          static_cast<std::uint32_t>(slp::slp_type_e::INTEGER));
  }

  SECTION("symbol in symbol table") {
    auto result = slp::parse("hello");
    CHECK(result.is_success());

    const auto &symbols = slp_test_accessor::get_symbols(result.object());
    CHECK(!symbols.empty());
    CHECK(symbols.size() == 1);

    const auto *view = slp_test_accessor::get_view(result.object());
    std::uint64_t symbol_id = view->data.uint64;
    CHECK(symbols.find(symbol_id) != symbols.end());
    CHECK(symbols.at(symbol_id) == "hello");
  }

  SECTION("multiple symbols") {
    auto result = slp::parse("(foo bar baz)");
    CHECK(result.is_success());

    const auto &symbols = slp_test_accessor::get_symbols(result.object());
    CHECK(symbols.size() == 3);

    auto list = result.object().as_list();
    CHECK(list.size() == 3);
  }

  SECTION("root offset") {
    auto result = slp::parse("42");
    CHECK(result.is_success());

    size_t offset = slp_test_accessor::get_root_offset(result.object());
    const auto &data = slp_test_accessor::get_data(result.object());
    CHECK(offset < data.size());
  }

  SECTION("has_data correctness") {
    auto result = slp::parse("hello");
    CHECK(result.is_success());
    CHECK(result.object().has_data());

    slp::slp_object_c empty_obj;
    CHECK(!empty_obj.has_data());
  }
}

TEST_CASE("slp type safety", "[unit][slp][safety]") {
  SECTION("as_int on non-integer") {
    auto result = slp::parse("hello");
    CHECK(result.is_success());
    CHECK(result.object().as_int() == 0);
  }

  SECTION("as_real on non-real") {
    auto result = slp::parse("hello");
    CHECK(result.is_success());
    CHECK(result.object().as_real() == 0.0);
  }

  SECTION("as_symbol on non-symbol") {
    auto result = slp::parse("42");
    CHECK(result.is_success());
    CHECK(std::string(result.object().as_symbol()) == "");
  }

  SECTION("list operations on non-list") {
    auto result = slp::parse("42");
    CHECK(result.is_success());
    auto list = result.object().as_list();
    CHECK(list.empty());
    CHECK(list.size() == 0);
  }

  SECTION("string operations on non-string") {
    auto result = slp::parse("42");
    CHECK(result.is_success());
    auto str = result.object().as_string();
    CHECK(str.empty());
    CHECK(str.size() == 0);
    CHECK(str.at(0) == '\0');
  }
}

TEST_CASE("slp quoted and error objects", "[unit][slp][quote][error]") {
  SECTION("quoted integer") {
    auto result = slp::parse("'42");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }

  SECTION("quoted symbol") {
    auto result = slp::parse("'hello");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }

  SECTION("quoted list") {
    auto result = slp::parse("'(1 2 3)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }

  SECTION("multiple quotes") {
    auto result = slp::parse("''42");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }

  SECTION("error integer") {
    auto result = slp::parse("@404");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("error symbol") {
    auto result = slp::parse("@error");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("error list") {
    auto result = slp::parse("@(error message)");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("multiple errors") {
    auto result = slp::parse("@@error");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }

  SECTION("quote error combination") {
    auto result = slp::parse("'@error");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::SOME);
  }

  SECTION("error quote combination") {
    auto result = slp::parse("@'value");
    CHECK(result.is_success());
    CHECK(result.object().type() == slp::slp_type_e::ERROR);
  }
}
