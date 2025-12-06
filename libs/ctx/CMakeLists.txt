add_library(ctx SHARED ctx.c)

target_include_directories(ctx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ctx PUBLIC map slp)

install(TARGETS ctx
    LIBRARY DESTINATION libs/ctx/lib
    ARCHIVE DESTINATION libs/ctx/lib
)

install(FILES ctx.h
    DESTINATION libs/ctx/include
)

add_subdirectory(tests)

