// Voxel cloud class
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 Stuart Buchanan (stuart13@gmail.com)

#ifdef HAVE_CONFIG_H
    #include <simgear_config.h>
#endif

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgUtil/PerlinNoise>

#include <simgear/compiler.h>

#include <3rdparty/thinks_fmm/fast_marching_method.hpp>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/SGImageUtils.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "newcloud.hxx"

using namespace simgear;
using namespace osg;
using namespace std;

namespace fmm = thinks::fast_marching_method;

SGVoxelCloud* SGVoxelCloud::buildCloud(const string name, const SGPropertyNode* cld_def, mt* s, const simgear::SGReaderWriterOptions* options)
{
    simgear::PropertyList detailedTextures = cld_def->getChildren("voxel-texture");

    if (!detailedTextures.empty()) {
        // If any textures are defined, then we'll assume it's a texture-based cloud
        return new SGVoxelTextureCloud(name, cld_def, s, options);
    }

    if (cld_def->getFloatValue("coverage", 0.0) > 0.0) {
        // Some coverage value indicates that this is Layer definition
        return new SGVoxelLayerCloud(name, cld_def, s, cld_def->getFloatValue("coverage-norm", 0.5), cld_def->getFloatValue("thickness-m", 400.0));
    }

    // Fall back to texture based cloud - default property values will ensures something is generated.
    return new SGVoxelTextureCloud(name, cld_def, s, options);
}

SGVoxelTextureCloud::SGVoxelTextureCloud(const string name, const SGPropertyNode* cld_def, mt* s, const simgear::SGReaderWriterOptions* options) : _name(name),
                                                                                                                                                   _seed(s),
                                                                                                                                                   _options(options)
{
    _reflectX = mt_rand(_seed) > 0.5;
    _reflectY = mt_rand(_seed) > 0.5;

    simgear::PropertyList detailedTextures = cld_def->getChildren("voxel-texture");

    const unsigned int cloudListSize = (unsigned int)detailedTextures.size();
    if (cloudListSize == 0) {
        _voxelTextureFile = "Textures/Sky/cu.medium.voxels.png";
        SG_LOG(SG_ENVIRONMENT, SG_DEV_ALERT, "Unable to find voxel-texture for cloud definition " << _name << " " << cld_def->getPath());
    } else {
        const unsigned int idx = mt_rand32(_seed) % cloudListSize;
        _voxelTextureFile = detailedTextures[idx]->getStringValue();
        SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Selected " << _voxelTextureFile << " for cloud definition " << _name << " " << cld_def->getPath());
    }
}

SGVoxelTextureCloud::~SGVoxelTextureCloud() = default;

void SGVoxelTextureCloud::copySubImage(const osg::Image* srcImage, int src_s, int src_t, int width, int height,
                                       osg::Image* destImage, int dest_s, int dest_t) const
{
    for (int row = 0; row < height; ++row) {
        const unsigned char* srcData = srcImage->data(src_s, src_t + row, 0);
        unsigned char* destData = destImage->data(dest_s, dest_t + row, 0);
        memcpy(destData, srcData, (width * destImage->getPixelSizeInBits()) / 8);
    }
}

const ImageRef SGVoxelTextureCloud::getDetailedCloud() const
{
    {
        const std::shared_lock<std::shared_mutex> lock_shared(_voxelImageCacheMutex); // Share lock the mutex for this scope
        if (_detailedVoxelImageCache.contains(_voxelTextureFile)) return _detailedVoxelImageCache[_voxelTextureFile];
    }

    ImageRef voxelImage2D = osgDB::readRefImageFile(_voxelTextureFile, _options);

    if (voxelImage2D == nullptr) {
        SG_LOG(SG_ALL, SG_DEV_ALERT, "Unable to load cloud voxel image file " << _voxelTextureFile);
        return nullptr;
    }

    ImageRef img = generateCloud(voxelImage2D);

    {
        const std::lock_guard<std::shared_mutex> lock(_voxelImageCacheMutex); // Exclusive lock on the mutex for this scope
        SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Adding cloud voxel image file to cache " << _voxelTextureFile << " " << _detailedVoxelImageCache.size());
        auto [it, inserted] = _detailedVoxelImageCache.try_emplace(_voxelTextureFile, img);

        if (!inserted) {
            // Another thread beat us to it — use their result and discard ours
            return it->second;
        }
    }

    return img;
}

const ImageRef SGVoxelTextureCloud::getRoughCloud() const
{
    ImageRef detailedImage = nullptr;

    {
        const std::shared_lock<std::shared_mutex> lock_shared(_voxelImageCacheMutex); // Share lock the mutex for this scope

        // Return the cached rough voxel image if available.
        if (_roughVoxelImageCache.contains(_voxelTextureFile)) return _roughVoxelImageCache.at(_voxelTextureFile);

        // Otherwise check if we already have the detailed one.
        if (_detailedVoxelImageCache.contains(_voxelTextureFile)) detailedImage = _detailedVoxelImageCache.at(_voxelTextureFile);
    }

    if (detailedImage == nullptr) {
        // No detailed image either, so read it
        ImageRef voxelImage2D = osgDB::readRefImageFile(_voxelTextureFile, _options);

        if (voxelImage2D == nullptr) {
            SG_LOG(SG_ALL, SG_DEV_ALERT, "Unable to load cloud voxel image file " << _voxelTextureFile);
            return nullptr;
        }

        // And generate and push into cache
        detailedImage = generateCloud(voxelImage2D);
        {
            const std::lock_guard<std::shared_mutex> lock(_voxelImageCacheMutex); // Exclusive lock on the mutex for this scope
            _detailedVoxelImageCache[_voxelTextureFile] = detailedImage;
        }
    }

    // Now scale to the required dimensions
    ImageRef roughImage = ImageUtils::cloneImage(detailedImage);
    roughImage->scaleImage(detailedImage->s() / _roughVoxelScale, detailedImage->t() / _roughVoxelScale, detailedImage->r() / _roughVoxelScale);

    {
        // Put into the cache for the future
        const std::lock_guard<std::shared_mutex> lock(_voxelImageCacheMutex); // Exclusive lock on the mutex for this scope
        auto [it, inserted] = _roughVoxelImageCache.try_emplace(_voxelTextureFile, roughImage);

        if (!inserted) {
            // Another thread beat us to it — use their result and discard ours
            return it->second;
        }
    }

    return roughImage;
}


ImageRef SGVoxelTextureCloud::generateCloud(ImageRef voxelImage2D) const
{
    ImageRef voxelImage = new osg::Image;
    int size = voxelImage2D->t();
    int depth = voxelImage2D->s() / voxelImage2D->t();

    // For our dimension profile below to work, we need a voxel space that has 0 values on all the borders.
    // To ensure this, we pad the 3D voxel image by one voxel on each border.
    voxelImage->allocateImage(size + 2, size + 2, depth + 2, voxelImage2D->getPixelFormat(), voxelImage2D->getDataType());
    for (int i = 0; i < voxelImage->s(); ++i) {
        for (int j = 0; j < voxelImage->t(); ++j) {
            for (int k = 0; k < voxelImage->r(); ++k) {
                voxelImage->setColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f), i, j, k);
            }
        }
    }

    for (int i = 0; i < depth; ++i) {
        ImageRef subimage = new osg::Image;
        subimage->allocateImage(size, size, 1,
                                voxelImage2D->getPixelFormat(), voxelImage2D->getDataType());
        copySubImage(voxelImage2D, size * i, 0, size, size, subimage.get(), 0, 0);

        // Copy into the _voxelImage offset by 1 in each dimension which builds the required border.
        voxelImage->copySubImage(1, 1, i + 1, subimage.get());
    }

    voxelImage->setInternalTextureFormat(voxelImage2D->getInternalTextureFormat());
    voxelImage->setName(voxelImage2D->getName());

    // Now generate the dimensional profile in the r channel by generating an SDF
    vector<std::array<int, 3>> cloudBoundaryIndices;
    vector<float> cloudBoundaryDistances;

    for (int i = 0; i < voxelImage->s(); ++i) {
        for (int j = 0; j < voxelImage->t(); ++j) {
            for (int k = 0; k < voxelImage->r(); ++k) {
                if (voxelImage->getColor(i, j, k).b() < SG_EPSILON) {
                    cloudBoundaryIndices.push_back(std::array<int, 3>{{i, j, k}});
                    cloudBoundaryDistances.push_back(0.0f);
                }
            }
        }
    }

    try {
        // The aim of the dimension profile is to provide a positive gradient towards the centre of the cloud.
        // It's not critical that this goes right to the centre.
        auto gridSize = std::array<size_t, 3>{{(size_t)voxelImage->s(), (size_t)voxelImage->t(), (size_t)voxelImage->r()}};
        auto sdf = fmm::SignedArrivalTime(
            gridSize,
            cloudBoundaryIndices,
            cloudBoundaryDistances,
            fmm::DistanceSolver<float, 3>(1.0));

        // The SDF is now calculated, so write it back to the voxel data.
        // The Voxel layout is as follows
        // 0 .r - Dimension.  This is a positive gradient with 1.0 at the center of the cloud, and 0.0 at the edge
        // 1 .g - Type.  From wispy (0.0) to billowy (1.0)
        // 2 .b - Density.  0.0 is no cloud density, 1.0 is fully opaque density.  Use this to determine if there is any cloud at this location.
        // 3 .a - Signed Distance Field in UV space.  The maximum radius sphere centered on this point that doesn't contain any cloud density.
        //        Used for adaptive ray marching.  Negative values represent areas inside the cloud.

        std::size_t idx = 0;
        for (auto k = 0; k < voxelImage->r(); ++k) {
            for (auto j = 0; j < voxelImage->t(); ++j) {
                for (auto i = 0; i < voxelImage->s(); ++i) {
                    // ffm values are < 0.0f as we are inside the boundary, and are the number of voxels to the edge of the cloud.
                    // We want a gradient where 0.0 is at the edge and 1.0 is in the centre, with approximately 3 voxels of distance from
                    // the edge to what we will consider the center.
                    float s = 1.0f / 3.0f * sdf[idx++];
                    float distance = std::min(abs(s), 1.0f);

                    //std::cout << std::setw(6) << std::fixed << std::setprecision(4) << s << " ";
                    osg::Vec4f c = voxelImage->getColor(i, j, k);
                    if (c[2] > 0.0f) {
                        // Write out the dimension profile from the calculation
                        c[0] = distance;
                        c[3] = -1.0f; // -ve SDF inside clouds
                        voxelImage->setColor(c, i, j, k);
                    }
                }
                //std::cout << "\n";
            }
            //std::cout << "\n\n";
        }
    } catch (const std::exception& e) {
        // The fmm may throw exceptions if it is unable to generate an SDF.  Given that
        // our data has a random element, this is insufficient reason to terminate FlightGear,
        // so we will simply log this.
        SG_LOG(SG_ENVIRONMENT, SG_DEV_ALERT, "Cloud Fast Marching Method to generate dimension field  for voxel texture " << voxelImage->getName() << " and threw exception: '" << e.what() << "'. Ignoring.  Cloud rendering will be impacted");
    }

    return voxelImage;
}

// Place a set of cloudVoxels within a particular voxelField
int SGVoxelTextureCloud::addCloudToVoxelField(ImageRef voxelField, ImageRef cloudVoxels, float voxelSize, osg::Vec3f p) const
{
    const float SDFMin = 1.0f / (float)voxelField->s();
    const float halfWidthM = 0.5f * voxelSize * (float)voxelField->s();

    // Determine where to place the origin in the voxel space.
    const int x = (int)((p.x() + halfWidthM) / voxelSize) - cloudVoxels->s() / 2;
    const int y = (int)((p.y() + halfWidthM) / voxelSize) - cloudVoxels->t() / 2;
    const int z = (int)(p.z() / voxelSize);

    SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Cloud " << cloudVoxels->getName() << " : " << cloudVoxels->s() << "x" << cloudVoxels->t() << "x" << cloudVoxels->r() << " - " << "," << z);

    for (int k = 0; k < cloudVoxels->r(); ++k) {
        for (int j = 0; j < cloudVoxels->t(); ++j) {
            for (int i = 0; i < cloudVoxels->s(); ++i) {
                int px = i + x;
                int py = j + y;
                int pz = k + z;

                if (px < 0 || py < 0 || pz < 0) continue;
                if (px > voxelField->s() - 1 || py > voxelField->t() - 1 || pz > voxelField->r() - 1) continue;

                // Produce variant clouds by optionally reflecting the cloud in the X and/or Y axis.  For simplicity we can just
                // do this with a simple coordinate transformation.
                int ii = _reflectX ? cloudVoxels->s() - i - 1 : i;
                int jj = _reflectY ? cloudVoxels->t() - j - 1 : j;

                const osg::Vec4f cloudV = cloudVoxels->getColor(ii, jj, k);
                if (cloudV[2] > 0.0f) {
                    const osg::Vec4f currentV = voxelField->getColor(px, py, pz);
                    // When merging with the existing voxel data we want to take the
                    // maximum Dimension, maximum Type, maximum density.
                    // We set the standard in-cloud SDF minimum as it is standard across all cloud interiors.
                    const osg::Vec4f newV = osg::Vec4f(std::max(cloudV[0], currentV[0]),
                                                       std::max(cloudV[1], currentV[1]),
                                                       std::max(cloudV[2], currentV[2]),
                                                       -SDFMin);
                    voxelField->setColor(newV, px, py, pz);
                }
            }
        }
    }

    return z + cloudVoxels->r() - 1;
}

int SGVoxelTextureCloud::addCloudToDetailedVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const { return addCloudToVoxelField(voxelField, getDetailedCloud(), voxelSize, p); }
int SGVoxelTextureCloud::addCloudToRoughVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const { return addCloudToVoxelField(voxelField, getRoughCloud(), voxelSize, p); }


SGVoxelLayerCloud::SGVoxelLayerCloud(string name, const SGPropertyNode* cld_def, mt* s, float coverage, float thicknessM) : _name(name),
                                                                                                                            _seed(s),
                                                                                                                            _coverage(coverage),
                                                                                                                            _thicknessM(thicknessM)
{
    _minDensity = cld_def->getFloatValue("min-density-norm", 0.5);
    _maxDensity = cld_def->getFloatValue("max-density-norm", 0.5);
    _minType = cld_def->getFloatValue("min-type-norm", 0.5);
    _maxType = cld_def->getFloatValue("max-type-norm", 0.5);

    _perlinNoiseFrequency = cld_def->getIntValue("perlin-noise-frequency", 4);
    _perlinAlpha = cld_def->getFloatValue("perlin-alpha", 1.);
    _perlinBeta = cld_def->getFloatValue("perlin-beta", 2.0);
    _perlinN = cld_def->getIntValue("perlin-n", 6);
}

SGVoxelLayerCloud::~SGVoxelLayerCloud()
{
}

int SGVoxelLayerCloud::addCloudToDetailedVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const { return addCloudToVoxelField(voxelField, voxelSize, p); }
int SGVoxelLayerCloud::addCloudToRoughVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const { return addCloudToVoxelField(voxelField, voxelSize, p); }

float layersmoothstep(float edge0, float edge1, float x)
{
    // Scale, bias and saturate x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

int SGVoxelLayerCloud::addCloudToVoxelField(ImageRef voxelField, float voxelSize, osg::Vec3f p) const
{
    const float SDFMin = 1.0f / (float)voxelField->s();

    // Generate a cloud layer covering the entire voxel field
    float cloudTopM = (p.z() + _thicknessM);
    float cloudBaseM = p.z();

    // For cloud layers less than a voxel in thickness we simply reduce the density
    float densityFactor = std::min(_thicknessM / voxelSize, 1.0f);

    int cloudBaseIdx = (int)std::floor(cloudBaseM / voxelSize);
    int cloudTopIdx = (int)std::ceil(cloudTopM / voxelSize);

    // Ensure we are generating some clouds
    cloudTopIdx = std::max(cloudTopIdx, cloudBaseIdx + 1);

    float cloudCenterM = float(cloudTopIdx + cloudBaseIdx) * 0.5f * voxelSize;

    SG_LOG(SG_ENVIRONMENT, SG_INFO, "Layer base in meters " << cloudBaseM << "m, center " << cloudCenterM << "m top " << cloudTopM);
    SG_LOG(SG_ENVIRONMENT, SG_INFO, "Layer base in voxels " << cloudBaseIdx << ", center " << (cloudCenterM / voxelSize) << " top " << cloudTopIdx << " density factor: " << densityFactor);

    // Generate some perlin noise
    osgUtil::PerlinNoise perlinNoise = osgUtil::PerlinNoise();
    perlinNoise.SetNoiseFrequency(_perlinNoiseFrequency);

    SG_LOG(SG_ENVIRONMENT, SG_INFO, "Generating Cloud layer for " << _name << " of coverage " << _coverage << ", depth " << (cloudTopIdx - cloudBaseIdx) << " with Perlin noise parameters freq: " << _perlinNoiseFrequency << " alpha:" << _perlinAlpha << " beta:" << _perlinBeta << " n:" << _perlinN);

    // Build a runtime inverse-CDF so that _coverage (a fraction in [0,1]) maps to the
    // noise threshold that actually produces that fraction of cloud voxels, regardless
    // of the Perlin parameters chosen.
    //
    // Perlin noise with typical octave parameters is NOT uniformly distributed — it
    // concentrates near 0.5, occupying only ~[0.22, 0.77] of the [0,1] range.  Without
    // this remapping, low coverage values (2/8, 3/8) fall below the noise minimum and
    // produce no cloud at all, while high values (6/8, 7/8) hit the ceiling and become
    // fully overcast.
    //
    // We sample a 64×64 probe grid of the noise (negligible cost — <1ms), sort it, and
    // use it as a piecewise-linear inverse-CDF.  This is exact for any parameter set.

    const int kProbeSide = 64;
    const int kProbeN = kProbeSide * kProbeSide;
    std::vector<float> noiseSamples;
    noiseSamples.reserve(kProbeN);

    for (int pj = 0; pj < kProbeSide; ++pj) {
        for (int pi = 0; pi < kProbeSide; ++pi) {
            double px = (double)pi / (double)kProbeSide;
            double py = (double)pj / (double)kProbeSide;
            float v = (float)perlinNoise.PerlinNoise3D(px, py, 0.0, _perlinAlpha, _perlinBeta, _perlinN);
            noiseSamples.push_back(0.5f * (v + 1.0f));
        }
    }
    std::sort(noiseSamples.begin(), noiseSamples.end());

    auto remapCoverage = [&](float p) -> float {
        if (p <= 0.0f) return noiseSamples.front();
        if (p >= 1.0f) return noiseSamples.back();
        float fIdx = p * float(kProbeN - 1);
        int lo = (int)fIdx;
        float frac = fIdx - float(lo);
        return noiseSamples[lo] + frac * (noiseSamples[lo + 1] - noiseSamples[lo]);
    };

    // Sharp coverage threshold: the noise value below which exactly _coverage
    // fraction of voxels fall.  This is the binary cloud/no-cloud gate and must
    // remain sharp to preserve correct coverage fractions.
    const float coverageThreshold = remapCoverage(_coverage);

    // Edge softness: a separate, wider fade applied only INSIDE the cloud region.
    // We express it as a fraction of the total noise range so it scales naturally
    // with any parameter set.  kEdgeFraction=0.20 means the outermost 20% of each
    // cloud patch's noise range fades from full density down to zero.
    // This is written into the density channel of boundary voxels so the GPU's
    // trilinear sampler interpolates a smooth gradient across the patch edge.
    //
    // The binary gate (noiseVal < coverageThreshold) is kept separate — it is NOT
    // widened — so coverage fractions remain exact.
    const float noiseRange = coverageThreshold - noiseSamples.front();
    const float kEdgeFraction = 0.25f;
    const float fadeSoftness = kEdgeFraction * noiseRange;

    // The layer dimension profile is a pure 1-D vertical gradient — 0.0 at base/top,
    // 1.0 at the vertical centre.  This is what keeps stratus flat-bottomed.
    const int layerDepth = cloudTopIdx - cloudBaseIdx;

    for (int k = cloudBaseIdx; k <= cloudTopIdx; ++k) {
        // Vertical dimension: triangle peaking at layer centre, 0 at base and top.
        float normalizedHeight = float(k - cloudBaseIdx) / float(std::max(layerDepth, 1));
        float verticalDim = 1.0f - std::abs(normalizedHeight * 2.0f - 1.0f);
        // Sharpen slightly so the middle of the layer gets dimension closer to 1.0,
        // but the base and top fall off quickly — keeps the underside flat.
        verticalDim = std::pow(verticalDim, 0.5f);

        for (int j = 0; j < voxelField->t(); ++j) {
            for (int i = 0; i < voxelField->s(); ++i) {
                double x = (double)i / (double)voxelField->s();
                double y = (double)j / (double)voxelField->t();

                // Slightly larger z variation per vertical level so each slice has
                // a subtly different breakup pattern — softens top and bottom edges.
                double z = 0.4 * ((double)k) / (double)voxelField->r();
                float noiseVal = (float)perlinNoise.PerlinNoise3D(x, y, z, _perlinAlpha, _perlinBeta, _perlinN);

                // Perlin noise is in range [-1.0, 1.0], remap to [0.0, 1.0]
                noiseVal = 0.5f * (noiseVal + 1.0f);

                // Binary gate: only place cloud voxels where noiseVal is below the threshold.
                // This keeps coverage fractions exact — the fade below is purely cosmetic.
                if (noiseVal < coverageThreshold) {
                    // The Voxel layout is as follows
                    // 0 .r - Dimension.  This is a positive gradient with 1.0 at the center of the cloud, and 0.0 at the edge.
                    // 1 .g - Type.  From wispy (0.0) to billowy (1.0)
                    // 2 .b - Density.  0.0 is no cloud density, 1.0 is fully opaque density.  Use this to determine if there is any cloud at this location.
                    // 3 .a - Signed Distance Field in UV space.  The maximum radius sphere centered on this point that doesn't contain any cloud density.
                    //        Used for adaptive ray marching.  Negative values represent areas inside the cloud.

                    // Edge density fade: voxels near the patch boundary (noiseVal close to
                    // coverageThreshold) get reduced density so the GPU trilinear sampler
                    // interpolates a soft transition across the 1-2 boundary voxels.
                    // fadeSoftness spans the outermost kEdgeFraction of each patch's noise range,
                    // giving a physically wide soft zone without affecting coverage fractions.
                    float edgeFade = 1.0f - layersmoothstep(coverageThreshold - fadeSoftness,
                                                            coverageThreshold,
                                                            noiseVal);

                    // Map noiseVal (low at cloud centres) to a 0->1 "cloudiness" factor:
                    // denser/more-billowy near the centre of each patch.
                    float cloudiness = 1.0f - (noiseVal / coverageThreshold);
                    cloudiness = clamp(cloudiness, 0.0f, 1.0f);

                    // Make the type more wispy at the edges
                    float type = _minType + cloudiness * (_maxType - _minType);

                    // Density takes into account both the cloudiness, the edgeFade, and a factor
                    // to handle cloud layers less than one voxel in height, allowing creation of
                    // very thin layers or cirrus like cloud.
                    float density = (_minDensity + cloudiness * (_maxDensity - _minDensity)) * edgeFade * densityFactor;

                    const osg::Vec4f currentV = voxelField->getColor(i, j, k);
                    const osg::Vec4f newV = osg::Vec4f(std::max(verticalDim, currentV[0]),
                                                       std::max(type, currentV[1]),
                                                       std::max(density, currentV[2]),
                                                       -SDFMin);
                    voxelField->setColor(newV, i, j, k);
                }
            }
        }
    }

    return cloudTopIdx;
}
