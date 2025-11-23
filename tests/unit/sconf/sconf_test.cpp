#include <sconf/sconf.hpp>
#include <snitch/snitch.hpp>

TEST_CASE("sconf basic integer field", "[unit][sconf][field]") {
  std::string source = "[(age 42)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("age").type() == slp::slp_type_e::INTEGER);
  CHECK(config.at("age").as_int() == 42);
}

TEST_CASE("sconf basic real field", "[unit][sconf][field]") {
  std::string source = "[(temperature 98.6)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::FLOAT64, "temperature")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("temperature").type() == slp::slp_type_e::REAL);
  CHECK(config.at("temperature").as_real() == 98.6);
}

TEST_CASE("sconf basic string field", "[unit][sconf][field]") {
  std::string source = R"([(name "Alice")])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::STRING, "name")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("name").type() == slp::slp_type_e::DQ_LIST);
  CHECK(config.at("name").as_string().to_string() == "Alice");
}

TEST_CASE("sconf multiple fields", "[unit][sconf][field]") {
  std::string source = R"([(age 42) (name "Bob") (score 95.5)])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .WithField(sconf::sconf_type_e::STRING, "name")
                    .WithField(sconf::sconf_type_e::FLOAT64, "score")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("age").as_int() == 42);
  CHECK(config.at("name").as_string().to_string() == "Bob");
  CHECK(config.at("score").as_real() == 95.5);
}

TEST_CASE("sconf list of integers", "[unit][sconf][list]") {
  std::string source = "[(numbers (1 2 3 4 5))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::INT64, "numbers")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto list = config.at("numbers").as_list();
  CHECK(list.size() == 5);
  CHECK(list.at(0).as_int() == 1);
  CHECK(list.at(4).as_int() == 5);
}

TEST_CASE("sconf list of reals", "[unit][sconf][list]") {
  std::string source = "[(values (1.1 2.2 3.3))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::FLOAT64, "values")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto list = config.at("values").as_list();
  CHECK(list.size() == 3);
  CHECK(list.at(0).as_real() == 1.1);
  CHECK(list.at(2).as_real() == 3.3);
}

TEST_CASE("sconf list of strings", "[unit][sconf][list]") {
  std::string source = R"([(names ("Alice" "Bob" "Charlie"))])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::STRING, "names")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto list = config.at("names").as_list();
  CHECK(list.size() == 3);
  CHECK(list.at(0).as_string().to_string() == "Alice");
  CHECK(list.at(1).as_string().to_string() == "Bob");
  CHECK(list.at(2).as_string().to_string() == "Charlie");
}

TEST_CASE("sconf empty list", "[unit][sconf][list]") {
  std::string source = "[(numbers ())]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::INT64, "numbers")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto list = config.at("numbers").as_list();
  CHECK(list.size() == 0);
  CHECK(list.empty());
}

TEST_CASE("sconf missing field error", "[unit][sconf][error]") {
  std::string source = "[(age 42)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .WithField(sconf::sconf_type_e::STRING, "name")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::MISSING_FIELD);
  CHECK(result.error().field_name == "name");
}

TEST_CASE("sconf type mismatch error", "[unit][sconf][error]") {
  std::string source = R"([(age "forty-two")])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::TYPE_MISMATCH);
  CHECK(result.error().field_name == "age");
}

TEST_CASE("sconf type mismatch real vs int", "[unit][sconf][error]") {
  std::string source = "[(score 95.5)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "score")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::TYPE_MISMATCH);
}

TEST_CASE("sconf non-homogeneous list error", "[unit][sconf][error]") {
  std::string source = "[(mixed (1 2.5 3))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::INT64, "mixed")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code ==
        sconf::sconf_error_e::INVALID_LIST_ELEMENT);
  CHECK(result.error().field_name == "mixed");
}

TEST_CASE("sconf non-homogeneous list with string", "[unit][sconf][error]") {
  std::string source = R"([(mixed (1 "two" 3))])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::INT64, "mixed")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code ==
        sconf::sconf_error_e::INVALID_LIST_ELEMENT);
}

TEST_CASE("sconf invalid structure not bracket list", "[unit][sconf][error]") {
  std::string source = "(age 42)";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::INVALID_STRUCTURE);
}

TEST_CASE("sconf invalid structure not pairs", "[unit][sconf][error]") {
  std::string source = "[(age)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::INVALID_STRUCTURE);
}

TEST_CASE("sconf invalid structure non-symbol key", "[unit][sconf][error]") {
  std::string source = R"([(42 "value")])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::STRING, "42")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::INVALID_STRUCTURE);
}

TEST_CASE("sconf invalid structure triple instead of pair",
          "[unit][sconf][error]") {
  std::string source = "[(age 42 extra)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::INVALID_STRUCTURE);
}

TEST_CASE("sconf slp parse error", "[unit][sconf][error]") {
  std::string source = "[(age 42";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code == sconf::sconf_error_e::SLP_PARSE_ERROR);
}

TEST_CASE("sconf extra fields allowed", "[unit][sconf][validation]") {
  std::string source = R"([(age 42) (name "Alice") (extra "data")])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT64, "age")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.size() == 3);
}

TEST_CASE("sconf all integer types", "[unit][sconf][types]") {
  std::string source =
      "[(i8 1) (i16 2) (i32 3) (i64 4) (u8 5) (u16 6) (u32 7) (u64 8)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::INT8, "i8")
                    .WithField(sconf::sconf_type_e::INT16, "i16")
                    .WithField(sconf::sconf_type_e::INT32, "i32")
                    .WithField(sconf::sconf_type_e::INT64, "i64")
                    .WithField(sconf::sconf_type_e::UINT8, "u8")
                    .WithField(sconf::sconf_type_e::UINT16, "u16")
                    .WithField(sconf::sconf_type_e::UINT32, "u32")
                    .WithField(sconf::sconf_type_e::UINT64, "u64")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("i8").as_int() == 1);
  CHECK(config.at("u64").as_int() == 8);
}

TEST_CASE("sconf all float types", "[unit][sconf][types]") {
  std::string source = "[(f32 1.5) (f64 2.5)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::FLOAT32, "f32")
                    .WithField(sconf::sconf_type_e::FLOAT64, "f64")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("f32").as_real() == 1.5);
  CHECK(config.at("f64").as_real() == 2.5);
}

TEST_CASE("sconf builder chaining", "[unit][sconf][builder]") {
  std::string source =
      R"([(host "localhost") (port 8080) (workers (1 2 3 4))])";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithField(sconf::sconf_type_e::STRING, "host")
                    .WithField(sconf::sconf_type_e::INT64, "port")
                    .WithList(sconf::sconf_type_e::INT64, "workers")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  CHECK(config.at("host").as_string().to_string() == "localhost");
  CHECK(config.at("port").as_int() == 8080);
  CHECK(config.at("workers").as_list().size() == 4);
}

TEST_CASE("sconf list not a scalar error", "[unit][sconf][error]") {
  std::string source = "[(data 42)]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::INT64, "data")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code ==
        sconf::sconf_error_e::INVALID_LIST_ELEMENT);
}

TEST_CASE("sconf list of lists paren", "[unit][sconf][list_list]") {
  std::string source = "[(matrix ((1 2 3) (4 5 6) (7 8 9)))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::LIST, "matrix")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto outer_list = config.at("matrix").as_list();
  CHECK(outer_list.size() == 3);

  auto first_elem = outer_list.at(0);
  auto first_row = first_elem.as_list();
  CHECK(first_row.size() == 3);
  CHECK(first_row.at(0).as_int() == 1);
  CHECK(first_row.at(2).as_int() == 3);
}

TEST_CASE("sconf list of lists bracket", "[unit][sconf][list_list]") {
  std::string source = "[(configs ([(a 1)] [(b 2)] [(c 3)]))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::LIST, "configs")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto outer_list = config.at("configs").as_list();
  CHECK(outer_list.size() == 3);
  CHECK(outer_list.at(0).type() == slp::slp_type_e::BRACKET_LIST);
}

TEST_CASE("sconf list of lists mixed types", "[unit][sconf][list_list]") {
  std::string source = "[(mixed ((1 2) [3 4] {5 6}))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::LIST, "mixed")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto outer_list = config.at("mixed").as_list();
  CHECK(outer_list.size() == 3);
  CHECK(outer_list.at(0).type() == slp::slp_type_e::PAREN_LIST);
  CHECK(outer_list.at(1).type() == slp::slp_type_e::BRACKET_LIST);
  CHECK(outer_list.at(2).type() == slp::slp_type_e::BRACE_LIST);
}

TEST_CASE("sconf list of lists empty inner lists", "[unit][sconf][list_list]") {
  std::string source = "[(empty_lists (() [] {}))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::LIST, "empty_lists")
                    .Parse();

  CHECK(result.is_success());
  const auto &config = result.config();
  auto outer_list = config.at("empty_lists").as_list();
  CHECK(outer_list.size() == 3);
  auto first_elem = outer_list.at(0);
  CHECK(first_elem.as_list().empty());
}

TEST_CASE("sconf list of lists with non-list element error",
          "[unit][sconf][error]") {
  std::string source = "[(bad ((1 2) 42 (3 4)))]";

  auto result = sconf::sconf_builder_c::from(source)
                    .WithList(sconf::sconf_type_e::LIST, "bad")
                    .Parse();

  CHECK(result.is_error());
  CHECK(result.error().error_code ==
        sconf::sconf_error_e::INVALID_LIST_ELEMENT);
  CHECK(result.error().field_name == "bad");
}
