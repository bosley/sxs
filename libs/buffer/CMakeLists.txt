add_library(buffer SHARED buffer.c)

target_include_directories(buffer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS buffer
    LIBRARY DESTINATION libs/buffer/lib
    ARCHIVE DESTINATION libs/buffer/lib
)

install(FILES buffer.h
    DESTINATION libs/buffer/include
)

add_subdirectory(tests)

