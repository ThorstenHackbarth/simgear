# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: Stuart Buchanan <stuart13@gmail.com>
add_library(FGTHinksFMM INTERFACE)

target_include_directories(FGTHinksFMM
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)

install(TARGETS FGTHinksFMM
  EXPORT SimGearTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES fast_marching_method.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
