# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2012 Thomas Geymayer

set(HEADERS
  canvas_fwd.hxx
  Canvas.hxx
  CanvasEvent.hxx
  CanvasMgr.hxx
  CanvasObjectPlacement.hxx
  CanvasPlacement.hxx
  CanvasSystemAdapter.hxx
  CanvasWindow.hxx
  ODGauge.hxx
  VGInitOperation.hxx
  CanvasEventTypes.hxx
)

add_subdirectory(ShaderVG/src)
add_subdirectory(elements)
add_subdirectory(events)
add_subdirectory(layout)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        Canvas.cxx
        CanvasEvent.cxx
        CanvasEventManager.cxx
        CanvasEventVisitor.cxx
        CanvasMgr.cxx
        CanvasObjectPlacement.cxx
        CanvasPlacement.cxx
        CanvasWindow.cxx
        ODGauge.cxx
        VGInitOperation.cxx
        CanvasEventManager.hxx
        CanvasEventVisitor.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/canvas)
endif()
