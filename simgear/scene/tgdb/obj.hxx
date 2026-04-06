// obj.hxx -- routines to handle loading scenery and building the plib
//            scene graph.
//
// Written by Curtis Olson, started October 1997.
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Curtis L. Olson

#pragma once


#include <string>

#include <simgear/scene/SGSceneFwd.hxx>


osg::Node*
SGLoadBTG(const std::string& path,
          const simgear::SGReaderWriterOptions* options);
