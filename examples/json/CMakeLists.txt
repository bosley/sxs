add_executable(example_json main.cpp)

target_link_libraries(example_json PRIVATE
    extern::json
    fmt::fmt
)

set_target_properties(example_json PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)


