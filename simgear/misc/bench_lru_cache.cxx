// SPDX-License-Identifier: LGPL-2.1-or-later
// Micro-benchmarks for simgear::lru_cache modelling real aircraft
// texture/resource cache access patterns.
//
// Simulated workload: a mix of 70% hot-key hits and 30% cold misses,
// matching typical FG texture lookup behaviour.

#include <benchmark/benchmark.h>
#include <simgear/misc/lru_cache.hxx>
#include <string>

static constexpr size_t kCacheSize = 256;
static constexpr size_t kHotKeys   = 32;   // frequently accessed
static constexpr size_t kColdKeys  = 1024; // wider address space

// --------------------------------------------------------------------------
// Hot-path: all accesses hit the cache (best case)
static void BM_LruCache_HotHit(benchmark::State& state) {
    simgear::lru_cache<std::string, std::string> cache(kCacheSize);
    for (size_t i = 0; i < kHotKeys; ++i)
        cache.insert("key" + std::to_string(i), "value" + std::to_string(i));

    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get("key" + std::to_string(idx % kHotKeys));
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetLabel("all-hits");
}
BENCHMARK(BM_LruCache_HotHit);

// --------------------------------------------------------------------------
// Mixed: 70% hit / 30% miss (cold eviction), realistic FG texture loading
static void BM_LruCache_MixedAccess(benchmark::State& state) {
    simgear::lru_cache<std::string, std::string> cache(kCacheSize);
    for (size_t i = 0; i < kCacheSize; ++i)
        cache.insert("key" + std::to_string(i), "val" + std::to_string(i));

    size_t idx = 0;
    for (auto _ : state) {
        // 70% hot, 30% cold
        size_t key = (idx % 10 < 7) ? (idx % kHotKeys)
                                     : (kCacheSize + idx % kColdKeys);
        auto v = cache.get("key" + std::to_string(key));
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetLabel("70%hit/30%miss");
}
BENCHMARK(BM_LruCache_MixedAccess);

// --------------------------------------------------------------------------
// Insert: steady stream of new keys (eviction stress)
static void BM_LruCache_Insert(benchmark::State& state) {
    simgear::lru_cache<std::string, std::string> cache(kCacheSize);
    size_t idx = 0;
    for (auto _ : state) {
        cache.insert("key" + std::to_string(idx), "val" + std::to_string(idx));
        ++idx;
    }
}
BENCHMARK(BM_LruCache_Insert);

// --------------------------------------------------------------------------
// FindValue: linear scan through cache values (path lookup scenario)
static void BM_LruCache_FindValue(benchmark::State& state) {
    simgear::lru_cache<std::string, std::string> cache(kCacheSize);
    for (size_t i = 0; i < kCacheSize; ++i)
        cache.insert("key" + std::to_string(i), "/Models/Aircraft/texture_" + std::to_string(i) + ".png");

    size_t idx = 0;
    for (auto _ : state) {
        auto k = cache.findValue("/Models/Aircraft/texture_" + std::to_string(idx % kCacheSize) + ".png");
        benchmark::DoNotOptimize(k);
        ++idx;
    }
    state.SetLabel("linear-scan");
}
BENCHMARK(BM_LruCache_FindValue);

BENCHMARK_MAIN();
