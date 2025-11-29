# random
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

# bytes
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

# kvds
add_library(pkg_kvds STATIC
    kvds/memstore.cpp
    kvds/datastore.cpp
    kvds/distributor.cpp
)

target_include_directories(pkg_kvds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_kvds PUBLIC
    fmt::fmt
    RocksDB::rocksdb
)

add_library(pkg::kvds ALIAS pkg_kvds)

add_library(pkg_sconf STATIC
    sconf/sconf.cpp
)

target_include_directories(pkg_sconf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_sconf PUBLIC
    ${SXS_SLP_LIB}
    fmt::fmt
)

add_library(pkg::sconf ALIAS pkg_sconf)

add_library(pkg_record STATIC
    record/record.cpp
)

target_include_directories(pkg_record PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_record PUBLIC
    pkg::kvds
    pkg::sconf
    spdlog::spdlog
)

add_library(pkg::record ALIAS pkg_record)

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
