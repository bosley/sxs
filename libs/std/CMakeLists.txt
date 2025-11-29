add_library(kernel_alu SHARED
  alu/alu.cpp
)

target_include_directories(kernel_alu PRIVATE
  ${CMAKE_SOURCE_DIR}/pkg
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
  ${CMAKE_SOURCE_DIR}/pkg
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
  ${CMAKE_SOURCE_DIR}/pkg
)

target_link_libraries(kernel_random PRIVATE
  pkg::random
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
  ${CMAKE_SOURCE_DIR}/pkg
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

add_custom_target(build_kernels ALL
  DEPENDS kernel_alu kernel_io kernel_random kernel_kv
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

