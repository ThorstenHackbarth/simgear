# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS newbucket.hxx)

target_sources(SimGearCore PRIVATE newbucket.cxx ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/bucket)

if(ENABLE_TESTS)
    add_simgear_autotest(test_bucket test_bucket.cxx)
endif(ENABLE_TESTS)
