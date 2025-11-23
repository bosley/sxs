add_executable(sconf_tests
  sconf_test.cpp
)

target_link_libraries(sconf_tests PRIVATE 
  extern::snitch
  pkg::sconf
)

add_dependencies(build_tests sconf_tests)

add_test(NAME sconf_tests COMMAND sconf_tests)

