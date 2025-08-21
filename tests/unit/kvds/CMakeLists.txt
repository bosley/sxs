add_executable(kvds_tests
  kvds_test.cpp
  memstore_test.cpp
)

target_link_libraries(kvds_tests PRIVATE 
  extern::snitch
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

# Set ASAN options for all kvds tests when AddressSanitizer is enabled
# This prevents false positive container overflow errors from RocksDB's thread pool
#
# Set ASAN options to avoid false positives with pre-compiled libraries like RocksDB
# This needs to be set as an environment variable when tests run
# 
# RATIONALE: RocksDB (installed via Homebrew) is compiled without AddressSanitizer,
# while our code is compiled with it. This causes false positive container overflow
# errors in RocksDB's internal thread pool (std::vector<std::thread>) during DB::Open.
# 
# CONFIRMED BY:
# - Memstore tests (using only std::map) pass without ASAN errors
# - All datastore tests fail with identical ASAN errors in rocksdb::VersionBuilder
# - Minimal standalone RocksDB test passes without errors
# - Error only occurs when RocksDB is linked with ASAN-instrumented code

if(WITH_ASAN)
  set_tests_properties(kvds_tests kvds_basic_operations kvds_with_manual_prefix_management 
                       kvds_batch_operations kvds_iteration kvds_error_conditions
    PROPERTIES 
      ENVIRONMENT "ASAN_OPTIONS=detect_container_overflow=0"
  )
endif()


