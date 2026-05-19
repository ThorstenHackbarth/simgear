# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

include (SimGearComponent)

include_directories(${PROJECT_SOURCE_DIR})

# Backend-neutral scene headers — always installed, always reachable from
# SimGearCore. Public FlightGear-side callers consume these regardless of
# whether SimGearScene is built.
set(BACKEND_NEUTRAL_HEADERS
    Handle.hxx
    SGSceneFwd.hxx
)
target_sources(SimGearCore PRIVATE ${BACKEND_NEUTRAL_HEADERS})
install(FILES ${BACKEND_NEUTRAL_HEADERS} DESTINATION include/simgear/scene)

# OSG-flavored forward declarations — internal partner header used by
# scene-subsystem code (sky/, tgdb/, ...) that legitimately holds OSG types
# in its signatures. Only installed when SimGearScene is built.
set(SCENE_HEADERS SGSceneFwd_osg.hxx)
simgear_scene_component(scene scene "${SCENE_SOURCES}" "${SCENE_HEADERS}")

# Backend-neutral migration tests (run on every backend, tagged 'migration').
if(ENABLE_TESTS)
    add_simgear_wicked_autotest(test_handle HandleTest.cxx)
    add_simgear_wicked_autotest(test_handle_neutrality HandleNeutralityTest.cxx)
endif()

foreach( mylibfolder
        material
        model
        sky
        tgdb
        util
        tsync
        viewer
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

if(ENABLE_GDAL)
    add_subdirectory(dem)
endif(ENABLE_GDAL)
