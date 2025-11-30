add_executable(cache_tests
  cache_test.cpp
)

target_link_libraries(cache_tests PRIVATE 
  snitch::snitch
)

target_include_directories(cache_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/pkg
)

add_dependencies(build_tests cache_tests)

add_test(NAME cache_tests COMMAND cache_tests)

