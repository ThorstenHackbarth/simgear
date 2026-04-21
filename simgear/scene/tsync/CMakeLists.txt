# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    terrasync.hxx
    )

target_sources(SimGearCore PRIVATE terrasync.cxx ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/scene/tsync)
