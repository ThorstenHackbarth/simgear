// SPDX-FileCopyrightText: 2008 Stuart Buchanan
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <vector>
#include <string>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Matrix>
#include <osg/LOD>

#include <simgear/scene/util/OsgMath.hxx>

namespace simgear
{
class TreeBin final {
public:
    TreeBin() = default;
    TreeBin(const SGMaterial *mat);
    TreeBin(const SGPath& absoluteFileName, const SGMaterial *mat);

    ~TreeBin() = default;   // non-virtual intentional

    int texture_varieties;
    double range;
    float height;
    float width;
    std::string texture;
    std::string normal_map;
    std::string teffect;

    void insert(osg::Vec3d t)
    { _trees.push_back(t); }

    void insert(const SGVec3f& p)
    { _trees.push_back(toOsg(p)); }

    unsigned getNumTrees() const
    { return _trees.size(); }

    const osg::Vec3d getTree(unsigned i) const
    {
        assert(i < _trees.size());
        return _trees.at(i);
    }

    std::vector<osg::Vec3d> _trees;
};

typedef std::list<TreeBin*> SGTreeBinList;

osg::Group* createForest(SGTreeBinList& forestList, osg::ref_ptr<simgear::SGReaderWriterOptions> options);
} // namespace simgear
