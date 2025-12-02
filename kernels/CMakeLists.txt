
add_library(kernel_alu SHARED
  alu/alu.cpp
)

target_include_directories(kernel_alu PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/root
)

target_compile_definitions(kernel_alu PRIVATE
  SXS_KERNEL_BUILD
)

target_link_libraries(kernel_alu PRIVATE
  pkg::slp
)

set_target_properties(kernel_alu PROPERTIES
  PREFIX "lib"
  OUTPUT_NAME "kernel_alu"
  SUFFIX ".dylib"
)

add_library(kernel_io SHARED
  io/io.cpp
)

target_include_directories(kernel_io PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/root
)

target_compile_definitions(kernel_io PRIVATE
  SXS_KERNEL_BUILD
)

target_link_libraries(kernel_io PRIVATE
  pkg::slp
)

set_target_properties(kernel_io PROPERTIES
  PREFIX "lib"
  OUTPUT_NAME "kernel_io"
  SUFFIX ".dylib"
)

add_library(kernel_random SHARED
  random/random.cpp
)

target_include_directories(kernel_random PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/root
  ${CMAKE_SOURCE_DIR}/integrants
)

target_compile_definitions(kernel_random PRIVATE
  SXS_KERNEL_BUILD
)

target_link_libraries(kernel_random PRIVATE
  pkg::random
  pkg::slp
)

set_target_properties(kernel_random PROPERTIES
  PREFIX "lib"
  OUTPUT_NAME "kernel_random"
  SUFFIX ".dylib"
)

add_library(kernel_kv SHARED
  kv/kv.cpp
)

target_include_directories(kernel_kv PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/root
  ${CMAKE_SOURCE_DIR}/integrants
)

target_compile_definitions(kernel_kv PRIVATE
  SXS_KERNEL_BUILD
)

target_link_libraries(kernel_kv PRIVATE
  pkg::kvds
  pkg::slp
)

set_target_properties(kernel_kv PROPERTIES
  PREFIX "lib"
  OUTPUT_NAME "kernel_kv"
  SUFFIX ".dylib"
)

add_library(kernel_forge SHARED
  forge/forge.cpp
)

target_include_directories(kernel_forge PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/root
)

target_compile_definitions(kernel_forge PRIVATE
  SXS_KERNEL_BUILD
)

target_link_libraries(kernel_forge PRIVATE
  pkg::slp
)

set_target_properties(kernel_forge PROPERTIES
  PREFIX "lib"
  OUTPUT_NAME "kernel_forge"
  SUFFIX ".dylib"
)

add_custom_target(build_kernels ALL
  DEPENDS kernel_alu kernel_io kernel_random kernel_kv kernel_forge
)

install(TARGETS kernel_alu
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/alu
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/alu
)

install(FILES alu/kernel.sxs
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/alu
)

install(TARGETS kernel_io
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/io
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/io
)

install(FILES io/kernel.sxs
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/io
)

install(TARGETS kernel_kv
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/kv
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/kv
)

install(FILES kv/kernel.sxs
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/kv
)

install(TARGETS kernel_random
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/random
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/random
)

install(FILES random/kernel.sxs
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/random
)

install(TARGETS kernel_forge
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/forge
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/forge
)

install(FILES forge/kernel.sxs
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/forge
)

install(FILES sxs/kernel.sxs
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/kernels/sxs
)

