// SGText.hxx - Manage text in the scene graph
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Torsten Dreyer Torsten (_at_) t3r *dot* de

#ifndef _SGTEXT_HXX
#define _SGTEXT_HXX 1

#include <osgDB/ReaderWriter>
#include <osg/Group>

#include <simgear/props/props.hxx>

class SGText : public osg::NodeCallback
{
public:
  static osg::Node * appendText(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::Options* options);
private:
  class UpdateCallback;
};

#endif
