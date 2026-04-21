# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    GroundLightManager.hxx
    LineFeatureBin.hxx
    LightBin.hxx
    ObjectInstanceBin.hxx
    ReaderWriterSTG.hxx
    SGDirectionalLightBin.hxx
    SGLightBin.hxx
    SGOceanTile.hxx
    TreeBin.hxx
    VPBBufferData.hxx
    VPBLineFeatureRenderer.hxx
    VPBTechnique.hxx
    apt_signs.hxx
    pt_lights.hxx
    userdata.hxx
    )

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        GroundLightManager.cxx
        LineFeatureBin.cxx
        LightBin.cxx
        ObjectInstanceBin.cxx
        ReaderWriterSPT.cxx
        ReaderWriterSTG.cxx
        SGBuildingBin.cxx
        SGOceanTile.cxx
        SGReaderWriterBTG.cxx
        SGVasiDrawable.cxx
        TreeBin.cxx
        VPBElevationSlice.cxx
        VPBMaterialHandler.cxx
        VPBLineFeatureRenderer.cxx
        VPBTileBounds.cxx
        VPBTechnique.cxx
        apt_signs.cxx
        obj.cxx
        pt_lights.cxx
        userdata.cxx
        ReaderWriterSPT.hxx
        SGBuildingBin.hxx
        SGModelBin.hxx
        SGNodeTriangles.hxx
        SGReaderWriterBTG.hxx
        SGTexturedTriangleBin.hxx
        SGTileDetailsCallback.hxx
        SGTileGeometryBin.hxx
        SGTriangleBin.hxx
        SGVasiDrawable.hxx
        SGVertexArrayBin.hxx
        VPBElevationSlice.hxx
        VPBMaterialHandler.hxx
        VPBTileBounds.hxx
        obj.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/tgdb)
endif()

if(ENABLE_TESTS)
  add_simgear_scene_autotest(BucketBoxTest BucketBoxTest.cxx)
endif(ENABLE_TESTS)
