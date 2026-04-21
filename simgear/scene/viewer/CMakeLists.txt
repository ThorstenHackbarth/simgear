# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2019 Fernando García Liñán

set(HEADERS
  Compositor.hxx
  CompositorBuffer.hxx
  CompositorPass.hxx
  CompositorUtil.hxx
  )

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        ClusteredShading.cxx
        Compositor.cxx
        CompositorBuffer.cxx
        CompositorPass.cxx
        CompositorUtil.cxx
        ClusteredShading.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/viewer)
endif()
