add_executable(sup
    main.cpp
    clean.cpp
    dep.cpp
    project.cpp
    runtime.cpp
)

target_link_libraries(sup PRIVATE
    pkg::core
    pkg::slp
    spdlog::spdlog
    fmt::fmt
)

set_target_properties(sup PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

install(TARGETS sup
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
