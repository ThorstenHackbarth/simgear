# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    SGGuard.hxx
    SGQueue.hxx
    SGThread.hxx)

target_sources(SimGearCore PRIVATE SGThread.cxx ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/threads)
