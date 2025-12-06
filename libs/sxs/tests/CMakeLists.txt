file(GLOB TEST_SOURCES "*.c")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_link_libraries(${TEST_NAME} PRIVATE libsxs slp testlib)
    
    if(EXEC_TESTS)
        add_custom_command(TARGET ${TEST_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libs/map:${CMAKE_BINARY_DIR}/libs/sxs:${CMAKE_BINARY_DIR}/libs/slp:${CMAKE_BINARY_DIR}/libs/buffer:${CMAKE_BINARY_DIR}/libs/scanner $<TARGET_FILE:${TEST_NAME}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running test: ${TEST_NAME}"
        )
        
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        set_tests_properties(${TEST_NAME} PROPERTIES
            ENVIRONMENT "DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libs/map:${CMAKE_BINARY_DIR}/libs/sxs:${CMAKE_BINARY_DIR}/libs/slp:${CMAKE_BINARY_DIR}/libs/buffer:${CMAKE_BINARY_DIR}/libs/scanner"
        )
    endif()
    
    install(TARGETS ${TEST_NAME}
        RUNTIME DESTINATION libs/sxs/tests
    )
endforeach()

