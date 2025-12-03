cmake_minimum_required(VERSION 3.15)

option(USE_TCC "Use TCC compiler instead of system compiler" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer (only with Clang/GCC)" OFF)

if(USE_TCC)
    set(CMAKE_C_COMPILER "tcc")
    set(CMAKE_C_COMPILER_WORKS 1)
    if(ENABLE_ASAN)
        message(WARNING "AddressSanitizer not supported with TCC, disabling ASAN")
        set(ENABLE_ASAN OFF)
    endif()
endif()

project(sxs C)

if(USE_TCC)
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
    set(CMAKE_C_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> -shared <OBJECTS> -o <TARGET>")
    set(CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG "")
    set(CMAKE_C_OSX_CURRENT_VERSION_FLAG "")
    set(CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG "")
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/libs/buffer:${CMAKE_BINARY_DIR}/libs/slp:${CMAKE_BINARY_DIR}/libs/scanner")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_ASAN AND NOT USE_TCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
    message(STATUS "AddressSanitizer enabled (includes leak detection)")
endif()

if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.sxs" CACHE PATH "Install prefix" FORCE)
endif()

option(EXEC_TESTS "Execute tests during build and register with CTest" ON)

if(EXEC_TESTS)
    enable_testing()
endif()

find_library(LIBTCC_LIBRARY NAMES tcc libtcc PATHS /usr/local/lib /usr/lib)
find_path(LIBTCC_INCLUDE_DIR NAMES libtcc.h PATHS /usr/local/include /usr/include)

if(NOT LIBTCC_LIBRARY OR NOT LIBTCC_INCLUDE_DIR)
    message(FATAL_ERROR "libtcc not found. Please install TCC using deps/install.sh")
endif()

message(STATUS "Found libtcc: ${LIBTCC_LIBRARY}")
message(STATUS "libtcc include dir: ${LIBTCC_INCLUDE_DIR}")

include_directories(${CMAKE_SOURCE_DIR}/libs)

add_subdirectory(libs)
add_subdirectory(cmds)

