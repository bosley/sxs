set(TYPE_CHECKER_TEST_DATA_FILES
    test_basic_types.sxs
    test_functions.sxs
    test_control_flow.sxs
    test_error_handling.sxs
    test_integration_lib.sxs
    test_forms_basic.sxs
    test_forms_nested.sxs
    test_forms_functions.sxs
    test_forms_control_flow.sxs
    test_forms_complex.sxs
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

add_executable(type_checker_composite_test type_checker_composite_test.cpp)
target_link_libraries(type_checker_composite_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
target_compile_definitions(type_checker_composite_test PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
add_test(NAME type_checker_composite_test COMMAND type_checker_composite_test)

add_executable(type_checker_composite_runtime_test type_checker_composite_runtime_test.cpp)
target_link_libraries(type_checker_composite_runtime_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
add_test(NAME type_checker_composite_runtime_test COMMAND type_checker_composite_runtime_test)

add_library(kernel_forms_test SHARED
  test_kernels/forms_test/forms_test.cpp
)

target_include_directories(kernel_forms_test PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/root
)

target_compile_definitions(kernel_forms_test PRIVATE
  SXS_KERNEL_BUILD
)

target_link_libraries(kernel_forms_test PRIVATE
  pkg::slp
)

set_target_properties(kernel_forms_test PROPERTIES
  PREFIX "lib"
  OUTPUT_NAME "kernel_forms_test"
  SUFFIX ".dylib"
)

add_executable(type_checker_kernel_forms_test type_checker_kernel_forms_test.cpp)
target_link_libraries(type_checker_kernel_forms_test PRIVATE pkg_core snitch::snitch spdlog::spdlog)
target_compile_definitions(type_checker_kernel_forms_test PRIVATE TEST_KERNEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_kernels")
add_test(NAME type_checker_kernel_forms_test COMMAND type_checker_kernel_forms_test)
add_dependencies(type_checker_kernel_forms_test kernel_forms_test)

