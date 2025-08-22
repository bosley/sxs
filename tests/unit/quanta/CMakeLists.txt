add_executable(quanta_tests
  quanta_schema_test.cpp
  quanta_kvds_test.cpp
)

target_link_libraries(quanta_tests PRIVATE
  extern::snitch
  pkg::quanta
  extern::json
)

add_dependencies(build_tests quanta_tests)

add_test(NAME quanta_tests COMMAND quanta_tests)

add_test(NAME quanta_field_builder_operations
         COMMAND quanta_tests "quanta field builder operations")
add_test(NAME quanta_schema_builder_operations
         COMMAND quanta_tests "quanta schema builder operations")
add_test(NAME quanta_schema_operations
         COMMAND quanta_tests "quanta schema operations")
add_test(NAME quanta_field_type_enum
         COMMAND quanta_tests "quanta field type enum")
add_test(NAME quanta_complex_builder_scenarios
         COMMAND quanta_tests "quanta complex builder scenarios")
add_test(NAME quanta_json_serialization
         COMMAND quanta_tests "quanta json serialization")