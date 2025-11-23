add_executable(runtime_tests
  runtime_test.cpp
  entity_test.cpp
  session_test.cpp
  events_test.cpp
)

target_link_libraries(runtime_tests PRIVATE 
  extern::snitch
  pkg::runtime
)

add_dependencies(build_tests runtime_tests)

add_test(NAME runtime_tests COMMAND runtime_tests)

add_test(NAME runtime_basic_operations 
         COMMAND runtime_tests "runtime basic operations")

add_test(NAME entity_basic_operations 
         COMMAND runtime_tests "entity basic operations")

add_test(NAME session_basic_operations 
         COMMAND runtime_tests "session basic operations")

add_test(NAME event_system_basic_operations 
         COMMAND runtime_tests "event system basic operations")

add_test(NAME event_producer_and_topic_writer 
         COMMAND runtime_tests "event producer and topic writer")

add_test(NAME event_consumer_registration 
         COMMAND runtime_tests "event consumer registration")

add_test(NAME event_publishing_and_consumption 
         COMMAND runtime_tests "event publishing and consumption")

add_test(NAME multi_threaded_event_processing 
         COMMAND runtime_tests "multi-threaded event processing")

add_test(NAME event_system_error_handling 
         COMMAND runtime_tests "event system error handling")

add_test(NAME event_system_shutdown_behavior 
         COMMAND runtime_tests "event system shutdown behavior")

add_test(NAME event_system_origin_handling 
         COMMAND runtime_tests "event system origin handling")

add_test(NAME entity_creation_and_basic_properties
         COMMAND runtime_tests "entity creation and basic properties")
add_test(NAME entity_permission_granting
         COMMAND runtime_tests "entity permission granting")
add_test(NAME entity_permission_checking
         COMMAND runtime_tests "entity permission checking")
add_test(NAME entity_permission_revocation
         COMMAND runtime_tests "entity permission revocation")
add_test(NAME entity_permission_persistence
         COMMAND runtime_tests "entity permission persistence")
add_test(NAME entity_get_and_set_permissions
         COMMAND runtime_tests "entity get and set permissions")
add_test(NAME multiple_entities_with_different_permissions
         COMMAND runtime_tests "multiple entities with different permissions")
add_test(NAME entity_permission_edge_cases
         COMMAND runtime_tests "entity permission edge cases")
add_test(NAME entity_deletion
         COMMAND runtime_tests "entity deletion")

if(WITH_ASAN)
  set_tests_properties(runtime_tests runtime_basic_operations
                       entity_basic_operations
                       entity_creation_and_basic_properties entity_permission_granting
                       entity_permission_checking entity_permission_revocation
                       entity_permission_persistence entity_get_and_set_permissions
                       multiple_entities_with_different_permissions entity_permission_edge_cases
                       entity_deletion
                       session_basic_operations
                       event_system_basic_operations event_producer_and_topic_writer
                       event_consumer_registration event_publishing_and_consumption
                       multi_threaded_event_processing event_system_error_handling
                       event_system_shutdown_behavior event_system_origin_handling
    PROPERTIES 
      ENVIRONMENT "ASAN_OPTIONS=detect_container_overflow=0"
  )
endif()

