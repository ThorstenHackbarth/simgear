# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    easyxml.hxx
    )

target_sources(SimGearCore PRIVATE easyxml.cxx ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/xml)
