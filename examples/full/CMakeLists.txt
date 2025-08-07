add_executable(example_full main.cpp)

target_link_libraries(example_full PRIVATE
    pkg::random
    pkg::bytes
    pkg::types
    fmt::fmt
)

set_target_properties(example_full PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
