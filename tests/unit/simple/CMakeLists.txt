add_executable(simple_tests
  simple.cpp
)

target_link_libraries(simple_tests PRIVATE 
  extern::snitch
)

add_dependencies(build_tests simple_tests)

add_test(NAME simple_tests COMMAND simple_tests)

