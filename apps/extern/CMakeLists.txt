set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib)
add_library(extern::httplib ALIAS httplib)

set(SNITCH_DO_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/snitch)
add_library(extern::snitch ALIAS snitch)
 
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/json)
add_library(extern::json ALIAS nlohmann_json)