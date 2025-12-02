
add_library(pkg_bytes INTERFACE)

target_include_directories(pkg_bytes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::bytes ALIAS pkg_bytes)

add_library(pkg_cache INTERFACE)

target_include_directories(pkg_cache INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::cache ALIAS pkg_cache)

add_library(pkg_random STATIC
  random/random.cpp
)

target_include_directories(pkg_random PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::random ALIAS pkg_random)

add_library(pkg_kvds STATIC
  kvds/datastore.cpp
  kvds/memstore.cpp
  kvds/distributor.cpp
)

target_include_directories(pkg_kvds PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pkg_kvds PUBLIC
  RocksDB::rocksdb
)

add_library(pkg::kvds ALIAS pkg_kvds)

add_library(pkg_types INTERFACE)

target_include_directories(pkg_types INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::types ALIAS pkg_types)

