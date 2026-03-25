// Voxel cloud class
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 Stuart Buchanan (stuart13@gmail.com)

#ifndef _NEWCLOUD_HXX
#define _NEWCLOUD_HXX

#include <osg/Image>
#include <simgear/compiler.h>
#include <string>
#include <vector>

#include <simgear/math/sg_random.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

using std::string;
using std::vector;

using ImageRef = osg::ref_ptr<osg::Image>;

/**
 * 3D cloud class.
 */

class SGVoxelCloud
{
public:
    SGVoxelCloud() {};
    virtual ~SGVoxelCloud() {};

    virtual int addCloudToDetailedVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const = 0;
    virtual int addCloudToRoughVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const = 0;

    // Factory
    static SGVoxelCloud* buildCloud(const string name, const SGPropertyNode* cld_def, mt* s, const simgear::SGReaderWriterOptions* options);
};

class SGVoxelTextureCloud : public SGVoxelCloud
{
public:
    SGVoxelTextureCloud(const string name, const SGPropertyNode* cld_def, mt* s, const simgear::SGReaderWriterOptions* options);
    ~SGVoxelTextureCloud();

    int addCloudToDetailedVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const override;
    int addCloudToRoughVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const override;

    // Manage the cache
    inline static void setRoughVoxelScale(size_t scale)
    {
        if (scale != _roughVoxelScale) _roughVoxelImageCache.clear();
        _roughVoxelScale = scale;
    };

private:
    const string _name;
    string _voxelTextureFile;

    // RNG seed for this cloud
    mt* _seed;
    const simgear::SGReaderWriterOptions* _options;
    bool _reflectX;
    bool _reflectY;

    void copySubImage(const osg::Image* srcImage, int src_s, int src_t, int width, int height, osg::Image* destImage, int dest_s, int dest_t) const;

    // Generate a Cloud
    ImageRef generateCloud(ImageRef voxelImage2D) const;
    const ImageRef getDetailedCloud() const;
    const ImageRef getRoughCloud() const;

    int addCloudToVoxelField(ImageRef voxelField, ImageRef cloudVoxels, float voxelSize, osg::Vec3f p) const;

    inline static std::shared_mutex _voxelImageCacheMutex;
    typedef std::unordered_map<std::string, ImageRef> ImageCache;
    inline static ImageCache _detailedVoxelImageCache;
    inline static ImageCache _roughVoxelImageCache;
    inline static size_t _roughVoxelScale;
};

class SGVoxelLayerCloud : public SGVoxelCloud
{
public:
    SGVoxelLayerCloud(const string name, const SGPropertyNode* cld_def, mt* s, float coverage, float thicknessM);
    ~SGVoxelLayerCloud();

    int addCloudToDetailedVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const override;
    int addCloudToRoughVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const override;
    int addCloudToVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const;

private:
    const string _name;
    mt* _seed;
    float _coverage;   // Percentage coverage
    float _thicknessM; // Thickness of layer in M

    float _minDensity; // Density of the field
    float _maxDensity;
    float _minType; // Cloud type 0 = wispy, 1 = structured
    float _maxType;

    // Perlin noise parameters.
    int _perlinNoiseFrequency;
    double _perlinAlpha;
    double _perlinBeta;
    int _perlinN;
};


#endif // _NEWCLOUD_HXX
