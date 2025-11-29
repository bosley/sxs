add_executable(sxs-rt
    main.cpp
    project.cpp
    dep.cpp
    runtime.cpp
    clean.cpp
)

target_link_libraries(sxs-rt PRIVATE
    pkg::core
    fmt::fmt
    spdlog::spdlog
)

install(TARGETS sxs-rt DESTINATION bin)

