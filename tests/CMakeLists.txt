add_custom_target(build_tests)

add_subdirectory(unit)

add_custom_target(run_tests ALL
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose -C $<CONFIG> --test-dir ${CMAKE_BINARY_DIR}
  DEPENDS build_tests sxs
)

