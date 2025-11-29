
# bytes
add_library(pkg_bytes INTERFACE)

target_include_directories(pkg_bytes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::bytes ALIAS pkg_bytes)


add_library(pkg_core STATIC
    core/core.cpp
    core/interpreter.cpp
    core/datum/datum.cpp
    core/instructions/instructions.cpp
    core/imports/imports.cpp
    core/kernels/kernels.cpp
)

target_include_directories(pkg_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_core PUBLIC
    ${SXS_SLP_LIB}
    spdlog::spdlog
    fmt::fmt
)

add_library(pkg::core ALIAS pkg_core)
