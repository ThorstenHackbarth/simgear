# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS metar.hxx precipitation.hxx)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE metar.cxx precipitation.cxx ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/environment)
endif()

if(ENABLE_TESTS)

add_simgear_autotest(test_metar test_metar.cxx)
target_link_libraries(test_metar SimGearScene)

add_simgear_autotest(test_precipitation test_precipitation.cxx)
target_link_libraries(test_precipitation SimGearScene)

endif(ENABLE_TESTS)
