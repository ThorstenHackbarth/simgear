# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    ColorInterpolator.hxx
    OptionsReadFileCallback.hxx
    OrthophotoManager.hxx
    OsgDebug.hxx
    OsgMath.hxx
    OsgSingleton.hxx
    OsgUtils.hxx
    OsgIoCapture.hxx
    ProjectionMatrix.hxx
    RenderConstants.hxx
    SGImageUtils.hxx
    SGNodeMasks.hxx
    SGPickCallback.hxx
    SGReaderWriterOptions.hxx
    SGSceneFeatures.hxx
    SGSceneUserData.hxx
    SGTransientModelData.hxx
    SGUpdateVisitor.hxx
    StateAttributeFactory.hxx
    UpdateOnceCallback.hxx
    SGProgram.hxx
    )

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        ColorInterpolator.cxx
        color_space.cxx
        CopyOp.cxx
        DeletionManager.cxx
        NodeAndDrawableVisitor.cxx
        Noise.cxx
        OptionsReadFileCallback.cxx
        OrthophotoManager.cxx
        OsgDebug.cxx
        OsgUtils.cxx
        parse_color.cxx
        PrimitiveUtils.cxx
        ProjectionMatrix.cxx
        QuadTreeBuilder.cxx
        SGEnlargeBoundingBox.cxx
        SGImageUtils.cxx
        SGReaderWriterOptions.cxx
        SGSceneFeatures.cxx
        SGSceneUserData.cxx
        SGStateAttributeVisitor.cxx
        SGTextureStateAttributeVisitor.cxx
        SplicingVisitor.cxx
        StateAttributeFactory.cxx
        UpdateOnceCallback.cxx
        project.cxx
        SGProgram.cxx
        LoadShader.cxx
        FindGroupVisitor.cxx
        color_space.hxx
        CopyOp.hxx
        DeletionManager.hxx
        MemoryBarrier.hxx
        NodeAndDrawableVisitor.hxx
        Noise.hxx
        parse_color.hxx
        PrimitiveUtils.hxx
        QuadTreeBuilder.hxx
        SGDebugDrawCallback.hxx
        SGEnlargeBoundingBox.hxx
        SGStateAttributeVisitor.hxx
        SGTextureStateAttributeVisitor.hxx
        SplicingVisitor.hxx
        VectorArrayAdapter.hxx
        project.hxx
        LoadShader.hxx
        FindGroupVisitor.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/util)
endif()

if(ENABLE_TESTS)

add_simgear_autotest(test_parse_color parse_color_test.cxx )
target_link_libraries(test_parse_color SimGearScene)

endif(ENABLE_TESTS)
