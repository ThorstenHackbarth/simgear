# SPDX-FileCopyrightText: 2025 Florent Rougon
# SPDX-License-Identifier: LGPL-2.0-or-later

set(HEADERS
    integers.hxx
    )

target_sources(SimGearCore PRIVATE ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/nasal/cpputils)
