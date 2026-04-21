# SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(HEADERS
  BoxLayout.hxx
  GridLayout.hxx
  ImageLayoutItem.hxx
  Layout.hxx
  LayoutItem.hxx
  NasalWidget.hxx
  SpacerItem.hxx
)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        BoxLayout.cxx
        GridLayout.cxx
        ImageLayoutItem.cxx
        Layout.cxx
        LayoutItem.cxx
        NasalWidget.cxx
        SpacerItem.cxx
        AlignFlag_values.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/canvas/layout)
endif()

add_boost_test(canvas_layout
  SOURCES canvas_layout_test.cxx
  LIBRARIES SimGearScene
)
