add_executable(ts_tests
  simple.cpp
)

target_link_libraries(ts_tests PRIVATE 
  extern::snitch
  pkg::ts
  pkg::runtime
)

add_dependencies(build_tests ts_tests)

add_test(NAME ts_tests COMMAND ts_tests)

