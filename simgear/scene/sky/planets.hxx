/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <simgear/scene/SGSceneFwd.hxx>

#include <simgear/math/SGVec3.hxx>
#include <simgear/structure/SGReferenced.hxx>

class SGPlanets : public SGReferenced {
public:
    SGPlanets() = default;

    // initialize the planets structure
    osg::Node* build(int num, const SGVec3d* planet_data, double planet_dist,
                     const simgear::SGReaderWriterOptions* options);
};
