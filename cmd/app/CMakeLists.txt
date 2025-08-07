add_executable(app main.cpp)

target_link_libraries(app PRIVATE
    fmt::fmt
)

set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
