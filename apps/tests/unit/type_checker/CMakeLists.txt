set(TYPE_CHECKER_TEST_DATA_FILES
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

foreach(TEST_FILE ${TYPE_CHECKER_TEST_DATA_FILES})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE}
        ${TEST_DATA_DIR}/${TEST_FILE}
        COPYONLY
    )
endforeach()

add_executable(type_checker_basic_test type_checker_basic_test.cpp)
target_link_libraries(type_checker_basic_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME type_checker_basic_test COMMAND type_checker_basic_test)

add_executable(type_checker_lambda_test type_checker_lambda_test.cpp)
target_link_libraries(type_checker_lambda_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME type_checker_lambda_test COMMAND type_checker_lambda_test)

add_executable(type_checker_datum_test type_checker_datum_test.cpp)
target_link_libraries(type_checker_datum_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME type_checker_datum_test COMMAND type_checker_datum_test)

add_executable(type_checker_kernel_test type_checker_kernel_test.cpp)
target_link_libraries(type_checker_kernel_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
target_compile_definitions(type_checker_kernel_test PRIVATE TEST_KERNEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_kernels")
add_test(NAME type_checker_kernel_test COMMAND type_checker_kernel_test)

add_executable(type_checker_file_test type_checker_file_test.cpp)
target_link_libraries(type_checker_file_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME type_checker_file_test COMMAND type_checker_file_test)

