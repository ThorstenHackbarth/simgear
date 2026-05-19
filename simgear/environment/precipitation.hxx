// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 Nicolas Vivien

/**
 * @file
 * @brief Precipitation effects to draw rain and snow.
 */

#ifndef _PRECIPITATION_HXX
#define _PRECIPITATION_HXX

#include <osg/Group>
#include <osg/Referenced>
#include <osgParticle/PrecipitationEffect>

#include <simgear/scene/Handle.hxx>

// MIGRATION NOTE (Phase 1, Step 1.5):
// SGPrecipitation is OSG-by-design (inherits osg::Referenced, returns
// osg::Group*, holds an osg::ref_ptr to an osgParticle effect). A full
// backend-neutral redesign lands in Phase 6 (sky / clouds / precipitation
// port). For now this header exposes Handle.hxx so callers can migrate
// to NodeHandle-flavored signatures as soon as the wicked backend lands a
// SGPrecipitation replacement; until then the OSG API is the only one
// available.


class SGPrecipitation : public osg::Referenced
{
protected:
    bool _freeze;
    bool _enabled;
    bool _droplet_external;

    float _snow_intensity;
    float _rain_intensity;
    float _clip_distance;
    float _rain_droplet_size;
    float _snow_flake_size;
    float _illumination;
	
    osg::Vec3 _wind_vec;
	
    osg::ref_ptr<osgParticle::PrecipitationEffect> _precipitationEffect;

public:
    SGPrecipitation();
    virtual ~SGPrecipitation() {}
    osg::Group* build(void);
    bool update(void);
	
    void setWindProperty(double, double);
    void setFreezing(bool);
    void setDropletExternal(bool);
    void setRainIntensity(float);
    void setSnowIntensity(float);
    void setRainDropletSize(float);
    void setSnowFlakeSize(float);
    void setIllumination(float);
    void setClipDistance(float);

    void setEnabled(bool);
    bool getEnabled() const;
};

#endif
