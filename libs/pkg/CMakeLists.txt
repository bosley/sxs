add_library(pkg_random STATIC
  random/random.cpp
)

target_include_directories(pkg_random PUBLIC
  ${CMAKE_SOURCE_DIR}/pkg
)

add_library(pkg_slp STATIC
  slp/buffer.cpp
  slp/slp.cpp
)

target_include_directories(pkg_slp PUBLIC
  ${CMAKE_SOURCE_DIR}/pkg
)

add_library(pkg_kvds STATIC
  kvds/datastore.cpp
  kvds/memstore.cpp
  kvds/distributor.cpp
)

target_include_directories(pkg_kvds PUBLIC
  ${CMAKE_SOURCE_DIR}/pkg
)

target_link_libraries(pkg_kvds PUBLIC
  RocksDB::rocksdb
)

add_library(pkg_events STATIC
  events/events.cpp
)

target_include_directories(pkg_events PUBLIC
  ${CMAKE_SOURCE_DIR}/pkg
)

target_link_libraries(pkg_events PUBLIC
  spdlog::spdlog
)

add_library(pkg::random ALIAS pkg_random)
add_library(pkg::slp ALIAS pkg_slp)
add_library(pkg::kvds ALIAS pkg_kvds)
add_library(pkg::events ALIAS pkg_events)

install(TARGETS pkg_slp
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
  slp/slp.hpp
  slp/buffer.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/slp
)

install(FILES
  kernel_api.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs
)

