add_executable(sup
    main.cpp
    project.cpp
    dep.cpp
    runtime.cpp
    clean.cpp
    tui/tui.cpp
)

target_link_libraries(sup PRIVATE
    pkg::core
    fmt::fmt
    spdlog::spdlog
    ftxui::screen
    ftxui::dom
    ftxui::component
)

install(TARGETS sup DESTINATION bin)

