add_library(slp SHARED slp.c)

target_include_directories(slp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(slp PUBLIC buffer scanner)

install(TARGETS slp
    LIBRARY DESTINATION libs/slp/lib
    ARCHIVE DESTINATION libs/slp/lib
)

install(FILES slp.h
    DESTINATION libs/slp/include
)

add_subdirectory(tests)

