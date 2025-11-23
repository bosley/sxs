add_executable(example_slp main.cpp)

target_link_libraries(example_slp PRIVATE
    pkg::slp
)

set_target_properties(example_slp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

