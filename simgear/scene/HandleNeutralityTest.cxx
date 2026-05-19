/*
 * SPDX-FileName: HandleNeutralityTest.cxx
 * SPDX-FileComment: Phase-1 audit — assert backend-neutral headers carry no OSG dependency
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
 */

// This translation unit is built with **no** OSG include path. If any of the
// Phase-1 backend-neutral public headers transitively #include an <osg/...>
// header (or anything that requires OSG to be installed), the compile fails.
// That is the audit: backend-neutral means really backend-neutral.

#include <simgear/scene/Handle.hxx>
#include <simgear/scene/SGSceneFwd.hxx>

#include <simgear/misc/test_macros.hxx>


int main()
{
    // Both header includes compiled — that is the test.
    sg::scene::NodeHandle node;
    sg::scene::TextureHandle texture;
    sg::scene::MaterialHandle material;
    sg::scene::ModelHandle model;
    sg::scene::CompositorHandle compositor;
    sg::scene::CanvasHandle canvas;

    SG_CHECK_EQUAL(node.valid(), false);
    SG_CHECK_EQUAL(texture.valid(), false);
    SG_CHECK_EQUAL(material.valid(), false);
    SG_CHECK_EQUAL(model.valid(), false);
    SG_CHECK_EQUAL(compositor.valid(), false);
    SG_CHECK_EQUAL(canvas.valid(), false);

    return 0;
}
