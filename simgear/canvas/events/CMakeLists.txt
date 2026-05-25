# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2012 Thomas Geymayer

set(HEADERS
  CustomEvent.hxx
  DeviceEvent.hxx
  KeyboardEvent.hxx
  MouseEvent.hxx
  CanvasKeyBinding.hxx
)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        CustomEvent.cxx
        DeviceEvent.cxx
        KeyboardEvent.cxx
        MouseEvent.cxx
        CanvasKeyBinding.cxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/canvas/events)
endif()

add_simgear_scene_autotest(canvas_event event_test.cpp)

add_simgear_test(input_event_demo input_event_demo.cxx)
target_link_libraries(input_event_demo
  SimGearScene
)
