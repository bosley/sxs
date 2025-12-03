add_executable(slp_tests
  slp_test.cpp
)

target_include_directories(slp_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/root
)

target_link_libraries(slp_tests PRIVATE 
  snitch::snitch
  pkg::slp
)

add_dependencies(build_tests slp_tests)

add_test(NAME slp_tests COMMAND slp_tests)

