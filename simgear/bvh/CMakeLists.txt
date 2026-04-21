# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    BVHBoundingBoxVisitor.hxx
    BVHGroup.hxx
    BVHLineGeometry.hxx
    BVHLineSegmentVisitor.hxx
    BVHMotionTransform.hxx
    BVHNearestPointVisitor.hxx
    BVHNode.hxx
    BVHPageNode.hxx
    BVHPager.hxx
    BVHStaticBinary.hxx
    BVHStaticData.hxx
    BVHStaticGeometry.hxx
    BVHStaticGeometryBuilder.hxx
    BVHStaticLeaf.hxx
    BVHStaticNode.hxx
    BVHStaticTriangle.hxx
    BVHSubTreeCollector.hxx
    BVHMaterial.hxx
    BVHTransform.hxx
    BVHTerrainTile.hxx
    BVHVisitor.hxx
)

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        BVHGroup.cxx
        BVHLineGeometry.cxx
        BVHLineSegmentVisitor.cxx
        BVHMotionTransform.cxx
        BVHNode.cxx
        BVHPageNode.cxx
        BVHPageRequest.cxx
        BVHPager.cxx
        BVHStaticBinary.cxx
        BVHStaticGeometry.cxx
        BVHStaticLeaf.cxx
        BVHStaticNode.cxx
        BVHStaticTriangle.cxx
        BVHSubTreeCollector.cxx
        BVHTerrainTile.cxx
        BVHTransform.cxx
        BVHPageRequest.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/bvh)
endif()

if(ENABLE_TESTS)
  add_simgear_scene_autotest(bvhtest bvhtest.cxx)
endif(ENABLE_TESTS)
