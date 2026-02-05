// Copyright (C) 2007 Tim Moore
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <OpenThreads/Mutex>
#include <map>
#include <mutex>
#include <osg/Array>
#include <osg/ref_ptr>

namespace osg {
class AlphaFunc;
class BlendFunc;
class CullFace;
class Depth;
class ShadeModel;
class Texture2D;
class Texture3D;
class TexEnv;
} // namespace osg

#include <simgear/scene/util/OsgSingleton.hxx>

// Return read-only instances of common OSG state attributes.
namespace simgear {
class StateAttributeFactory : public ReferencedSingleton<StateAttributeFactory>
{
public:
    virtual ~StateAttributeFactory();

    // alpha source, 1 - alpha destination
    osg::BlendFunc* getStandardBlendFunc() { return _standardBlendFunc.get(); }
    // White color
    osg::Vec4Array* getWhiteColor() { return _white.get(); }
    // White, repeating texture
    osg::Texture2D* getWhiteTexture() { return _whiteTexture.get(); }
    // A white, completely transparent texture
    osg::Texture2D* getTransparentTexture() { return _transparentTexture.get(); }
    // Null normalmap texture vec3(0.5, 0.5, 1.0)
    osg::Texture2D* getNullNormalmapTexture() { return _nullNormalmapTexture.get(); }

    // Voxel textures for clouds
    osg::Texture3D* getDetailedCloudVoxelTexture() { return _detailedVoxelTexture.get(); }
    osg::Texture3D* getRoughCloudVoxelTexture() { return _roughVoxelTexture.get(); }
    osg::Texture3D* getCloudVoxelShadeTexture() { return _cloudVoxelShadeTexture.get(); }
    void setCloudVoxelImages(osg::ref_ptr<osg::Image> detailedVoxelImage, osg::ref_ptr<osg::Image> retailedVoxelImage, osg::ref_ptr<osg::Image> voxelShadeImage, bool repeat);

    // cull front and back facing polygons
    osg::CullFace* getCullFaceFront() { return _cullFaceFront.get(); }
    osg::CullFace* getCullFaceBack() { return _cullFaceBack.get(); }
    // Standard depth
    osg::Depth* getStandardDepth() { return _standardDepth.get(); }
    // Standard depth with writes disabled
    osg::Depth* getStandardDepthWritesDisabled() { return _standardDepthWritesDisabled.get(); }
    osg::Texture3D* getNoiseTexture(int size);
    osg::Texture3D* getCloudNoiseTexture(int size);
    osg::Texture* getBufferTexture(const std::string& name);

    void addBufferTexture(const std::string& name, osg::Texture* texture);
    void removeBufferTexture(const std::string& name);

    StateAttributeFactory();

protected:
    osg::ref_ptr<osg::BlendFunc> _standardBlendFunc;
    osg::ref_ptr<osg::Vec4Array> _white;
    osg::ref_ptr<osg::Texture2D> _whiteTexture;
    osg::ref_ptr<osg::Texture2D> _transparentTexture;
    osg::ref_ptr<osg::Texture2D> _nullNormalmapTexture;
    osg::ref_ptr<osg::Texture3D> _detailedVoxelTexture;
    osg::ref_ptr<osg::Texture3D> _roughVoxelTexture;
    osg::ref_ptr<osg::Texture3D> _cloudVoxelShadeTexture;
    osg::ref_ptr<osg::CullFace> _cullFaceFront;
    osg::ref_ptr<osg::CullFace> _cullFaceBack;
    osg::ref_ptr<osg::Depth> _standardDepth;
    osg::ref_ptr<osg::Depth> _standardDepthWritesDisabled;

    typedef std::map<int, osg::ref_ptr<osg::Texture3D>> NoiseMap;
    NoiseMap _noises;
    osg::ref_ptr<osg::Texture3D> _cloudnoise;
    using BufferMap = std::map<std::string, osg::ref_ptr<osg::Texture>>;
    BufferMap _buffers;

    inline static std::mutex _noise_mutex;      // Protects the NoiseMap _noises for mult-threaded access
    inline static std::mutex _cloudnoise_mutex; // Protects the NoiseMap _cloudnoises for mult-threaded access
    inline static std::mutex _buffer_mutex;     // Protects the BufferMap _buffers for mult-threaded access

    void copySubImage(const osg::Image* srcImage, int src_s, int src_t, int width, int height, osg::Image* destImage, int dest_s, int dest_t);
};

} // namespace simgear
