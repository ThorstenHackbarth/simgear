# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

include_directories(${PROJECT_SOURCE_DIR})

# Top-level scene headers (no associated sources)
set(SCENE_HEADERS SGSceneFwd.hxx)
if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE ${SCENE_HEADERS})
    install(FILES ${SCENE_HEADERS} DESTINATION include/simgear/scene)
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
