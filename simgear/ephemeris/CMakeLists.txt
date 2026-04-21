# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    celestialBody.hxx
    ephemeris.hxx
    jupiter.hxx
    mars.hxx
    mercury.hxx
    moonpos.hxx
    neptune.hxx
    pluto.hxx
    saturn.hxx
    star.hxx
    stardata.hxx
    uranus.hxx
    venus.hxx
    )

target_sources(SimGearCore PRIVATE
    celestialBody.cxx
    ephemeris.cxx
    jupiter.cxx
    mars.cxx
    mercury.cxx
    moonpos.cxx
    neptune.cxx
    saturn.cxx
    star.cxx
    stardata.cxx
    uranus.cxx
    venus.cxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/ephemeris)
