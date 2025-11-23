add_executable(runtime_tests
  runtime_test.cpp
  entity_test.cpp
  session_test.cpp
  session_event_test.cpp
  events_test.cpp
  processor_test.cpp
  processor_event_sub_test.cpp
  processor_event_sub_stress_test.cpp
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

add_test(NAME session_subsystem_initialization
         COMMAND runtime_tests "session subsystem initialization")

add_test(NAME session_creation_with_entity_and_scope
         COMMAND runtime_tests "session creation with entity and scope")

add_test(NAME scoped_kv_operations_with_permission_checks
         COMMAND runtime_tests "scoped kv operations with permission checks")

add_test(NAME session_subsystem_management
         COMMAND runtime_tests "session subsystem management")

add_test(NAME session_lifecycle_management
         COMMAND runtime_tests "session lifecycle management")

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

add_test(NAME entity_topic_permission_granting
         COMMAND runtime_tests "entity topic permission granting")
add_test(NAME entity_topic_permission_revocation
         COMMAND runtime_tests "entity topic permission revocation")
add_test(NAME entity_topic_permission_persistence
         COMMAND runtime_tests "entity topic permission persistence")
add_test(NAME session_publish_with_permissions
         COMMAND runtime_tests "session publish with permissions")
add_test(NAME session_subscribe_with_permissions
         COMMAND runtime_tests "session subscribe with permissions")
add_test(NAME session_event_publish_and_consume
         COMMAND runtime_tests "session event publish and consume")
add_test(NAME session_unsubscribe_from_topics
         COMMAND runtime_tests "session unsubscribe from topics")
add_test(NAME session_multiple_topic_subscriptions
         COMMAND runtime_tests "session multiple topic subscriptions")
add_test(NAME session_event_payload_types
         COMMAND runtime_tests "session event payload types")
add_test(NAME session_event_origin_verification
         COMMAND runtime_tests "session event origin verification")
add_test(NAME bidirectional_communication_on_same_topic
         COMMAND runtime_tests "bidirectional communication on same topic")
add_test(NAME topic_isolation_prevents_cross_topic_leakage
         COMMAND runtime_tests "topic isolation prevents cross-topic leakage")
add_test(NAME mixed_permissions_on_same_topic
         COMMAND runtime_tests "mixed permissions on same topic")
add_test(NAME multiple_sessions_per_entity_receive_events
         COMMAND runtime_tests "multiple sessions per entity receive events")

add_test(NAME processor_initialization
         COMMAND runtime_tests "processor initialization")
add_test(NAME processor_execute_simple_integer_script
         COMMAND runtime_tests "processor execute simple integer script")
add_test(NAME processor_kv_set_and_kv_get_operations
         COMMAND runtime_tests "processor kv/set and kv/get operations")
add_test(NAME processor_kv_del_and_kv_exists_operations
         COMMAND runtime_tests "processor kv/del and kv/exists operations")
add_test(NAME processor_event_pub_operation
         COMMAND runtime_tests "processor event/pub operation")
add_test(NAME processor_event_sub_operation
         COMMAND runtime_tests "processor event/sub operation")
add_test(NAME processor_runtime_log_operation
         COMMAND runtime_tests "processor runtime/log operation")
add_test(NAME processor_error_handling
         COMMAND runtime_tests "processor error handling")
add_test(NAME processor_permission_denied_scenarios
         COMMAND runtime_tests "processor permission denied scenarios")
add_test(NAME processor_bracket_list_execution
         COMMAND runtime_tests "processor bracket list execution")
add_test(NAME processor_complex_script_execution
         COMMAND runtime_tests "processor complex script execution")
add_test(NAME event_sub_with_handler_body_executes_on_event
         COMMAND runtime_tests "event/sub with handler body executes on event")
add_test(NAME multiple_sessions_subscribe_to_same_topic
         COMMAND runtime_tests "multiple sessions subscribe to same topic")
add_test(NAME session_subscribes_to_multiple_topics
         COMMAND runtime_tests "session subscribes to multiple topics")
add_test(NAME rapid_fire_event_delivery_to_handler
         COMMAND runtime_tests "rapid fire event delivery to handler")
add_test(NAME handler_with_parse_error_in_body
         COMMAND runtime_tests "handler with parse error in body")
add_test(NAME handler_with_nested_function_calls
         COMMAND runtime_tests "handler with nested function calls")
add_test(NAME handler_publishes_event_creating_chain
         COMMAND runtime_tests "handler publishes event creating chain")
add_test(NAME empty_handler_body
         COMMAND runtime_tests "empty handler body")

if(WITH_ASAN)
  set_tests_properties(runtime_tests runtime_basic_operations
                       entity_basic_operations
                       entity_creation_and_basic_properties entity_permission_granting
                       entity_permission_checking entity_permission_revocation
                       entity_permission_persistence entity_get_and_set_permissions
                       multiple_entities_with_different_permissions entity_permission_edge_cases
                       entity_deletion
                       entity_topic_permission_granting entity_topic_permission_revocation
                       entity_topic_permission_persistence
                       session_subsystem_initialization session_creation_with_entity_and_scope
                       scoped_kv_operations_with_permission_checks session_subsystem_management
                       session_lifecycle_management
                       session_publish_with_permissions session_subscribe_with_permissions
                       session_event_publish_and_consume session_unsubscribe_from_topics
                       session_multiple_topic_subscriptions session_event_payload_types
                       session_event_origin_verification
                       bidirectional_communication_on_same_topic topic_isolation_prevents_cross_topic_leakage
                       mixed_permissions_on_same_topic multiple_sessions_per_entity_receive_events
                       event_system_basic_operations event_producer_and_topic_writer
                       event_consumer_registration event_publishing_and_consumption
                       multi_threaded_event_processing event_system_error_handling
                       event_system_shutdown_behavior event_system_origin_handling
                       processor_initialization processor_execute_simple_integer_script
                       processor_kv_set_and_kv_get_operations processor_kv_del_and_kv_exists_operations
                       processor_event_pub_operation processor_event_sub_operation
                       processor_runtime_log_operation processor_error_handling
                       processor_permission_denied_scenarios processor_bracket_list_execution
                       processor_complex_script_execution
                       event_sub_with_handler_body_executes_on_event
                       multiple_sessions_subscribe_to_same_topic session_subscribes_to_multiple_topics
                       rapid_fire_event_delivery_to_handler handler_with_parse_error_in_body
                       handler_with_nested_function_calls handler_publishes_event_creating_chain
                       empty_handler_body
    PROPERTIES 
      ENVIRONMENT "ASAN_OPTIONS=detect_container_overflow=0"
  )
endif()

