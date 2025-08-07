add_executable(example_random main.cpp)

target_link_libraries(example_random PRIVATE
    pkg::random
    fmt::fmt
)

set_target_properties(example_random PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
