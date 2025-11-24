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

add_library(pkg_slp STATIC
    slp/slp.cpp
)

target_include_directories(pkg_slp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_slp PUBLIC
    fmt::fmt
)

add_library(pkg::slp ALIAS pkg_slp)

add_library(pkg_sconf STATIC
    sconf/sconf.cpp
)

target_include_directories(pkg_sconf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_sconf PUBLIC
    pkg::slp
    fmt::fmt
)

add_library(pkg::sconf ALIAS pkg_sconf)

add_library(pkg_runtime STATIC
    runtime/runtime.cpp
    runtime/entity/entity.cpp
    runtime/session/session.cpp
    runtime/system/system.cpp
    runtime/events/events.cpp
    runtime/processor.cpp
    runtime/fns/fns.cpp
    runtime/fns/kv.cpp
    runtime/fns/event.cpp
    runtime/fns/runtime_fns.cpp
)

target_include_directories(pkg_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_runtime PUBLIC
    fmt::fmt
    spdlog::spdlog
    pkg::kvds
    pkg::slp
    pkg::record
)

add_library(pkg::runtime ALIAS pkg_runtime)

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
