# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS magvar.hxx coremag.hxx)

target_sources(SimGearCore PRIVATE magvar.cxx coremag.cxx ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/magvar)

if(ENABLE_TESTS)
    add_executable(test_magvar testmagvar.cxx )
    target_link_libraries(test_magvar SimGearCore)
endif(ENABLE_TESTS)
