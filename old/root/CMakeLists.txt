add_library(pkg_slp STATIC
  slp/buffer.cpp
  slp/slp.cpp
)

target_include_directories(pkg_slp PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(pkg::slp ALIAS pkg_slp)

install(TARGETS pkg_slp
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
  slp/slp.hpp
  slp/buffer.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs/slp
)

install(FILES
  kernel_api.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sxs
)

