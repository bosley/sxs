file(GLOB TEST_SOURCES "*.c")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_link_libraries(${TEST_NAME} PRIVATE slp testlib)
    
    if(EXEC_TESTS)
        add_custom_command(TARGET ${TEST_NAME} POST_BUILD
            COMMAND ${TEST_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running test: ${TEST_NAME}"
        )
        
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
    
    install(TARGETS ${TEST_NAME}
        RUNTIME DESTINATION libs/slp/tests
    )
endforeach()

