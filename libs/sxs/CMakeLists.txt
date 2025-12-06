add_library(libsxs SHARED sxs.c eval.c slpcb.c forms.c builtins.c errors.c typecheck.c typecheck_callbacks.c impls/load_store.c impls/debug.c impls/rotate.c impls/insist.c impls/catch.c impls/insist_typecheck.c impls/load_store_typecheck.c impls/proc.c impls/proc_typecheck.c impls/do.c)

target_include_directories(libsxs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libsxs PUBLIC slp map ctx)

install(TARGETS libsxs
    LIBRARY DESTINATION libs/sxs/lib
    ARCHIVE DESTINATION libs/sxs/lib
)

install(FILES sxs.h
    DESTINATION libs/sxs/include
)

add_subdirectory(tests)
