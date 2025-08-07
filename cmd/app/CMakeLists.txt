add_executable(ytho_app main.cpp)

target_link_libraries(ytho_app PRIVATE
    fmt::fmt
)

set_target_properties(ytho_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
