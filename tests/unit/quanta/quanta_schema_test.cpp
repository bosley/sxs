#include <snitch/snitch.hpp>
#include <quanta/schema.hpp>
#include <nlohmann/json.hpp>

using namespace pkg::quanta;

TEST_CASE("quanta field builder operations", "[unit][quanta]") {
    SECTION("default field builder") {
        field_builder_c builder;
        auto field = builder.build();

        CHECK(field.type == schema_field_type_e::UNSET);
        CHECK(field.length == 0);
        CHECK(field.max_length == 0);
        CHECK_FALSE(field.is_unique);
        CHECK_FALSE(field.is_required);
    }

    SECTION("field builder with type") {
        field_builder_c builder;
        auto field = builder.set_type(schema_field_type_e::STRING).build();

        CHECK(field.type == schema_field_type_e::STRING);
        CHECK(field.length == 0);
        CHECK(field.max_length == 0);
        CHECK_FALSE(field.is_unique);
        CHECK_FALSE(field.is_required);
    }

    SECTION("field builder with all properties") {
        field_builder_c builder;
        auto field = builder
            .set_type(schema_field_type_e::INT)
            .set_length(4)
            .set_max_length(100)
            .set_is_unique(true)
            .set_is_required(true)
            .build();

        CHECK(field.type == schema_field_type_e::INT);
        CHECK(field.length == 4);
        CHECK(field.max_length == 100);
        CHECK(field.is_unique);
        CHECK(field.is_required);
    }

    SECTION("field builder method chaining") {
        field_builder_c builder;

        // Test that methods return reference to builder
        auto& ref1 = builder.set_type(schema_field_type_e::FLOAT);
        auto& ref2 = builder.set_length(8);
        auto& ref3 = builder.set_max_length(50);

        CHECK(&ref1 == &builder);
        CHECK(&ref2 == &builder);
        CHECK(&ref3 == &builder);

        auto field = builder.build();
        CHECK(field.type == schema_field_type_e::FLOAT);
        CHECK(field.length == 8);
        CHECK(field.max_length == 50);
    }

    SECTION("field builder partial configuration") {
        field_builder_c builder;
        auto field = builder
            .set_type(schema_field_type_e::BOOL)
            .set_is_required(true)
            .build();

        CHECK(field.type == schema_field_type_e::BOOL);
        CHECK(field.length == 0);  // unchanged
        CHECK(field.max_length == 0);  // unchanged
        CHECK_FALSE(field.is_unique);  // unchanged
        CHECK(field.is_required);  // changed
    }
}

TEST_CASE("quanta schema builder operations", "[unit][quanta]") {
    SECTION("basic schema builder") {
        schema_builder_c builder("test_schema");
        auto schema = builder.build();

        CHECK(schema.get_name() == "test_schema");
        CHECK(schema.get_fields_meta().empty());
    }

    SECTION("schema builder with single field") {
        field_builder_c field_builder;
        auto field = field_builder
            .set_type(schema_field_type_e::STRING)
            .set_max_length(255)
            .set_is_required(true)
            .build();

        schema_builder_c schema_builder("user_schema");
        auto schema = schema_builder
            .with_field("username", field_builder)
            .build();

        CHECK(schema.get_name() == "user_schema");
        CHECK(schema.has_field("username"));
        CHECK_FALSE(schema.has_field("nonexistent"));

        auto field_meta = schema.get_field_meta("username");
        CHECK(field_meta.has_value());
        CHECK(field_meta->type == schema_field_type_e::STRING);
        CHECK(field_meta->max_length == 255);
        CHECK(field_meta->is_required);
    }

    SECTION("schema builder with multiple fields") {
        schema_builder_c builder("complex_schema");

        field_builder_c string_field;
        string_field
            .set_type(schema_field_type_e::STRING)
            .set_max_length(100);

        field_builder_c int_field;
        int_field
            .set_type(schema_field_type_e::INT)
            .set_length(4)
            .set_is_unique(true);

        field_builder_c bool_field;
        bool_field
            .set_type(schema_field_type_e::BOOL)
            .set_is_required(true);

        auto schema = builder
            .with_field("name", string_field)
            .with_field("age", int_field)
            .with_field("active", bool_field)
            .build();

        CHECK(schema.get_name() == "complex_schema");
        CHECK(schema.has_field("name"));
        CHECK(schema.has_field("age"));
        CHECK(schema.has_field("active"));

        auto name_meta = schema.get_field_meta("name");
        CHECK(name_meta.has_value());
        CHECK(name_meta->type == schema_field_type_e::STRING);
        CHECK(name_meta->max_length == 100);

        auto age_meta = schema.get_field_meta("age");
        CHECK(age_meta.has_value());
        CHECK(age_meta->type == schema_field_type_e::INT);
        CHECK(age_meta->length == 4);
        CHECK(age_meta->is_unique);

        auto active_meta = schema.get_field_meta("active");
        CHECK(active_meta.has_value());
        CHECK(active_meta->type == schema_field_type_e::BOOL);
        CHECK(active_meta->is_required);
    }

    SECTION("schema builder method chaining") {
        field_builder_c field_builder;
        field_builder.set_type(schema_field_type_e::INT);

        schema_builder_c schema_builder("test_schema");

        // Test that with_field returns reference to builder
        auto& ref = schema_builder.with_field("test_field", field_builder);
        CHECK(&ref == &schema_builder);

        auto schema = schema_builder.build();
        CHECK(schema.has_field("test_field"));
    }

    SECTION("schema builder field replacement") {
        field_builder_c field_builder1;
        field_builder1.set_type(schema_field_type_e::STRING);

        field_builder_c field_builder2;
        field_builder2.set_type(schema_field_type_e::INT);

        schema_builder_c builder("test_schema");
        auto schema = builder
            .with_field("test_field", field_builder1)
            .with_field("test_field", field_builder2)  // Should replace first field
            .build();

        CHECK(schema.has_field("test_field"));
        auto field_meta = schema.get_field_meta("test_field");
        CHECK(field_meta.has_value());
        CHECK(field_meta->type == schema_field_type_e::INT);  // Should be the last one set
    }
}

TEST_CASE("quanta schema operations", "[unit][quanta]") {
    SECTION("schema field existence") {
        std::map<std::string, schema_field_meta_c> fields;
        schema_field_meta_c field_meta;
        field_meta.type = schema_field_type_e::STRING;
        fields["username"] = field_meta;

        schema_c schema("user_schema", fields);

        CHECK(schema.has_field("username"));
        CHECK_FALSE(schema.has_field("password"));
        CHECK_FALSE(schema.has_field(""));
    }

    SECTION("schema get field meta") {
        std::map<std::string, schema_field_meta_c> fields;
        schema_field_meta_c field_meta;
        field_meta.type = schema_field_type_e::INT;
        field_meta.length = 4;
        field_meta.is_unique = true;
        fields["user_id"] = field_meta;

        schema_c schema("user_schema", fields);

        auto meta = schema.get_field_meta("user_id");
        CHECK(meta.has_value());
        CHECK(meta->type == schema_field_type_e::INT);
        CHECK(meta->length == 4);
        CHECK(meta->is_unique);

        auto nonexistent = schema.get_field_meta("nonexistent");
        CHECK_FALSE(nonexistent.has_value());
    }

    SECTION("schema get all fields meta") {
        std::map<std::string, schema_field_meta_c> fields;
        schema_field_meta_c field1, field2;
        field1.type = schema_field_type_e::STRING;
        field2.type = schema_field_type_e::INT;
        fields["name"] = field1;
        fields["age"] = field2;

        schema_c schema("user_schema", fields);

        const auto& all_fields = schema.get_fields_meta();
        CHECK(all_fields.size() == 2);
        CHECK(all_fields.count("name") == 1);
        CHECK(all_fields.count("age") == 1);
        CHECK(all_fields.at("name").type == schema_field_type_e::STRING);
        CHECK(all_fields.at("age").type == schema_field_type_e::INT);
    }

    SECTION("schema get name") {
        std::map<std::string, schema_field_meta_c> fields;
        schema_c schema("test_schema", fields);

        CHECK(schema.get_name() == "test_schema");
    }
}

TEST_CASE("quanta field type enum", "[unit][quanta]") {
    SECTION("field type enum values") {
        CHECK(schema_field_type_e::UNSET == schema_field_type_e::UNSET);
        CHECK(schema_field_type_e::STRING == schema_field_type_e::STRING);
        CHECK(schema_field_type_e::INT == schema_field_type_e::INT);
        CHECK(schema_field_type_e::FLOAT == schema_field_type_e::FLOAT);
        CHECK(schema_field_type_e::BOOL == schema_field_type_e::BOOL);
        CHECK(schema_field_type_e::TIMEPOINT == schema_field_type_e::TIMEPOINT);
        CHECK(schema_field_type_e::DURATION == schema_field_type_e::DURATION);
        CHECK(schema_field_type_e::BINARY == schema_field_type_e::BINARY);
        CHECK(schema_field_type_e::SENTINEL == schema_field_type_e::SENTINEL);
    }

    SECTION("field type to string mapping") {
        CHECK(schema_field_type_to_string.at(schema_field_type_e::UNSET) == "UNSET");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::STRING) == "STRING");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::INT) == "INT");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::FLOAT) == "FLOAT");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::BOOL) == "BOOL");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::TIMEPOINT) == "TIMEPOINT");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::DURATION) == "DURATION");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::BINARY) == "BINARY");
        CHECK(schema_field_type_to_string.at(schema_field_type_e::SENTINEL) == "SENTINEL");
    }
}

TEST_CASE("quanta complex builder scenarios", "[unit][quanta]") {
    SECTION("user profile schema") {
        auto schema = schema_builder_c("user_profile")
            .with_field("id", field_builder_c().set_type(schema_field_type_e::INT).set_length(8).set_is_unique(true).set_is_required(true))
            .with_field("username", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(50).set_is_unique(true).set_is_required(true))
            .with_field("email", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(255).set_is_unique(true).set_is_required(true))
            .with_field("age", field_builder_c().set_type(schema_field_type_e::INT).set_length(2).set_is_required(false))
            .with_field("is_active", field_builder_c().set_type(schema_field_type_e::BOOL).set_is_required(true))
            .with_field("created_at", field_builder_c().set_type(schema_field_type_e::TIMEPOINT).set_is_required(true))
            .with_field("profile_data", field_builder_c().set_type(schema_field_type_e::BINARY).set_max_length(1024).set_is_required(false))
            .build();

        CHECK(schema.get_name() == "user_profile");
        CHECK(schema.get_fields_meta().size() == 7);

        // Verify ID field
        auto id_meta = schema.get_field_meta("id");
        CHECK(id_meta.has_value());
        CHECK(id_meta->type == schema_field_type_e::INT);
        CHECK(id_meta->length == 8);
        CHECK(id_meta->is_unique);
        CHECK(id_meta->is_required);

        // Verify username field
        auto username_meta = schema.get_field_meta("username");
        CHECK(username_meta.has_value());
        CHECK(username_meta->type == schema_field_type_e::STRING);
        CHECK(username_meta->max_length == 50);
        CHECK(username_meta->is_unique);
        CHECK(username_meta->is_required);

        // Verify optional age field
        auto age_meta = schema.get_field_meta("age");
        CHECK(age_meta.has_value());
        CHECK(age_meta->type == schema_field_type_e::INT);
        CHECK(age_meta->length == 2);
        CHECK_FALSE(age_meta->is_unique);
        CHECK_FALSE(age_meta->is_required);
    }

    SECTION("product catalog schema") {
        auto schema = schema_builder_c("product")
            .with_field("sku", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(20).set_is_unique(true).set_is_required(true))
            .with_field("name", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(200).set_is_required(true))
            .with_field("price", field_builder_c().set_type(schema_field_type_e::FLOAT).set_length(8).set_is_required(true))
            .with_field("in_stock", field_builder_c().set_type(schema_field_type_e::BOOL).set_is_required(true))
            .with_field("description", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(1000).set_is_required(false))
            .build();

        CHECK(schema.get_name() == "product");
        CHECK(schema.get_fields_meta().size() == 5);

        // Verify SKU field
        auto sku_meta = schema.get_field_meta("sku");
        CHECK(sku_meta.has_value());
        CHECK(sku_meta->type == schema_field_type_e::STRING);
        CHECK(sku_meta->max_length == 20);
        CHECK(sku_meta->is_unique);
        CHECK(sku_meta->is_required);

        // Verify price field
        auto price_meta = schema.get_field_meta("price");
        CHECK(price_meta.has_value());
        CHECK(price_meta->type == schema_field_type_e::FLOAT);
        CHECK(price_meta->length == 8);
        CHECK(price_meta->is_required);

        // Verify optional description
        auto desc_meta = schema.get_field_meta("description");
        CHECK(desc_meta.has_value());
        CHECK(desc_meta->type == schema_field_type_e::STRING);
        CHECK(desc_meta->max_length == 1000);
        CHECK_FALSE(desc_meta->is_required);
    }
}

TEST_CASE("quanta json serialization", "[unit][quanta]") {
    SECTION("schema_field_type_e json serialization") {
        SECTION("enum to json") {
            nlohmann::json j = schema_field_type_e::STRING;
            CHECK(j == "STRING");

            j = schema_field_type_e::INT;
            CHECK(j == "INT");

            j = schema_field_type_e::BOOL;
            CHECK(j == "BOOL");

            j = schema_field_type_e::UNSET;
            CHECK(j == "UNSET");
        }

        SECTION("enum from json") {
            schema_field_type_e type;
            nlohmann::json j = "FLOAT";
            j.get_to(type);
            CHECK(type == schema_field_type_e::FLOAT);

            j = "BINARY";
            j.get_to(type);
            CHECK(type == schema_field_type_e::BINARY);
        }

        SECTION("enum roundtrip") {
            schema_field_type_e original = schema_field_type_e::TIMEPOINT;
            nlohmann::json j = original;
            schema_field_type_e restored;
            j.get_to(restored);
            CHECK(original == restored);
        }

        SECTION("invalid enum from json throws") {
            schema_field_type_e type;
            nlohmann::json j = "INVALID_TYPE";
            CHECK_THROWS_AS(j.get_to(type), std::runtime_error);
        }
    }

    SECTION("schema_field_meta_c json serialization") {
        SECTION("struct to json") {
            schema_field_meta_c meta;
            meta.type = schema_field_type_e::STRING;
            meta.length = 10;
            meta.max_length = 255;
            meta.is_unique = true;
            meta.is_required = false;

            nlohmann::json j = meta;
            CHECK(j["type"] == "STRING");
            CHECK(j["length"] == 10);
            CHECK(j["max_length"] == 255);
            CHECK(j["is_unique"] == true);
            CHECK(j["is_required"] == false);
        }

        SECTION("struct from json") {
            nlohmann::json j = {
                {"type", "INT"},
                {"length", 4},
                {"max_length", 100},
                {"is_unique", false},
                {"is_required", true}
            };

            schema_field_meta_c meta = j.get<schema_field_meta_c>();
            CHECK(meta.type == schema_field_type_e::INT);
            CHECK(meta.length == 4);
            CHECK(meta.max_length == 100);
            CHECK_FALSE(meta.is_unique);
            CHECK(meta.is_required);
        }

        SECTION("struct roundtrip") {
            schema_field_meta_c original;
            original.type = schema_field_type_e::BOOL;
            original.length = 1;
            original.max_length = 1;
            original.is_unique = false;
            original.is_required = true;

            nlohmann::json j = original;
            schema_field_meta_c restored = j.get<schema_field_meta_c>();

            CHECK(original.type == restored.type);
            CHECK(original.length == restored.length);
            CHECK(original.max_length == restored.max_length);
            CHECK(original.is_unique == restored.is_unique);
            CHECK(original.is_required == restored.is_required);
        }
    }

    SECTION("schema_c json serialization") {
        SECTION("empty schema to json") {
            std::map<std::string, schema_field_meta_c> fields;
            schema_c schema("empty_schema", fields);

            nlohmann::json j = schema.to_json();
            CHECK(j["name"] == "empty_schema");
            CHECK(j["fields"].empty());
        }

        SECTION("schema from json") {
            nlohmann::json j = {
                {"name", "test_schema"},
                {"fields", {
                    {"field1", {
                        {"type", "STRING"},
                        {"length", 0},
                        {"max_length", 100},
                        {"is_unique", false},
                        {"is_required", true}
                    }},
                    {"field2", {
                        {"type", "INT"},
                        {"length", 4},
                        {"max_length", 0},
                        {"is_unique", true},
                        {"is_required", false}
                    }}
                }}
            };

            schema_c schema = schema_c::from_json(j);
            CHECK(schema.get_name() == "test_schema");
            CHECK(schema.has_field("field1"));
            CHECK(schema.has_field("field2"));

            auto field1_meta = schema.get_field_meta("field1");
            CHECK(field1_meta.has_value());
            CHECK(field1_meta->type == schema_field_type_e::STRING);
            CHECK(field1_meta->max_length == 100);
            CHECK(field1_meta->is_required);

            auto field2_meta = schema.get_field_meta("field2");
            CHECK(field2_meta.has_value());
            CHECK(field2_meta->type == schema_field_type_e::INT);
            CHECK(field2_meta->length == 4);
            CHECK(field2_meta->is_unique);
            CHECK_FALSE(field2_meta->is_required);
        }

        SECTION("schema roundtrip") {
            // Create a schema using builder pattern
            auto original_schema = schema_builder_c("user_profile")
                .with_field("id", field_builder_c().set_type(schema_field_type_e::INT).set_length(8).set_is_unique(true).set_is_required(true))
                .with_field("username", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(50).set_is_unique(true).set_is_required(true))
                .with_field("email", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(255).set_is_required(true))
                .with_field("active", field_builder_c().set_type(schema_field_type_e::BOOL).set_is_required(false))
                .build();

            // Serialize to JSON
            nlohmann::json j = original_schema.to_json();

            // Deserialize from JSON
            schema_c restored_schema = schema_c::from_json(j);

            // Verify all properties match
            CHECK(original_schema.get_name() == restored_schema.get_name());
            CHECK(original_schema.get_fields_meta().size() == restored_schema.get_fields_meta().size());

            // Check each field
            for (const auto& [field_name, original_meta] : original_schema.get_fields_meta()) {
                auto restored_meta = restored_schema.get_field_meta(field_name);
                CHECK(restored_meta.has_value());
                CHECK(original_meta.type == restored_meta->type);
                CHECK(original_meta.length == restored_meta->length);
                CHECK(original_meta.max_length == restored_meta->max_length);
                CHECK(original_meta.is_unique == restored_meta->is_unique);
                CHECK(original_meta.is_required == restored_meta->is_required);
            }
        }

        SECTION("simple multi-field schema json serialization") {
            // Test schema with multiple fields to verify basic multi-field functionality
            auto multi_schema = schema_builder_c("multi_test")
                .with_field("name", field_builder_c().set_type(schema_field_type_e::STRING).set_max_length(100))
                .with_field("age", field_builder_c().set_type(schema_field_type_e::INT).set_length(4))
                .with_field("active", field_builder_c().set_type(schema_field_type_e::BOOL))
                .build();

            // Test roundtrip
            nlohmann::json j = multi_schema.to_json();
            schema_c restored = schema_c::from_json(j);

            CHECK(multi_schema.get_name() == restored.get_name());
            CHECK(multi_schema.get_fields_meta().size() == restored.get_fields_meta().size());

            // Check each field individually
            auto name_meta = restored.get_field_meta("name");
            CHECK(name_meta.has_value());
            CHECK(name_meta->type == schema_field_type_e::STRING);
            CHECK(name_meta->max_length == 100);

            auto age_meta = restored.get_field_meta("age");
            CHECK(age_meta.has_value());
            CHECK(age_meta->type == schema_field_type_e::INT);
            CHECK(age_meta->length == 4);

            auto active_meta = restored.get_field_meta("active");
            CHECK(active_meta.has_value());
            CHECK(active_meta->type == schema_field_type_e::BOOL);
        }
    }
}
