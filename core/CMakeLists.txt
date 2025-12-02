
add_library(pkg_core SHARED
    core.cpp
    interpreter.cpp
    context.cpp
    instructions/datum.cpp
    instructions/instructions.cpp
    instructions/generation/generation.cpp
    instructions/interpretation/interpretation.cpp
    instructions/typechecking/typechecking.cpp
    imports/imports.cpp
    kernels/kernels.cpp
    type_checker/type_checker.cpp
)

target_include_directories(pkg_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/root>
    $<INSTALL_INTERFACE:include/sxs>
)

target_link_libraries(pkg_core PUBLIC
    pkg::slp
    spdlog::spdlog
    fmt::fmt
)

add_library(pkg::core ALIAS pkg_core)

install(TARGETS pkg_core
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
    core.hpp
    interpreter.hpp
    context.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core
)

install(FILES
    instructions/datum.hpp
    instructions/instructions.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/instructions
)

install(FILES
    instructions/generation/generation.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/instructions/generation
)

install(FILES
    instructions/interpretation/interpretation.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/instructions/interpretation
)

install(FILES
    instructions/typechecking/typechecking.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/instructions/typechecking
)

install(FILES
    imports/imports.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/imports
)

install(FILES
    kernels/kernels.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/kernels
)

install(FILES
    type_checker/type_checker.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/core/type_checker
)

