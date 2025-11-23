add_executable(slp_tests
  slp_test.cpp
)

target_link_libraries(slp_tests PRIVATE 
  extern::snitch
  pkg::slp
)

add_dependencies(build_tests slp_tests)

add_test(NAME slp_tests COMMAND slp_tests)

