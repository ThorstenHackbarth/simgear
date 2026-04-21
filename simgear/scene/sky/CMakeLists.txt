# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    CloudShaderGeometry.hxx
    cloud.hxx
    cloudfield.hxx
    dome.hxx
    galaxy.hxx
    moon.hxx
    newcloud.hxx
    oursun.hxx
    planets.hxx
    sky.hxx
    sphere.hxx
    stars.hxx
    )

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        CloudShaderGeometry.cxx
        cloud.cxx
        cloudfield.cxx
        dome.cxx
        galaxy.cxx
        moon.cxx
        newcloud.cxx
        oursun.cxx
        planets.cxx
        sky.cxx
        sphere.cxx
        stars.cxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/sky)
endif()
