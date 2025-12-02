cmake_minimum_required (VERSION 3.15)

project(beau)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(RocksDB REQUIRED)

include(FetchContent)

if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
  snitch
  GIT_REPOSITORY https://github.com/cschreib/snitch.git
  GIT_TAG v1.2.5
)

set(SNITCH_DO_TEST OFF CACHE BOOL "" FORCE)

FetchContent_GetProperties(snitch)
if(NOT snitch_POPULATED)
  FetchContent_Populate(snitch)
  add_subdirectory(${snitch_SOURCE_DIR} ${snitch_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(DEFINED ENV{SXS_HOME})
    set(SXS_STD_ROOT "$ENV{SXS_HOME}" CACHE PATH "sxs-std installation directory")
else()
    set(SXS_STD_ROOT "$ENV{HOME}/.sxs" CACHE PATH "sxs-std installation directory")
endif()

include_directories(${SXS_STD_ROOT}/include)

find_library(SXS_SLP_LIB
    NAMES pkg_slp libpkg_slp
    PATHS ${SXS_STD_ROOT}/lib
    NO_DEFAULT_PATH
    REQUIRED
)

set(SXS_KERNEL_PATH "${SXS_STD_ROOT}/lib/kernels" CACHE PATH "Kernel modules directory")

option(RUN_TESTS      "Run tests" ON)
option(EXTRA_DEBUG_STMT     "Enable extra debug statements" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(TEST_DATA_DIR "${CMAKE_BINARY_DIR}/test_data")
add_compile_definitions(TEST_DATA_DIR="${TEST_DATA_DIR}")

#
# Options
#
option(WITH_ASAN     "Compile with ASAN" OFF)

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
# Build project subdirectories
#
add_subdirectory(${PROJECT_SOURCE_DIR}/pkg)
add_subdirectory(${PROJECT_SOURCE_DIR}/cmd)

#
# Custom target for release builds
#
add_custom_target(release
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DWITH_ASAN=OFF ${PROJECT_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release -j8
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Building release version without ASAN"
)
