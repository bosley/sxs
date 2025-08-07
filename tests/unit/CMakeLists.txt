add_executable(snitch_tests_unit
  simple.cpp
)

target_link_libraries(snitch_tests_unit PRIVATE extern::snitch)

add_dependencies(build_tests snitch_tests_unit)

add_test(NAME snitch_tests_unit COMMAND snitch_tests_unit)


