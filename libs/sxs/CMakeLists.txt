add_library(libsxs SHARED sxs.c eval.c slpcb.c)

target_include_directories(libsxs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libsxs PUBLIC slp)

install(TARGETS libsxs
    LIBRARY DESTINATION libs/sxs/lib
    ARCHIVE DESTINATION libs/sxs/lib
)

install(FILES sxs.h
    DESTINATION libs/sxs/include
)

