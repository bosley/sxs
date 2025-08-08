add_executable(oserv main.cpp)

target_link_libraries(oserv PRIVATE
    fmt::fmt
    extern::json
)

set_target_properties(oserv PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
