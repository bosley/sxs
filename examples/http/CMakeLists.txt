add_executable(example_http main.cpp)

target_link_libraries(example_http PRIVATE
    extern::httplib
    fmt::fmt
)

if(APPLE)
    # Find and link required frameworks - CFHost functions are in CFNetwork
    find_library(CFNETWORK_LIBRARY CFNetwork)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration)

    if(CFNETWORK_LIBRARY)
        target_link_libraries(example_http PRIVATE ${CFNETWORK_LIBRARY})
    endif()

    if(COREFOUNDATION_LIBRARY)
        target_link_libraries(example_http PRIVATE ${COREFOUNDATION_LIBRARY})
    endif()

    if(SYSTEMCONFIGURATION_LIBRARY)
        target_link_libraries(example_http PRIVATE ${SYSTEMCONFIGURATION_LIBRARY})
    endif()
endif()

set_target_properties(example_http PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

