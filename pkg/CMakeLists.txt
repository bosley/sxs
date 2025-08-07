add_library(pkg_random STATIC
    random/random.cpp
)

target_include_directories(pkg_random PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_random PUBLIC
    fmt::fmt
)

add_library(pkg::random ALIAS pkg_random)

add_library(pkg_bytes INTERFACE)

target_include_directories(pkg_bytes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::bytes ALIAS pkg_bytes)

add_library(pkg_types INTERFACE)

target_include_directories(pkg_types INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::types ALIAS pkg_types)
