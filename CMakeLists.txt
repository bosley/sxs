cmake_minimum_required (VERSION 3.15)

project(beau)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(RocksDB REQUIRED)

# Find and configure zstd for cpp-httplib
find_package(PkgConfig REQUIRED)
pkg_check_modules(zstd REQUIRED IMPORTED_TARGET libzstd)

# Create the zstd::libzstd target that cpp-httplib expects
if(TARGET PkgConfig::zstd)
    add_library(zstd::libzstd ALIAS PkgConfig::zstd)
    message(STATUS "Created zstd::libzstd target from pkg-config")
endif()

option(RUN_TESTS      "Run tests" ON)
option(EXTRA_DEBUG_STMT     "Enable extra debug statements" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

#
# Options
#
option(WITH_ASAN     "Compile with ASAN" ON)

#
# Setup build type 'Release vs Debug'
#
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

#
# Setup ASAN
#
if(WITH_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

#
# Add special definitions for specific debugging functions
#
if (EXTRA_DEBUG_STMT)
  add_compile_definitions(EXTRA_DEBUG=1)
else()
  add_compile_definitions(EXTRA_DEBUG=0)
endif()

#
# Add special definitions for specific debugging functions
#
if (RUN_TESTS)
  message(STATUS "Running tests")
  enable_testing()
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

#
# Include meta information
#
include(${PROJECT_SOURCE_DIR}/cmake/CheckGit.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Platform.cmake)

#
# External code setup
#
# Bring in external dependencies
add_subdirectory(${PROJECT_SOURCE_DIR}/extern)
add_subdirectory(${PROJECT_SOURCE_DIR}/pkg)
add_subdirectory(${PROJECT_SOURCE_DIR}/cmd)
add_subdirectory(${PROJECT_SOURCE_DIR}/examples)

