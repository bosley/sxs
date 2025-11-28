file(MAKE_DIRECTORY ${TEST_DATA_DIR})

set(SXS_TEST_FILES
  test_basic_scoping.sxs
  test_functions.sxs
  test_type_checking.sxs
  test_edge_cases.sxs
  test_nested_scopes.sxs
  test_lambda_cleanup.sxs
  test_import_basic.sxs
  test_import_exported_basic.sxs
  test_import_lambda.sxs
  test_import_exported_lambda.sxs
  test_import_values.sxs
  test_import_exported_values.sxs
  test_import_circular_a.sxs
  test_import_circular_b.sxs
  test_import_circular_3way_a.sxs
  test_import_circular_3way_b.sxs
  test_import_circular_3way_c.sxs
)

foreach(TEST_FILE ${SXS_TEST_FILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE}
    ${TEST_DATA_DIR}/${TEST_FILE}
    COPYONLY
  )
endforeach()

add_executable(basic_scoping_tests
  basic_scoping_test.cpp
)

target_link_libraries(basic_scoping_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
)

add_dependencies(build_tests basic_scoping_tests)
add_test(NAME basic_scoping_tests COMMAND basic_scoping_tests)

add_executable(functions_tests
  functions_test.cpp
)

target_link_libraries(functions_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
)

add_dependencies(build_tests functions_tests)
add_test(NAME functions_tests COMMAND functions_tests)

add_executable(type_checking_tests
  type_checking_test.cpp
)

target_link_libraries(type_checking_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
)

add_dependencies(build_tests type_checking_tests)
add_test(NAME type_checking_tests COMMAND type_checking_tests)

add_executable(edge_cases_tests
  edge_cases_test.cpp
)

target_link_libraries(edge_cases_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
)

add_dependencies(build_tests edge_cases_tests)
add_test(NAME edge_cases_tests COMMAND edge_cases_tests)

add_executable(nested_scopes_tests
  nested_scopes_test.cpp
)

target_link_libraries(nested_scopes_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
)

add_dependencies(build_tests nested_scopes_tests)
add_test(NAME nested_scopes_tests COMMAND nested_scopes_tests)

add_executable(lambda_cleanup_tests
  lambda_cleanup_test.cpp
)

target_link_libraries(lambda_cleanup_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
)

add_dependencies(build_tests lambda_cleanup_tests)
add_test(NAME lambda_cleanup_tests COMMAND lambda_cleanup_tests)

add_executable(import_tests
  import_test.cpp
)

target_link_libraries(import_tests PRIVATE 
  extern::snitch
  pkg::core
  pkg::slp
  spdlog::spdlog
)

add_dependencies(build_tests import_tests)
add_test(NAME import_tests COMMAND import_tests)

