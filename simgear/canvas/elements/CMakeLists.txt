# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2012 Thomas Geymayer

set(HEADERS
  CanvasElement.hxx
  CanvasGroup.hxx
  CanvasImage.hxx
  CanvasMap.hxx
  CanvasPath.hxx
  CanvasText.hxx
)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        CanvasElement.cxx
        CanvasGroup.cxx
        CanvasImage.cxx
        CanvasMap.cxx
        CanvasPath.cxx
        CanvasText.cxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/canvas/elements)
endif()

add_boost_test(canvas_element
  SOURCES canvas_element_test.cpp
  LIBRARIES SimGearScene
)
