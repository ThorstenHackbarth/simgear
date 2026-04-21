# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    screen-dump.hxx
    video-encoder.hxx
    )


if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        extensions.cxx
        screen-dump.cxx
        video-encoder.cxx
        colors.hxx
        extensions.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/screen)
endif()
