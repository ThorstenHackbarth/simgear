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

if(TARGET SimGearScene AND ENABLE_TESTS)
  add_simgear_scene_autotest(canvas_element canvas_element_test.cpp)
endif()
