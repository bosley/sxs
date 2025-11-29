add_executable(sup
    main.cpp
    project.cpp
    dep.cpp
    runtime.cpp
    clean.cpp
)

target_link_libraries(sup PRIVATE
    pkg::core
    fmt::fmt
    spdlog::spdlog
)

install(TARGETS sup DESTINATION bin)

