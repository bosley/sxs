add_executable(kvds_tests
  kvds_test.cpp
)

target_link_libraries(kvds_tests PRIVATE 
  extern::snitch
  pkg::kvds
)

add_dependencies(build_tests kvds_tests)

add_test(NAME kvds_tests COMMAND kvds_tests)

add_test(NAME kvds_basic_operations 
         COMMAND kvds_tests "kvds basic operations")
add_test(NAME kvds_prefix_operations 
         COMMAND kvds_tests "kvds prefix operations") 
add_test(NAME kvds_batch_operations 
         COMMAND kvds_tests "kvds batch operations")
add_test(NAME kvds_iteration 
         COMMAND kvds_tests "kvds iteration")
add_test(NAME kvds_error_conditions 
         COMMAND kvds_tests "kvds error conditions")


