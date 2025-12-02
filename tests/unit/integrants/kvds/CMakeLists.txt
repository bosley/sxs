add_executable(kvds_tests
  kvds_test.cpp
  memstore_test.cpp
  distributor_test.cpp
)

target_link_libraries(kvds_tests PRIVATE 
  snitch::snitch
  pkg::kvds
)

add_dependencies(build_tests kvds_tests)

add_test(NAME kvds_tests COMMAND kvds_tests)

add_test(NAME kvds_basic_operations 
         COMMAND kvds_tests "kvds basic operations")
add_test(NAME kvds_with_manual_prefix_management 
         COMMAND kvds_tests "kvds with manual prefix management") 
add_test(NAME kvds_batch_operations 
         COMMAND kvds_tests "kvds batch operations")
add_test(NAME kvds_iteration 
         COMMAND kvds_tests "kvds iteration")
add_test(NAME kvds_error_conditions 
         COMMAND kvds_tests "kvds error conditions")

add_test(NAME distributor_memory_backed_store
         COMMAND kvds_tests "distributor memory-backed store")
add_test(NAME distributor_disk_backed_store
         COMMAND kvds_tests "distributor disk-backed store")
add_test(NAME distributor_multiple_stores
         COMMAND kvds_tests "distributor multiple stores")
add_test(NAME distributor_shared_obj_c_references
         COMMAND kvds_tests "distributor shared_obj_c references")
add_test(NAME distributor_lazy_cleanup
         COMMAND kvds_tests "distributor lazy cleanup")
add_test(NAME distributor_thread_safety
         COMMAND kvds_tests "distributor thread safety")
add_test(NAME distributor_batch_operations
         COMMAND kvds_tests "distributor batch operations")
add_test(NAME distributor_iteration
         COMMAND kvds_tests "distributor iteration")

if(WITH_ASAN)
  set_tests_properties(kvds_tests kvds_basic_operations kvds_with_manual_prefix_management 
                       kvds_batch_operations kvds_iteration kvds_error_conditions
                       distributor_memory_backed_store distributor_disk_backed_store
                       distributor_multiple_stores distributor_shared_obj_c_references
                       distributor_lazy_cleanup distributor_thread_safety
                       distributor_batch_operations distributor_iteration
    PROPERTIES 
      ENVIRONMENT "ASAN_OPTIONS=detect_container_overflow=0"
  )
endif()


