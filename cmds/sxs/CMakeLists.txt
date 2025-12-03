add_executable(sxs main.c)

target_link_libraries(sxs PRIVATE
    buffer
    slp
    ${LIBTCC_LIBRARY}
)

target_include_directories(sxs PRIVATE
    ${LIBTCC_INCLUDE_DIR}
)

install(TARGETS sxs
    RUNTIME DESTINATION bin
)

