// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006-2007 Mathias Froehlich

#pragma once

#include <simgear/scene/SGSceneFwd.hxx>

class SGBucket;

// Generate an ocean tile
osg::Node* SGOceanTile(double clat, double clon, double width, double height, SGMaterialLib *matlib, int latPoints = 5, int lonPoints = 5);
osg::Node* SGOceanTile(const SGBucket& b, SGMaterialLib* matlib, int latPoints = 5, int lonPoints = 5);
