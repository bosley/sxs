add_executable(assembler main.cpp)

target_link_libraries(assembler PRIVATE
    fmt::fmt
    pkg::parser
)

set_target_properties(assembler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
