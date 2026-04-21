# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    BoundingVolumeBuildVisitor.hxx
    BVHDebugCollectVisitor.hxx
    BVHPageNodeOSG.hxx
    CheckSceneryVisitor.hxx
    ModelRegistry.hxx
    PrimitiveCollector.hxx
    SGLight.hxx
    SGPickAnimation.hxx
    animation.hxx
    modellib.hxx
    particles.hxx
    persparam.hxx
    placement.hxx
    )

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        BVHPageNodeOSG.cxx
        CheckSceneryVisitor.cxx
        ConditionNode.cxx
        ModelRegistry.cxx
        PrimitiveCollector.cxx
        ReaderWriterAC3D.cxx
        ReaderWriterGLTF.cxx
        SGInteractionAnimation.cxx
        SGLight.cxx
        SGLightAnimation.cxx
        SGPickAnimation.cxx
        SGPBRAnimation.cxx
        SGOffsetTransform.cxx
        SGReaderWriterXML.cxx
        SGRotateTransform.cxx
        SGScaleTransform.cxx
        SGText.cxx
        SGTrackToAnimation.cxx
        SGTranslateTransform.cxx
        animation.cxx
        model.cxx
        modellib.cxx
        particles.cxx
        persparam.cxx
        placement.cxx
        ConditionNode.hxx
        ReaderWriterAC3D.hxx
        ReaderWriterGLTF.hxx
        SGInteractionAnimation.hxx
        SGPBRAnimation.hxx
        SGOffsetTransform.hxx
        SGReaderWriterXML.hxx
        SGRotateTransform.hxx
        SGScaleTransform.hxx
        SGText.hxx
        SGTrackToAnimation.hxx
        SGTranslateTransform.hxx
        model.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/model)
endif()

if(ENABLE_TESTS)
  add_simgear_scene_autotest(test_animations animation_test.cxx)
endif(ENABLE_TESTS)
