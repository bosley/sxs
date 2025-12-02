cmake_minimum_required(VERSION 3.20)

project(sxs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED ENV{SXS_HOME})
    set(CMAKE_INSTALL_PREFIX "$ENV{SXS_HOME}" CACHE PATH "Installation directory" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.sxs" CACHE PATH "Installation directory" FORCE)
endif()

include(GNUInstallDirs)

option(WITH_ASAN "Enable AddressSanitizer" OFF)
option(RUN_TESTS "Run tests" ON)
option(EXTRA_DEBUG_STMT "Enable extra debug statements" OFF)

if(WITH_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

if(EXTRA_DEBUG_STMT)
  add_compile_definitions(EXTRA_DEBUG=1)
else()
  add_compile_definitions(EXTRA_DEBUG=0)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

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
  
  set(CMAKE_INSTALL_INCLUDEDIR_SAVE ${CMAKE_INSTALL_INCLUDEDIR})
  set(CMAKE_INSTALL_LIBDIR_SAVE ${CMAKE_INSTALL_LIBDIR})
  
  set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_BINARY_DIR}/_deps/_snitch_dummy/include")
  set(CMAKE_INSTALL_LIBDIR "${CMAKE_BINARY_DIR}/_deps/_snitch_dummy/lib")
  
  add_subdirectory(${snitch_SOURCE_DIR} ${snitch_BINARY_DIR} EXCLUDE_FROM_ALL)
  
  set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR_SAVE})
  set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR_SAVE})
endif()

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(RocksDB REQUIRED)

set(SXS_KERNEL_PATH "${CMAKE_INSTALL_PREFIX}/lib/kernels" CACHE PATH "Kernel modules directory")
set(TEST_DATA_DIR "${CMAKE_BINARY_DIR}/test_data")
add_compile_definitions(TEST_DATA_DIR="${TEST_DATA_DIR}")

include(${PROJECT_SOURCE_DIR}/cmake/CheckGit.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Platform.cmake)

add_subdirectory(root)
add_subdirectory(core)
add_subdirectory(integrants)
add_subdirectory(kernels)
add_subdirectory(sxs)
add_subdirectory(sup)

if(RUN_TESTS)
  message(STATUS "Tests enabled")
  enable_testing()
  add_subdirectory(tests)
endif()

add_custom_target(release
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DWITH_ASAN=OFF ${PROJECT_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release -j8
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Building release version without ASAN"
)

