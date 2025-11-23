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

