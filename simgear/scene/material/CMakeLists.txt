# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    Atlas.hxx
    Effect.hxx
    EffectCullVisitor.hxx
    EffectGeode.hxx
    mat.hxx
    matlib.hxx
    )

if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        Atlas.cxx
        Effect.cxx
        EffectBuilder.cxx
        EffectCullVisitor.cxx
        EffectGeode.cxx
        Pass.cxx
        Technique.cxx
        TextureBuilder.cxx
        makeEffect.cxx
        mat.cxx
        matlib.cxx
        matmodel.cxx
        mipmap.cxx
        parseBlendFunc.cxx
        EffectBuilder.hxx
        Pass.hxx
        Technique.hxx
        TextureBuilder.hxx
        matmodel.hxx
        mipmap.hxx
        parseBlendFunc.hxx
        ${HEADERS})
    install(FILES ${HEADERS} DESTINATION include/simgear/scene/material)
endif()


if(ENABLE_TESTS)

add_simgear_autotest(test_parseBlendFunc parseBlendFunc_test.cxx )
target_link_libraries(test_parseBlendFunc SimGearScene)

endif(ENABLE_TESTS)
