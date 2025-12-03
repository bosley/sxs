add_library(testlib INTERFACE)

target_include_directories(testlib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

install(FILES test.h
    DESTINATION libs/testlib/include
)

