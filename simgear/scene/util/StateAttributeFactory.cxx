// Copyright (C) 2007 Tim Moore
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "StateAttributeFactory.hxx"
#include <simgear_config.h>

#include <osg/AlphaFunc>
#include <osg/Array>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/Texture2D>
#include <osg/Texture3D>

#include <osg/Image>
#include <osgDB/ReadFile>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/material/mipmap.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "Noise.hxx"

using namespace osg;

namespace simgear {
StateAttributeFactory::StateAttributeFactory()
{
    // Standard blend function
    _standardBlendFunc = new BlendFunc;
    _standardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    _standardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    _standardBlendFunc->setDataVariance(Object::STATIC);

    // White color
    _white = new Vec4Array(1);
    (*_white)[0].set(1.0f, 1.0f, 1.0f, 1.0f);
    _white->setDataVariance(Object::STATIC);

    // White texture
    osg::ref_ptr<osg::Image> whiteImage = new osg::Image;
    whiteImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char* whiteImageBytes = whiteImage->data();
    whiteImageBytes[0] = 255;
    whiteImageBytes[1] = 255;
    whiteImageBytes[2] = 255;
    whiteImageBytes[3] = 255;
    _whiteTexture = new osg::Texture2D;
    _whiteTexture->setImage(whiteImage);
    _whiteTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _whiteTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _whiteTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _whiteTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _whiteTexture->setDataVariance(osg::Object::STATIC);

    // Transparent texture
    osg::ref_ptr<osg::Image> transparentImage = new osg::Image;
    transparentImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char* transparentImageBytes = transparentImage->data();
    transparentImageBytes[0] = 255;
    transparentImageBytes[1] = 255;
    transparentImageBytes[2] = 255;
    transparentImageBytes[3] = 0;
    _transparentTexture = new osg::Texture2D;
    _transparentTexture->setImage(transparentImage);
    _transparentTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _transparentTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _transparentTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _transparentTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _transparentTexture->setDataVariance(osg::Object::STATIC);

    // Null normal map texture
    osg::ref_ptr<osg::Image> nullNormalMapImage = new osg::Image;
    nullNormalMapImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char* nullNormalMapBytes = nullNormalMapImage->data();
    nullNormalMapBytes[0] = 128;
    nullNormalMapBytes[1] = 128;
    nullNormalMapBytes[2] = 255;
    nullNormalMapBytes[3] = 255;
    _nullNormalmapTexture = new osg::Texture2D;
    _nullNormalmapTexture->setImage(nullNormalMapImage);
    _nullNormalmapTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _nullNormalmapTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _nullNormalmapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _nullNormalmapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _nullNormalmapTexture->setDataVariance(osg::Object::STATIC);

    // Cloud Voxel Textures.  The Image of this will be replaced with clouds by FGClouds, but needs
    // to have a A value > 0 as it is used as a Signed Distance Field (SDF), and a value of 0 will
    // cause the cloud shader to tight loop
    osg::ref_ptr<osg::Image> detailedVoxelImage = new osg::Image;
    detailedVoxelImage->allocateImage(1, 1, 1, GL_RGBA, GL_FLOAT);
    detailedVoxelImage->setColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f), 0, 0, 0);
    detailedVoxelImage->setName("Initial detailedVoxelImage - no data");
    _detailedVoxelTexture = new osg::Texture3D;
    _detailedVoxelTexture->setImage(detailedVoxelImage);
    _detailedVoxelTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _detailedVoxelTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // Clamp to the a border of clear sky
    _detailedVoxelTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _detailedVoxelTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _detailedVoxelTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
    _detailedVoxelTexture->setBorderColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    _detailedVoxelTexture->setDataVariance(osg::Object::DYNAMIC);

    osg::ref_ptr<osg::Image> roughVoxelImage = new osg::Image;
    roughVoxelImage->allocateImage(1, 1, 1, GL_RGBA, GL_FLOAT);
    roughVoxelImage->setColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f), 0, 0, 0);
    roughVoxelImage->setName("Initial roughVoxelImage - no data");
    _roughVoxelTexture = new osg::Texture3D;
    _roughVoxelTexture->setImage(roughVoxelImage);
    _roughVoxelTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _roughVoxelTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // Clamp to the a border of clear sky
    _roughVoxelTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _roughVoxelTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _roughVoxelTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
    _roughVoxelTexture->setBorderColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    _roughVoxelTexture->setDataVariance(osg::Object::DYNAMIC);

    // Cloud Shading Texture.  The Image of this will be replaced with clouds by FGClouds
    osg::ref_ptr<osg::Image> cloudVoxelShadeImage = new osg::Image;
    cloudVoxelShadeImage->allocateImage(1, 1, 1, GL_RGBA, GL_FLOAT);
    cloudVoxelShadeImage->setColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f), 0, 0, 0);
    cloudVoxelShadeImage->setName("Initial cloudVoxelShadeImage - no data");
    _cloudVoxelShadeTexture = new osg::Texture3D;
    _cloudVoxelShadeTexture->setImage(cloudVoxelShadeImage);
    _cloudVoxelShadeTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _cloudVoxelShadeTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // Clamp to the a border of clear sky
    _cloudVoxelShadeTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _cloudVoxelShadeTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _cloudVoxelShadeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
    _cloudVoxelShadeTexture->setBorderColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    _cloudVoxelShadeTexture->setDataVariance(osg::Object::DYNAMIC);

    // Cloud Wind Offset Texture.  This image is used to offset the voxel space to account for the wind
    osg::ref_ptr<osg::Image> cloudWindOffsetImage = new osg::Image;
    cloudWindOffsetImage->allocateImage(1, 1, 1, GL_RGBA, GL_FLOAT);
    cloudWindOffsetImage->setColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f), 0, 0, 0);
    cloudWindOffsetImage->setName("Initial cloudWindOffsetImage - no data");

    _cloudWindOffsetTexture = new osg::Texture1D;
    _cloudWindOffsetTexture->setImage(cloudWindOffsetImage);
    _cloudWindOffsetTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _cloudWindOffsetTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _cloudWindOffsetTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    _cloudWindOffsetTexture->setDataVariance(osg::Object::DYNAMIC);

    // Cull front facing polygons
    _cullFaceFront = new CullFace(CullFace::FRONT);
    _cullFaceFront->setDataVariance(Object::STATIC);

    // Cull back facing polygons
    _cullFaceBack = new CullFace(CullFace::BACK);
    _cullFaceBack->setDataVariance(Object::STATIC);

    // Standard depth function
    _standardDepth = new Depth(Depth::LESS, 0.0, 1.0, true);
    _standardDepth->setDataVariance(Object::STATIC);

    // Standard depth function with writes disabled
    _standardDepthWritesDisabled = new Depth(Depth::LESS, 0.0, 1.0, false);
    _standardDepthWritesDisabled->setDataVariance(Object::STATIC);
}

osg::Image* make3DNoiseImage(int texSize)
{
    osg::Image* image = new osg::Image;
    image->setImage(texSize, texSize, texSize,
                    4, GL_RGBA, GL_UNSIGNED_BYTE,
                    new unsigned char[4 * texSize * texSize * texSize],
                    osg::Image::USE_NEW_DELETE);

    const int startFrequency = 4;
    const int numOctaves = 4;

    int f, i, j, k, inc;
    double ni[3];
    double inci, incj, inck;
    int frequency = startFrequency;
    GLubyte* ptr;
    double amp = 0.5;

    SG_BULK_LOG(SG_TERRAIN, "creating 3D noise texture... ");

    for (f = 0, inc = 0; f < numOctaves; ++f, frequency *= 2, ++inc, amp *= 0.5) {
        SetNoiseFrequency(frequency);
        ptr = image->data();
        ni[0] = ni[1] = ni[2] = 0;

        inci = 1.0 / (texSize / frequency);
        for (i = 0; i < texSize; ++i, ni[0] += inci) {
            incj = 1.0 / (texSize / frequency);
            for (j = 0; j < texSize; ++j, ni[1] += incj) {
                inck = 1.0 / (texSize / frequency);
                for (k = 0; k < texSize; ++k, ni[2] += inck, ptr += 4) {
                    *(ptr + inc) = (GLubyte)(((noise3(ni) + 1.0) * amp) * 128.0);
                }
            }
        }
    }

    SG_LOG(SG_TERRAIN, SG_BULK, "DONE");

    return image;
}

osg::Texture3D* StateAttributeFactory::getNoiseTexture(int size)
{
    std::lock_guard<std::mutex> lock(StateAttributeFactory::_noise_mutex); // Lock the _noises for this scope
    NoiseMap::iterator itr = _noises.find(size);
    if (itr != _noises.end())
        return itr->second.get();
    Texture3D* noiseTexture = new osg::Texture3D;
    noiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::REPEAT);
    noiseTexture->setImage(make3DNoiseImage(size));
    _noises.insert(std::make_pair(size, noiseTexture));
    return noiseTexture;
}

void StateAttributeFactory::copySubImage(const osg::Image* srcImage, int src_s, int src_t, int width, int height,
                                         osg::Image* destImage, int dest_s, int dest_t)
{
    for (int row = 0; row < height; ++row) {
        const unsigned char* srcData = srcImage->data(src_s, src_t + row, 0);
        unsigned char* destData = destImage->data(dest_s, dest_t + row, 0);
        memcpy(destData, srcData, (width * destImage->getPixelSizeInBits()) / 8);
    }
}


osg::Texture3D* StateAttributeFactory::getCloudNoiseTexture(int s)
{
    std::lock_guard<std::mutex> lock(StateAttributeFactory::_cloudnoise_mutex); // Lock the _noises for this scope

    if (_cloudnoise != nullptr) return _cloudnoise;

    osg::ref_ptr<osg::Image> noiseImage = osgDB::readRefImageFile("Textures/Sky/cloudnoise.png", simgear::SGReaderWriterOptions::copyOrCreate(nullptr));

    osg::ref_ptr<osg::Image> noiseImage3d = new osg::Image;
    int size = noiseImage->t();
    int depth = noiseImage->s() / noiseImage->t();

    noiseImage3d->allocateImage(size, size, depth, noiseImage->getPixelFormat(), noiseImage->getDataType());

    for (int i = 0; i < depth; ++i) {
        osg::ref_ptr<osg::Image> subimage = new osg::Image;
        subimage->allocateImage(size, size, 1,
                                noiseImage->getPixelFormat(), noiseImage->getDataType());
        copySubImage(noiseImage, size * i, 0, size, size, subimage.get(), 0, 0);
        noiseImage3d->copySubImage(0, 0, i, subimage.get());
    }

    noiseImage3d->setInternalTextureFormat(noiseImage->getInternalTextureFormat());
    noiseImage3d = simgear::effect::computeMipmap(noiseImage3d.get(), simgear::effect::MipMapTuple(simgear::effect::MipMapFunction::AUTOMATIC, simgear::effect::MipMapFunction::AUTOMATIC, simgear::effect::MipMapFunction::AUTOMATIC, simgear::effect::MipMapFunction::AUTOMATIC));

    osg::ref_ptr<osg::Texture3D> noiseTexture = new osg::Texture3D;
    noiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::REPEAT);
    noiseTexture->setImage(noiseImage3d);

    _cloudnoise = noiseTexture;
    return noiseTexture;
}

void StateAttributeFactory::setCloudVoxelImages(osg::ref_ptr<osg::Image> detailedVoxelImage, osg::ref_ptr<osg::Image> roughVoxelImage, osg::ref_ptr<osg::Image> voxelShadeImage, bool repeat)
{
    _detailedVoxelTexture->setTextureSize(detailedVoxelImage->s(), detailedVoxelImage->t(), detailedVoxelImage->r());
    _detailedVoxelTexture->setInternalFormat(GL_RGBA32F);
    _detailedVoxelTexture->setImage(detailedVoxelImage);
    if (repeat) {
        // Repeating texture is set to mirrored so that the SDF is correct across UV boundaries
        _detailedVoxelTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
        _detailedVoxelTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::MIRROR);
        _detailedVoxelTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
        _detailedVoxelTexture->setBorderColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f / detailedVoxelImage->s()));

        _cloudVoxelShadeTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
        _cloudVoxelShadeTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::MIRROR);
        _cloudVoxelShadeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);

    } else {
        _detailedVoxelTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        _detailedVoxelTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        _detailedVoxelTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
        _detailedVoxelTexture->setBorderColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f / detailedVoxelImage->s()));

        _cloudVoxelShadeTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        _cloudVoxelShadeTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
        _cloudVoxelShadeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
        _cloudVoxelShadeTexture->setBorderColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    }
    _detailedVoxelTexture->dirtyTextureObject();


    _roughVoxelTexture->setTextureSize(roughVoxelImage->s(), roughVoxelImage->t(), roughVoxelImage->r());
    _roughVoxelTexture->setInternalFormat(GL_RGBA32F);
    _roughVoxelTexture->setImage(roughVoxelImage);
    _roughVoxelTexture->dirtyTextureObject();

    _cloudVoxelShadeTexture->setTextureSize(voxelShadeImage->s(), voxelShadeImage->t(), voxelShadeImage->r());
    _cloudVoxelShadeTexture->setInternalFormat(GL_RGBA32F);
    _cloudVoxelShadeTexture->setImage(voxelShadeImage);
    _cloudVoxelShadeTexture->dirtyTextureObject();
}

osg::Texture* StateAttributeFactory::getBufferTexture(const std::string& name)
{
    std::lock_guard<std::mutex> lock(StateAttributeFactory::_buffer_mutex); // Lock the _buffers for this scope
    BufferMap::iterator itr = _buffers.find(name);
    if (itr != _buffers.end())
        return itr->second.get();
    return nullptr;
}

void StateAttributeFactory::addBufferTexture(const std::string& name,
                                             osg::Texture* texture)
{
    std::lock_guard<std::mutex> lock(StateAttributeFactory::_buffer_mutex); // Lock the _buffers for this scope
    _buffers[name] = texture;
}

void StateAttributeFactory::removeBufferTexture(const std::string& name)
{
    std::lock_guard<std::mutex> lock(StateAttributeFactory::_buffer_mutex); // Lock the _buffers for this scope
    _buffers.erase(name);
}

void StateAttributeFactory::setCloudWindOffsetImage(osg::ref_ptr<osg::Image> cloudWindOffsetImage)
{
    if (cloudWindOffsetImage) {
        _cloudWindOffsetTexture->setTextureWidth(cloudWindOffsetImage->s());
        _cloudWindOffsetTexture->setInternalFormat(GL_RGBA32F);
        _cloudWindOffsetTexture->setImage(cloudWindOffsetImage);
        _cloudWindOffsetTexture->dirtyTextureObject();
    }
}

// anchor the destructor into this file, to avoid ref_ptr warnings
StateAttributeFactory::~StateAttributeFactory()
{
}

} // namespace simgear
