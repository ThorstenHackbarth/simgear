/*
 * SPDX-FileName: GeodEcefRoundtripTest.cxx
 * SPDX-FileComment: Phase-2 audit — assert SGGeod ↔ ECEF round-trip < 1 mm worldwide
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Thorsten Hackbarth <thorsten.hackbarth@gmx.de>
 */

// Migration plan Step 2.3a: the SGGeod ↔ ECEF helpers used by the floating-
// origin system (Section 8.1) must round-trip without losing precision —
// otherwise long-haul flights accumulate drift. SimGear already provides
// SGGeodesy::SGGeodToCart / SGCartToGeod in double precision; this test
// pins the round-trip error budget at sub-millimetre across the globe.

#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGGeodesy.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/misc/test_macros.hxx>

#include <array>
#include <cmath>
#include <random>


namespace {

// Round-trip a single point and return the great-circle position error in
// metres. Uses SimGear's existing SGGeodesy helpers, which operate in
// double-precision ECEF throughout.
double roundtripErrorMetres(const SGGeod& original)
{
    SGVec3<double> cart;
    SGGeodesy::SGGeodToCart(original, cart);

    SGGeod recovered;
    SGGeodesy::SGCartToGeod(cart, recovered);

    // Convert both back to cart and compare in metres — gives a position
    // error invariant to latitude (avoids the cos(lat) wrap-around mess).
    SGVec3<double> recoveredCart;
    SGGeodesy::SGGeodToCart(recovered, recoveredCart);

    const double dx = cart.x() - recoveredCart.x();
    const double dy = cart.y() - recoveredCart.y();
    const double dz = cart.z() - recoveredCart.z();
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace


void testKnownAirportsRoundtripUnderOneMillimetre()
{
    // Hand-picked WGS84 reference points covering the latitude/longitude
    // extremes and a few major airports.
    struct NamedGeod {
        const char* name;
        double lat_deg;
        double lon_deg;
        double alt_m;
    };
    const std::array<NamedGeod, 10> points = {{
        {"KSFO", 37.6189, -122.3750, 4.0},
        {"EGLL", 51.4775,   -0.4614, 25.0},
        {"YSSY", -33.9461, 151.1772, 6.0},
        {"FACT", -33.9648,  18.6017, 46.0},
        {"NZAA", -37.0082, 174.7917, 7.0},
        {"OERK",  24.9576,  46.6988, 614.0},
        {"NORTH_POLE",      89.999,   0.0,     0.0},
        {"SOUTH_POLE",     -89.999,   0.0,     0.0},
        {"EQUATOR_DATE",     0.0,   180.0,     0.0},
        {"NEAR_ANTIPODE",   -0.001, 179.999,   0.0},
    }};

    for (const auto& p : points) {
        SGGeod g = SGGeod::fromDegM(p.lon_deg, p.lat_deg, p.alt_m);
        const double err = roundtripErrorMetres(g);
        // 1 mm = 1e-3 m. SGCartToGeod uses iterative refinement, so 1 micron
        // is realistic for ground-level points; we set the gate at 1 mm so
        // small floating-point drift doesn't flake the test.
        SG_VERIFY(err < 1.0e-3);
    }
}


void testRandomGlobalPointsRoundtripUnderOneMillimetre()
{
    // 1000 random points covering the WGS84 surface, altitudes from
    // -500 m (Dead Sea floor) to 30 000 m (high-altitude flight). Use a
    // fixed seed so failures are reproducible.
    std::mt19937 rng{0xFA17B0DC};
    std::uniform_real_distribution<double> lat_dist(-89.9, 89.9);
    std::uniform_real_distribution<double> lon_dist(-180.0, 180.0);
    std::uniform_real_distribution<double> alt_dist(-500.0, 30000.0);

    double worst_error = 0.0;
    for (int i = 0; i < 1000; ++i) {
        SGGeod g = SGGeod::fromDegM(lon_dist(rng), lat_dist(rng), alt_dist(rng));
        const double err = roundtripErrorMetres(g);
        if (err > worst_error) worst_error = err;
    }
    // Print the worst observed error for diagnostic value; the assertion
    // keeps it under 1 mm.
    std::printf("GeodEcefRoundtripTest: worst round-trip error = %.6f mm\n",
                worst_error * 1000.0);
    SG_VERIFY(worst_error < 1.0e-3);
}


void testEcefMagnitudeNearEarthRadius()
{
    // Sanity check: surface points produce ECEF vectors whose magnitude is
    // close to the WGS84 equatorial radius (6 378 137 m). This protects
    // against an axis swap or unit-confusion regression.
    SGGeod equator = SGGeod::fromDegM(0.0, 0.0, 0.0);
    SGVec3<double> cart;
    SGGeodesy::SGGeodToCart(equator, cart);
    const double r = std::sqrt(cart.x()*cart.x() + cart.y()*cart.y() + cart.z()*cart.z());
    SG_VERIFY(std::abs(r - SGGeodesy::EQURAD) < 1.0e-3);
}


int main()
{
    testKnownAirportsRoundtripUnderOneMillimetre();
    testRandomGlobalPointsRoundtripUnderOneMillimetre();
    testEcefMagnitudeNearEarthRadius();
    return 0;
}
