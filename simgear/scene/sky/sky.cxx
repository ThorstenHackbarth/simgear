// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Curtis L. Olson - http://www.flightgear.org/~curt

#ifdef HAVE_CONFIG_H
    #include <simgear_config.h>
#endif

#include "cloudfield.hxx"
#include "newcloud.hxx"
#include "sky.hxx"

#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/sg_inlines.h>

#include <osg/Depth>
#include <osg/StateSet>

// Constructor
SGSky::SGSky(void)
{
    effective_visibility = visibility = 10000.0;

    clouds_3d_enabled = false;
    clouds_3d_density = 0.8;

    pre_root = new osg::Group;
    pre_root->setName("SGSky-pre-root");
    pre_root->setNodeMask(simgear::BACKGROUND_BIT);
    osg::StateSet* preStateSet = new osg::StateSet;
    preStateSet->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0,
                                             false));
    pre_root->setStateSet(preStateSet);
    cloud_root = new osg::Switch;
    cloud_root->setNodeMask(simgear::MODEL_BIT);
    cloud_root->setName("SGSky-cloud-root");

    pre_transform = new osg::Group;
    pre_transform->setName("SGSky-pre-transform");

    _ephTransform = new osg::MatrixTransform;
    _ephTransform->setName("SGSky-eph-transform");

    // Set up a RNG that is repeatable within 10 minutes to ensure that clouds
    // are synced up in multi-process deployments.
    mt_init_time_10(&seed);
}


// Destructor
SGSky::~SGSky(void)
{
}


// initialize the sky and connect the components to the scene graph at
// the provided branch
void SGSky::build(double h_radius_m,
                  double v_radius_m,
                  double sun_size,
                  double moon_size,
                  const SGEphemeris& eph,
                  SGPropertyNode* property_tree_node,
                  simgear::SGReaderWriterOptions* options)
{
    dome = new SGSkyDome;
    pre_transform->addChild(dome->build(h_radius_m, v_radius_m, options));

    pre_transform->addChild(_ephTransform.get());
    planets = new SGPlanets;
    _ephTransform->addChild(planets->build(eph.getNumPlanets(), eph.getPlanets(), h_radius_m, options));

    stars = new SGStars;
    _ephTransform->addChild(stars->build(eph.getNumStars(), eph.getStars(), h_radius_m, options));

    galaxy = new SGGalaxy;
    _ephTransform->addChild(galaxy->build(h_radius_m, options));

    moon = new SGMoon;
    _ephTransform->addChild(moon->build(moon_size, options));

    oursun = new SGSun;
    _ephTransform->addChild(oursun->build(sun_size, property_tree_node, options));

    pre_root->addChild(pre_transform.get());
}


// repaint the sky components based on current value of sun_angle,
// sky, and fog colors.
//
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGSky::repaint(const SGSkyColor& sc, const SGEphemeris& eph)
{
    return true;
}

// reposition the sky at the specified origin and orientation
//
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (this allows
// additional orientation for the sunrise/set effects and is used by
// the skydome and perhaps clouds.
bool SGSky::reposition(const SGSkyState& st, const SGEphemeris& eph, double dt)
{
    double angle = st.gst * 15; // degrees
    double angleRad = SGMiscd::deg2rad(angle);

    SGVec3f zero_elev, view_up;
    double lon, lat, alt, lst;

    SGGeod geodZeroViewPos = SGGeod::fromGeodM(st.pos_geod, 0);
    zero_elev = toVec3f(SGVec3d::fromGeod(geodZeroViewPos));

    // calculate the scenery up vector
    SGQuatd hlOr = SGQuatd::fromLonLat(st.pos_geod);
    view_up = toVec3f(hlOr.backTransform(-SGVec3d::e3()));

    // viewer location
    lon = st.pos_geod.getLongitudeRad();
    lat = st.pos_geod.getLatitudeRad();
    alt = st.pos_geod.getElevationM();
    // Local sidereal time
    lst = angleRad + lon;

    dome->reposition(zero_elev, alt, lon, lat, st.spin);

    osg::Matrix m = osg::Matrix::rotate(angleRad, osg::Vec3(0, 0, -1));
    m.postMultTranslate(toOsg(st.pos));
    _ephTransform->setMatrix(m);

    double sun_ra = eph.getSunRightAscension();
    double sun_dec = eph.getSunDeclination();
    oursun->reposition(sun_ra, sun_dec, st.sun_dist, lat, alt, st.sun_angle);

    double moon_ra = eph.getMoonRightAscension();
    double moon_dec = eph.getMoonDeclination();
    double moon_r = eph.getMoonDistanceInMayorAxis();

    //this allows to render the moon closer to the viewer when the
    //moon is closer to the center of Earth, times any articial extra factors
    double moon_dist_factor = moon_r * st.moon_dist_factor;
    moon->reposition(moon_ra, moon_dec, st.moon_dist_bare, moon_dist_factor, lst, lat, alt);

    for (unsigned i = 0; i < cloud_layers.size(); ++i) {
        if (cloud_layers[i]->getCoverage() != SGCloudLayer::SG_CLOUD_CLEAR ||
            cloud_layers[i]->get_layer3D()->isDefined3D()) {
            cloud_layers[i]->reposition(zero_elev, view_up, lon, lat, alt, dt);
        } else {
            cloud_layers[i]->getNode()->setAllChildrenOff();
        }
    }

    return true;
}

void SGSky::set_visibility(float v)
{
    visibility = std::max(v, 25.0f);
}

void SGSky::add_cloud_layer(SGCloudLayer* layer)
{
    cloud_layers.push_back(layer);
    cloud_root->addChild(layer->getNode(), true);

    layer->set_enable3dClouds(clouds_3d_enabled);
}

const SGCloudLayer*
SGSky::get_cloud_layer(int i) const
{
    return cloud_layers[i];
}

SGCloudLayer*
SGSky::get_cloud_layer(int i)
{
    return cloud_layers[i];
}

int SGSky::get_cloud_layer_count() const
{
    return cloud_layers.size();
}

float SGSky::get_3dCloudVisRange() const
{
    return SGCloudField::getVisRange();
}

void SGSky::set_3dCloudVisRange(float vis)
{
    SGCloudField::setVisRange(vis);
    for (int i = 0; i < (int)cloud_layers.size(); ++i) {
        cloud_layers[i]->get_layer3D()->applyVisAndLoDRange();
    }
}

bool SGSky::get_3dCloudWrap() const
{
    return SGCloudField::getWrap();
}

void SGSky::set_3dCloudWrap(bool wrap)
{
    SGCloudField::setWrap(wrap);
}

// modify the current visibility based on cloud layers, thickness,
// transition range, and simulated "puffs".
void SGSky::modify_vis(float alt, float time_factor)
{
    float effvis = visibility;

    for (int i = 0; i < (int)cloud_layers.size(); ++i) {
        float asl = cloud_layers[i]->getElevation_m();
        float thickness = cloud_layers[i]->getThickness_m();
        float transition = cloud_layers[i]->getTransition_m();

        double ratio = 1.0;

        if (cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_CLEAR) {
            // less than 50% coverage -- assume we're in the clear for now
            ratio = 1.0;
        } else if (alt < asl - transition) {
            // below cloud layer
            ratio = 1.0;
        } else if (alt < asl) {
            // in lower transition
            ratio = (asl - alt) / transition;
        } else if (alt < asl + thickness) {
            // in cloud layer
            ratio = 0.0;
        } else if (alt < asl + thickness + transition) {
            // in upper transition
            ratio = (alt - (asl + thickness)) / transition;
        } else {
            // above cloud layer
            ratio = 1.0;
        }

        if (cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_CLEAR ||
            cloud_layers[i]->get_layer3D()->isDefined3D()) {
            // do nothing, clear layers aren't drawn, don't affect
            // visibility andn don't need to be faded in or out.
        } else if ((cloud_layers[i]->getCoverage() ==
                    SGCloudLayer::SG_CLOUD_FEW) ||
                   (cloud_layers[i]->getCoverage() ==
                    SGCloudLayer::SG_CLOUD_SCATTERED)) {
            // set the alpha fade value for the cloud layer.  For less
            // dense cloud layers we fade the layer to nothing as we
            // approach it because we stay clear visibility-wise as we
            // pass through it.
            float temp = ratio * 2.0;
            if (temp > 1.0) { temp = 1.0; }
            cloud_layers[i]->setAlpha(temp);

            // don't touch visibility
        } else {
            // maintain full alpha for denser cloud layer types.
            // Let's set the value explicitly in case someone changed
            // the layer type.
            cloud_layers[i]->setAlpha(1.0);

            // lower visibility as we approach the cloud layer.
            // accumulate effects from multiple cloud layers
            effvis *= ratio;
        }

        // never let visibility drop below the layer's configured visibility
        effvis = SG_MAX2<float>(cloud_layers[i]->getVisibility_m(), effvis);

    } // for

    effective_visibility = effvis;
}

void SGSky::set_clouds_enabled(bool enabled)
{
    if (enabled) {
        cloud_root->setAllChildrenOn();
    } else {
        cloud_root->setAllChildrenOff();
    }
}
