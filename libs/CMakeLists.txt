cmake_minimum_required(VERSION 3.20)
project(sxs-std VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.sxs" CACHE PATH "Installation directory" FORCE)

include(GNUInstallDirs)

option(WITH_ASAN "Enable AddressSanitizer" OFF)

if(WITH_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

include(FetchContent)

FetchContent_Declare(
  snitch
  GIT_REPOSITORY https://github.com/cschreib/snitch.git
  GIT_TAG v1.2.5
)

set(SNITCH_DO_NOT_INSTALL ON CACHE INTERNAL "")

FetchContent_GetProperties(snitch)
if(NOT snitch_POPULATED)
  FetchContent_Populate(snitch)
  add_subdirectory(${snitch_SOURCE_DIR} ${snitch_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

find_package(RocksDB REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/pkg)

enable_testing()

add_subdirectory(pkg)
add_subdirectory(tests)
add_subdirectory(std)

if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

