# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 Florent Rougon

set(HEADERS EmbeddedResource.hxx
            EmbeddedResourceManager.hxx
)

target_sources(SimGearCore PRIVATE
    EmbeddedResource.cxx
    EmbeddedResourceManager.cxx
    EmbeddedResourceProxy.cxx
    EmbeddedResourceProxy.hxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/embedded_resources)

if(ENABLE_TESTS)
    add_simgear_autotest(test_embedded_resources embedded_resources_test.cxx)
endif(ENABLE_TESTS)
