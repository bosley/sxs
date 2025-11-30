add_executable(events_test
  events_test.cpp
)

target_link_libraries(events_test PRIVATE
  pkg_events
  snitch::snitch
)

add_test(NAME events_test COMMAND events_test)

