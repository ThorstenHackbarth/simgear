// SPDX-License-Identifier: LGPL-2.1-or-later
// Micro-benchmarks for the SGPropertyNode system, modelling the FG
// property tree access patterns: avionics, autopilot, and FDM property
// reads/writes at ~60Hz frame rate with hundreds of active properties.

#include <benchmark/benchmark.h>
#include <simgear/props/props.hxx>
#include <string>

// --------------------------------------------------------------------------
// Helpers: build a realistic aircraft property tree hierarchy
static SGPropertyNode_ptr buildTree() {
    auto root = new SGPropertyNode;
    // Simulate /fdm/jsbsim/atmosphere/*, /instrumentation/*, /autopilot/*
    root->getNode("fdm/jsbsim/atmosphere/temperature-sl-degk", true)->setFloatValue(288.15f);
    root->getNode("fdm/jsbsim/atmosphere/pressure-sl-inhg",  true)->setFloatValue(29.92f);
    root->getNode("fdm/jsbsim/velocities/airspeed-kt",       true)->setFloatValue(150.0f);
    root->getNode("fdm/jsbsim/velocities/groundspeed-kt",    true)->setFloatValue(148.0f);
    root->getNode("fdm/jsbsim/position/altitude-ft",         true)->setDoubleValue(8000.0);
    root->getNode("fdm/jsbsim/position/latitude-deg",        true)->setDoubleValue(47.8);
    root->getNode("fdm/jsbsim/position/longitude-deg",       true)->setDoubleValue(8.6);
    root->getNode("instrumentation/altimeter/indicated-altitude-ft", true)->setFloatValue(7980.0f);
    root->getNode("instrumentation/airspeed-indicator/indicated-speed-kt", true)->setFloatValue(149.0f);
    root->getNode("autopilot/settings/target-altitude-ft",   true)->setFloatValue(8000.0f);
    root->getNode("autopilot/settings/target-speed-kt",      true)->setFloatValue(150.0f);
    for (int i = 0; i < 32; ++i)
        root->getNode("ai/models/aircraft[" + std::to_string(i) + "]/position/latitude-deg", true)
            ->setDoubleValue(47.0 + i * 0.01);
    return root;
}

// --------------------------------------------------------------------------
// Read a double property by path: typical FDM read
static void BM_Props_GetDouble(benchmark::State& state) {
    auto root = buildTree();
    for (auto _ : state) {
        double v = root->getDoubleValue("fdm/jsbsim/position/altitude-ft");
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_Props_GetDouble);

// --------------------------------------------------------------------------
// Write a float property: autopilot output each frame
static void BM_Props_SetFloat(benchmark::State& state) {
    auto root = buildTree();
    float v = 8000.0f;
    for (auto _ : state) {
        root->setFloatValue("autopilot/settings/target-altitude-ft", v);
        v += 0.01f;
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_Props_SetFloat);

// --------------------------------------------------------------------------
// getNode with create=false: cached node resolution (hot path)
static void BM_Props_GetNode_Cached(benchmark::State& state) {
    auto root = buildTree();
    // pre-resolve node
    auto node = root->getNode("fdm/jsbsim/velocities/airspeed-kt", false);
    for (auto _ : state) {
        float v = node->getFloatValue();
        benchmark::DoNotOptimize(v);
    }
    state.SetLabel("direct-node");
}
BENCHMARK(BM_Props_GetNode_Cached);

// --------------------------------------------------------------------------
// getNode with path lookup: uncached path resolution
static void BM_Props_GetNode_Path(benchmark::State& state) {
    auto root = buildTree();
    for (auto _ : state) {
        auto n = root->getNode("instrumentation/airspeed-indicator/indicated-speed-kt");
        float v = n ? n->getFloatValue() : 0.0f;
        benchmark::DoNotOptimize(v);
    }
    state.SetLabel("path-lookup");
}
BENCHMARK(BM_Props_GetNode_Path);

// --------------------------------------------------------------------------
// AI traffic scan: iterate 32 AI aircraft positions (realistic AI workload)
static void BM_Props_AITrafficScan(benchmark::State& state) {
    auto root = buildTree();
    for (auto _ : state) {
        double sum = 0.0;
        for (int i = 0; i < 32; ++i) {
            auto n = root->getNode("ai/models/aircraft[" + std::to_string(i) + "]/position/latitude-deg");
            if (n) sum += n->getDoubleValue();
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetLabel("32-aircraft-scan");
}
BENCHMARK(BM_Props_AITrafficScan);

// --------------------------------------------------------------------------
// getDisplayName: called by property browser / debug UI
static void BM_Props_GetDisplayName(benchmark::State& state) {
    auto root = buildTree();
    auto node = root->getNode("fdm/jsbsim/position/altitude-ft");
    for (auto _ : state) {
        std::string s = node->getDisplayName(true);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Props_GetDisplayName);

BENCHMARK_MAIN();
