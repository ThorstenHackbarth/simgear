# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    sg_time.hxx
    timestamp.hxx
    lowleveltime.h
    )

if(ENABLE_TESTS)
    function(create_test TEST_NAME)
        add_executable(${TEST_NAME} ${TEST_NAME}.cxx)
        target_link_libraries(${TEST_NAME} SimGearCore)
        set_target_properties(${TEST_NAME} PROPERTIES
            COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

        # for simgear_config.h
        target_include_directories(${TEST_NAME} PRIVATE ${PROJECT_BINARY_DIR}/simgear)
    endfunction()

    create_test(zonetest)
endif()

target_sources(SimGearCore PRIVATE
    lowleveltime.cxx
    sg_time.cxx
    timestamp.cxx
    timezone.cxx
    zonedetect.c
    timezone.h
    zonedetect.h
    rawprofile.hxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/timing)
