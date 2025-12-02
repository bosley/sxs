add_executable(sxs
    main.cpp
)

target_link_libraries(sxs PRIVATE
    pkg::core
    pkg::slp
    spdlog::spdlog
    fmt::fmt
)

set_target_properties(sxs PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

install(TARGETS sxs
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
