add_executable(example_http main.cpp)

target_link_libraries(example_http PRIVATE
    extern::httplib
    fmt::fmt
)

set_target_properties(example_http PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

