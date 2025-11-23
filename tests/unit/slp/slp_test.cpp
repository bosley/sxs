#include <slp/slp.hpp>
#include <snitch/snitch.hpp>

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

TEST_CASE("slp parse bracket lists", "[unit][slp][list]") {
    SECTION("empty bracket list") {
        auto result = slp::parse("[]");
        CHECK(result.is_success());
        CHECK(result.object().type() == slp::slp_type_e::BRACKET_LIST);
    }
    
    SECTION("bracket list with content") {
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

TEST_CASE("slp parse env directive", "[unit][slp][env]") {
    SECTION("simple env") {
        auto result = slp::parse("[env my-program (let a 3)]");
        CHECK(result.is_success());
        CHECK(result.object().type() == slp::slp_type_e::ENVIRONMENT);
    }
    
    SECTION("env with multiple expressions") {
        auto result = slp::parse("[env test (let a 3) (putln a)]");
        CHECK(result.is_success());
        CHECK(result.object().type() == slp::slp_type_e::ENVIRONMENT);
    }
}

TEST_CASE("slp parse debug directive", "[unit][slp][debug]") {
    auto result = slp::parse("[debug (1 2 3)]");
    CHECK(result.is_success());
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
        CHECK(result.error().error_code == slp::slp_parse_error_e::UNCLOSED_PAREN_LIST);
    }
    
    SECTION("unclosed bracket") {
        auto result = slp::parse("[1 2 3");
        CHECK(result.is_error());
        CHECK(result.error().error_code == slp::slp_parse_error_e::UNCLOSED_BRACKET_LIST);
    }
    
    SECTION("unclosed brace") {
        auto result = slp::parse("{1 2 3");
        CHECK(result.is_error());
        CHECK(result.error().error_code == slp::slp_parse_error_e::UNCLOSED_BRACE_LIST);
    }
    
    SECTION("unclosed string") {
        auto result = slp::parse("\"hello");
        CHECK(result.is_error());
        CHECK(result.error().error_code == slp::slp_parse_error_e::UNCLOSED_DQ_LIST);
    }
}

TEST_CASE("slp parse example.slp structure", "[unit][slp][example]") {
    std::string example = R"(
[env my-program
    (let a 3)
    (putln a)
    [debug a]
]
)";
    auto result = slp::parse(example);
    CHECK(result.is_success());
}

TEST_CASE("slp parse complex example", "[unit][slp][example]") {
    std::string example = R"(
[env my-program
    (let a [env my-sub-env
        (let my_data 0)
    ])
    (let something {a my_data})
    (let a '(1 2 3))
]
)";
    auto result = slp::parse(example);
    CHECK(result.is_success());
}

