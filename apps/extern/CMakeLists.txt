set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib)
add_library(extern::httplib ALIAS httplib)

find_library(SNITCH_LIB
    NAMES snitch libsnitch
    PATHS ${SXS_STD_ROOT}/lib
    NO_DEFAULT_PATH
    REQUIRED
)
add_library(snitch_imported STATIC IMPORTED GLOBAL)
set_target_properties(snitch_imported PROPERTIES 
    IMPORTED_LOCATION ${SNITCH_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${SXS_STD_ROOT}/include
)
add_library(extern::snitch ALIAS snitch_imported)
add_library(snitch::snitch ALIAS snitch_imported)
 
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/json)
add_library(extern::json ALIAS nlohmann_json)