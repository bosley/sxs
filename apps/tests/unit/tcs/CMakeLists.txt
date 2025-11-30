set(TCS_TEST_DATA_FILES
    test_basic_types.sxs
    test_functions.sxs
    test_control_flow.sxs
    test_error_handling.sxs
    test_imports.sxs
    test_import_exported.sxs
    test_import_lambda_a.sxs
    test_import_lambda_b.sxs
    test_import_nested_a.sxs
    test_import_nested_b.sxs
    test_integration_lib.sxs
)

foreach(TEST_FILE ${TCS_TEST_DATA_FILES})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE}
        ${TEST_DATA_DIR}/${TEST_FILE}
        COPYONLY
    )
endforeach()

add_executable(tcs_basic_types_test tcs_basic_types_test.cpp)
target_link_libraries(tcs_basic_types_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_basic_types_test COMMAND tcs_basic_types_test)

add_executable(tcs_functions_test tcs_functions_test.cpp)
target_link_libraries(tcs_functions_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_functions_test COMMAND tcs_functions_test)

add_executable(tcs_control_flow_test tcs_control_flow_test.cpp)
target_link_libraries(tcs_control_flow_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_control_flow_test COMMAND tcs_control_flow_test)

add_executable(tcs_error_handling_test tcs_error_handling_test.cpp)
target_link_libraries(tcs_error_handling_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_error_handling_test COMMAND tcs_error_handling_test)

add_executable(tcs_imports_test tcs_imports_test.cpp)
target_link_libraries(tcs_imports_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_imports_test COMMAND tcs_imports_test)

add_executable(tcs_type_errors_test tcs_type_errors_test.cpp)
target_link_libraries(tcs_type_errors_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_type_errors_test COMMAND tcs_type_errors_test)

add_executable(tcs_scope_test tcs_scope_test.cpp)
target_link_libraries(tcs_scope_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_scope_test COMMAND tcs_scope_test)

add_executable(tcs_lambda_invocation_test tcs_lambda_invocation_test.cpp)
target_link_libraries(tcs_lambda_invocation_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_lambda_invocation_test COMMAND tcs_lambda_invocation_test)

add_executable(tcs_kernel_validation_test tcs_kernel_validation_test.cpp)
target_link_libraries(tcs_kernel_validation_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_kernel_validation_test COMMAND tcs_kernel_validation_test)

add_executable(tcs_import_lambda_test tcs_import_lambda_test.cpp)
target_link_libraries(tcs_import_lambda_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_import_lambda_test COMMAND tcs_import_lambda_test)

add_executable(tcs_integration_test tcs_integration_test.cpp)
target_link_libraries(tcs_integration_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME tcs_integration_test COMMAND tcs_integration_test)

