add_executable(record_tests
  record_test.cpp
)

target_link_libraries(record_tests PRIVATE 
  extern::snitch
  pkg::record
)

add_dependencies(build_tests record_tests)

add_test(NAME record_tests COMMAND record_tests)

