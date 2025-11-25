add_executable(runtime_tests
  runtime_test.cpp
  entity_test.cpp
  entity_rps_test.cpp
  entity_rps_await_test.cpp
  session_test.cpp
  session_event_test.cpp
  events_test.cpp
  processor_test.cpp
  processor_event_sub_test.cpp
  processor_event_sub_stress_test.cpp
  processor_kv_atomic_test.cpp
  processor_kv_context_test.cpp
  multi_processor_test.cpp
  insist_test.cpp
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

add_test(NAME entity_rps_basic_set_and_get
         COMMAND runtime_tests "entity rps basic set and get")
add_test(NAME entity_rps_unlimited_publishing
         COMMAND runtime_tests "entity rps unlimited publishing")
add_test(NAME entity_rps_single_session_rate_limiting
         COMMAND runtime_tests "entity rps single session rate limiting")
add_test(NAME entity_rps_multiple_sessions_share_limit
         COMMAND runtime_tests "entity rps multiple sessions share limit")
add_test(NAME entity_rps_concurrent_multi_threaded_publishing
         COMMAND runtime_tests "entity rps concurrent multi-threaded publishing")
add_test(NAME entity_rps_sliding_window_behavior
         COMMAND runtime_tests "entity rps sliding window behavior")
add_test(NAME entity_rps_edge_cases
         COMMAND runtime_tests "entity rps edge cases")
add_test(NAME entity_rps_with_permission_blocking
         COMMAND runtime_tests "entity rps with permission blocking")
add_test(NAME entity_rps_stress_test_with_rapid_publishes
         COMMAND runtime_tests "entity rps stress test with rapid publishes")
add_test(NAME entity_rps_different_entities_independent_limits
         COMMAND runtime_tests "entity rps different entities independent limits")
add_test(NAME entity_rps_with_runtime_await_cross_channel_limiting
         COMMAND runtime_tests "entity rps with core/expr/await cross-channel limiting")
add_test(NAME entity_rps_limit_tracking_across_channels
         COMMAND runtime_tests "entity rps limit tracking across channels")

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
         COMMAND runtime_tests "processor core/kv/set and core/kv/get operations")
add_test(NAME processor_kv_del_and_kv_exists_operations
         COMMAND runtime_tests "processor core/kv/del and core/kv/exists operations")
add_test(NAME processor_kv_snx_sets_key_only_if_not_exists
         COMMAND runtime_tests "core/kv/snx sets key only if not exists")
add_test(NAME processor_kv_cas_compares_and_swaps_atomically
         COMMAND runtime_tests "core/kv/cas compares and swaps atomically")
add_test(NAME processor_kv_iterate_processes_keys_with_prefix
         COMMAND runtime_tests "core/kv/iterate processes keys with prefix")
add_test(NAME processor_event_pub_operation
         COMMAND runtime_tests "processor core/event/pub operation")
add_test(NAME processor_event_sub_operation
         COMMAND runtime_tests "processor core/event/sub operation")
add_test(NAME processor_runtime_log_operation
         COMMAND runtime_tests "processor core/util/log operation")
add_test(NAME processor_error_handling
         COMMAND runtime_tests "processor error handling")
add_test(NAME processor_permission_denied_scenarios
         COMMAND runtime_tests "processor permission denied scenarios")
add_test(NAME processor_bracket_list_execution
         COMMAND runtime_tests "processor bracket list execution")
add_test(NAME processor_complex_script_execution
         COMMAND runtime_tests "processor complex script execution")
add_test(NAME event_sub_with_handler_body_executes_on_event
         COMMAND runtime_tests "core/event/sub with handler body executes on event")
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

add_test(NAME processor_runtime_eval_operation
         COMMAND runtime_tests "processor core/expr/eval operation")
add_test(NAME processor_runtime_await_operation
         COMMAND runtime_tests "processor core/expr/await operation")
add_test(NAME processor_runtime_eval_with_kv_operations
         COMMAND runtime_tests "processor core/expr/eval with kv operations")
add_test(NAME processor_runtime_await_async_communication
         COMMAND runtime_tests "processor core/expr/await async communication")

add_test(NAME insist_passes_through_non_error_values
         COMMAND runtime_tests "core/util/insist passes through non-error values")
add_test(NAME insist_halts_execution_on_error_object
         COMMAND runtime_tests "core/util/insist halts execution on ERROR object")
add_test(NAME insist_enables_safe_type_patterns
         COMMAND runtime_tests "core/util/insist enables safe type patterns")
add_test(NAME insist_with_bracket_list_stops_on_first_error
         COMMAND runtime_tests "core/util/insist with bracket list stops on first error")

add_test(NAME kv_context_basic_iterate_with_key_del
         COMMAND runtime_tests "basic iterate with $key del")
add_test(NAME kv_context_iterate_with_key_exists_check
         COMMAND runtime_tests "iterate with $key exists check")
add_test(NAME kv_context_iterate_with_key_load
         COMMAND runtime_tests "iterate with $key load")
add_test(NAME kv_context_combined_del_exists_load
         COMMAND runtime_tests "combined del + exists + load in iteration")
add_test(NAME kv_context_insist_with_key_operations
         COMMAND runtime_tests "insist with $key operations in iteration")
add_test(NAME kv_context_insist_failure_with_key
         COMMAND runtime_tests "insist failure with $key in iteration")
add_test(NAME kv_context_event_handler_with_key
         COMMAND runtime_tests "event handler with $key operations")
add_test(NAME kv_context_complex_integration
         COMMAND runtime_tests "complex integration: events + iterate + insist")
add_test(NAME kv_context_error_key_not_available
         COMMAND runtime_tests "error case: $key not available")
add_test(NAME kv_context_error_insist_with_missing_key
         COMMAND runtime_tests "error case: insist with missing $key")
add_test(NAME kv_context_exists_returning_false
         COMMAND runtime_tests "iterate with $key exists returning false")
add_test(NAME kv_context_loads_all_values_separately
         COMMAND runtime_tests "iterate loads all values into separate keys")
add_test(NAME kv_context_vs_literal_key_behavior
         COMMAND runtime_tests "context variable vs literal key behavior")

add_test(NAME multi_processor_initialization_with_1_processor
         COMMAND runtime_tests "multi processor initialization with 1 processor")
add_test(NAME multi_processor_initialization_with_4_processors
         COMMAND runtime_tests "multi processor initialization with 4 processors")
add_test(NAME multi_processor_concurrent_execution_on_different_topics
         COMMAND runtime_tests "multi processor concurrent execution on different topics")
add_test(NAME multi_processor_topic_isolation
         COMMAND runtime_tests "multi processor topic isolation")
add_test(NAME multi_processor_with_kv_operations_on_different_scopes
         COMMAND runtime_tests "multi processor with kv operations on different scopes")
add_test(NAME multi_processor_stress_test_with_rapid_concurrent_requests
         COMMAND runtime_tests "multi processor stress test with rapid concurrent requests")

if(WITH_ASAN)
  set_tests_properties(runtime_tests runtime_basic_operations
                       entity_basic_operations
                       entity_creation_and_basic_properties entity_permission_granting
                       entity_permission_checking entity_permission_revocation
                       entity_permission_persistence entity_get_and_set_permissions
                       multiple_entities_with_different_permissions entity_permission_edge_cases
                       entity_deletion
                       entity_rps_basic_set_and_get entity_rps_unlimited_publishing
                       entity_rps_single_session_rate_limiting entity_rps_multiple_sessions_share_limit
                       entity_rps_concurrent_multi_threaded_publishing entity_rps_sliding_window_behavior
                       entity_rps_edge_cases entity_rps_with_permission_blocking
                       entity_rps_stress_test_with_rapid_publishes entity_rps_different_entities_independent_limits
                       entity_rps_with_runtime_await_cross_channel_limiting entity_rps_limit_tracking_across_channels
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
                       processor_kv_snx_sets_key_only_if_not_exists processor_kv_cas_compares_and_swaps_atomically
                       processor_kv_iterate_processes_keys_with_prefix
                       processor_event_pub_operation processor_event_sub_operation
                       processor_runtime_log_operation processor_error_handling
                       processor_permission_denied_scenarios processor_bracket_list_execution
                       processor_complex_script_execution
                       event_sub_with_handler_body_executes_on_event
                       multiple_sessions_subscribe_to_same_topic session_subscribes_to_multiple_topics
                       rapid_fire_event_delivery_to_handler handler_with_parse_error_in_body
                       handler_with_nested_function_calls handler_publishes_event_creating_chain
                       empty_handler_body
                       processor_runtime_eval_operation processor_runtime_await_operation
                       processor_runtime_eval_with_kv_operations processor_runtime_await_async_communication
                       insist_passes_through_non_error_values insist_halts_execution_on_error_object
                       insist_enables_safe_type_patterns insist_with_bracket_list_stops_on_first_error
                       kv_context_basic_iterate_with_key_del kv_context_iterate_with_key_exists_check
                       kv_context_iterate_with_key_load kv_context_combined_del_exists_load
                       kv_context_insist_with_key_operations kv_context_insist_failure_with_key
                       kv_context_event_handler_with_key kv_context_complex_integration
                       kv_context_error_key_not_available kv_context_error_insist_with_missing_key
                       kv_context_exists_returning_false kv_context_loads_all_values_separately
                       kv_context_vs_literal_key_behavior
                       multi_processor_initialization_with_1_processor
                       multi_processor_initialization_with_4_processors
                       multi_processor_concurrent_execution_on_different_topics
                       multi_processor_topic_isolation
                       multi_processor_with_kv_operations_on_different_scopes
                       multi_processor_stress_test_with_rapid_concurrent_requests
    PROPERTIES 
      ENVIRONMENT "ASAN_OPTIONS=detect_container_overflow=0"
  )
endif()

