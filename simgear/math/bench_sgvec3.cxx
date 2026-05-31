// SPDX-License-Identifier: LGPL-2.1-or-later
// Micro-benchmarks for SGVec3 math operations, modelling FDM and coordinate
// transformation workloads in FlightGear (position updates, great-circle
// distance, attitude conversions).

#include <benchmark/benchmark.h>
#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGQuat.hxx>
#include <cmath>
#include <vector>

static const size_t kVecCount = 1024;

// Pre-build input vectors once
static std::vector<SGVec3d> makeVectors(size_t n) {
    std::vector<SGVec3d> v;
    v.reserve(n);
    for (size_t i = 0; i < n; ++i)
        v.emplace_back(std::sin(i * 0.01), std::cos(i * 0.01), i * 0.001);
    return v;
}

// --------------------------------------------------------------------------
// Dot product: aircraft forces projection, happens thousands of times per frame
static void BM_SGVec3_Dot(benchmark::State& state) {
    auto vecs = makeVectors(kVecCount);
    size_t idx = 0;
    for (auto _ : state) {
        double d = dot(vecs[idx % kVecCount], vecs[(idx + 1) % kVecCount]);
        benchmark::DoNotOptimize(d);
        ++idx;
    }
}
BENCHMARK(BM_SGVec3_Dot);

// --------------------------------------------------------------------------
// Cross product: angular velocity, lift vector computation
static void BM_SGVec3_Cross(benchmark::State& state) {
    auto vecs = makeVectors(kVecCount);
    size_t idx = 0;
    for (auto _ : state) {
        auto r = cross(vecs[idx % kVecCount], vecs[(idx + 1) % kVecCount]);
        benchmark::DoNotOptimize(r);
        ++idx;
    }
}
BENCHMARK(BM_SGVec3_Cross);

// --------------------------------------------------------------------------
// Normalize: used every frame for normal vectors, velocity directions
static void BM_SGVec3_Normalize(benchmark::State& state) {
    auto vecs = makeVectors(kVecCount);
    size_t idx = 0;
    for (auto _ : state) {
        auto r = normalize(vecs[idx % kVecCount]);
        benchmark::DoNotOptimize(r);
        ++idx;
    }
}
BENCHMARK(BM_SGVec3_Normalize);

// --------------------------------------------------------------------------
// Length: distance checks, collision detection
static void BM_SGVec3_Length(benchmark::State& state) {
    auto vecs = makeVectors(kVecCount);
    size_t idx = 0;
    for (auto _ : state) {
        double l = length(vecs[idx % kVecCount]);
        benchmark::DoNotOptimize(l);
        ++idx;
    }
}
BENCHMARK(BM_SGVec3_Length);

// --------------------------------------------------------------------------
// Comparison: sorting / ordering of position vectors (map/set usage)
static void BM_SGVec3_LessThan(benchmark::State& state) {
    auto vecs = makeVectors(kVecCount);
    size_t idx = 0;
    for (auto _ : state) {
        bool r = (vecs[idx % kVecCount] < vecs[(idx + 1) % kVecCount]);
        benchmark::DoNotOptimize(r);
        ++idx;
    }
}
BENCHMARK(BM_SGVec3_LessThan);

// --------------------------------------------------------------------------
// Full FDM step simulation: dot + cross + normalize in sequence
static void BM_SGVec3_FDMStep(benchmark::State& state) {
    auto vecs = makeVectors(kVecCount);
    size_t idx = 0;
    for (auto _ : state) {
        const auto& v1 = vecs[idx % kVecCount];
        const auto& v2 = vecs[(idx + 1) % kVecCount];
        auto lift    = cross(v1, v2);
        auto drag    = normalize(v1 - v2);
        double power = dot(lift, drag);
        benchmark::DoNotOptimize(power);
        ++idx;
    }
    state.SetLabel("cross+normalize+dot");
}
BENCHMARK(BM_SGVec3_FDMStep);

BENCHMARK_MAIN();
