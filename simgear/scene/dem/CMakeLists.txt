# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 Mathias Froehlich

set(HEADERS
    SGDem.hxx
    SGDemLevel.hxx
    SGDemRoot.hxx
    SGDemSession.hxx
    SGDemTile.hxx
)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        ReaderWriterPGT.cxx
        SGDem.cxx
        SGDemLevel.cxx
        SGDemRoot.cxx
        SGDemSession.cxx
        SGDemTile.cxx
        SGDemTile_gdal.cxx
        SGMesh.cxx
        ReaderWriterPGT.hxx
        SGMesh.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/dem)
endif()
