add_executable(sxs
    main.cpp
)

target_link_libraries(sxs PRIVATE
    pkg::core
    spdlog::spdlog
)

install(TARGETS sxs DESTINATION bin)

