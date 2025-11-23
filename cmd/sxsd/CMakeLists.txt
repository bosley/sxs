add_executable(sxsd main.cpp)

target_link_libraries(sxsd PRIVATE
    fmt::fmt
    extern::json
    pkg::runtime
)

set_target_properties(sxsd PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
