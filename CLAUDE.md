# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is SimGear?

SimGear is a C++20 shared simulation library used primarily by FlightGear. It provides math, scene graph, scripting, property system, networking, audio, and other subsystems. Version: 2024.2.0.

## Building

Out-of-source builds are required. CMake 3.20+ is needed.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja
ninja
```

Key CMake options:

| Option | Default | Purpose |
|--------|---------|---------|
| `SIMGEAR_HEADLESS` | OFF | Build without graphics (no OSG/OpenGL needed) |
| `SIMGEAR_SHARED` | OFF | Build shared libraries |
| `ENABLE_TESTS` | ON | Build test executables |
| `ENABLE_SOUND` | ON | OpenAL/AeonWave audio support |
| `ENABLE_GDAL` | OFF | Geospatial raster support |
| `ENABLE_CCACHE` | OFF | Compiler cache |
| `ENABLE_SIMD` | ON | SSE/SSE2 optimizations |
| `ENABLE_CYCLONE` | ON | CycloneDDS support |

Required dependencies: Boost (headers), zlib, CURL, LibLZMA, c-ares 1.17+, Threads.  
Graphics adds: OpenSceneGraph (FlightGear fork), OpenGL.

## Testing

```bash
cd build
ctest --output-on-failure          # run all tests
ctest -R SGMathTest                # run a single test by name
ctest --output-on-failure -j4      # parallel
```

Test sources live alongside their subject code as `*Test.cxx` files. They use Boost.Test and are registered via `add_simgear_autotest()` in the local `CMakeLists.txt`.

## Code Style

Formatting is enforced via `.clang-format` (C++11 profile): 4-space indentation, no tabs, left-aligned pointers, no column limit. Run `clang-format -i <file>` before committing.

Pre-commit hooks (`pre-commit run --all-files`) check trailing whitespace, EOF newlines, spelling (codespell), YAML formatting, and SPDX license headers (REUSE). The `3rdparty/` tree is excluded from all checks.

## Architecture

All library code lives under `simgear/`. The two CMake targets are:

- **SimGearCore** — everything except scene/canvas/nasal
- **SimGearScene** — adds 3D rendering, OSG-dependent code

### Core subsystems

| Directory | Role |
|-----------|------|
| `math/` | Vectors, quaternions, matrices, geodesy (SGVec*, SGQuat, SGGeod, SGGeoc) |
| `props/` | Hierarchical property tree — the global runtime state store shared with FlightGear |
| `nasal/` | Nasal scripting language runtime + C++ binding layer (`cppbind/`) |
| `io/` | File I/O, HTTP client, TCP/UDP sockets, async DNS (c-ares), serial ports |
| `package/` | Package/catalog management for downloadable scenery and aircraft |
| `emesary/` | Pub/sub notification bus used for loose coupling between subsystems |
| `misc/` | String utils, path handling, platform abstraction, `sg_exception` base |
| `threads/` | POSIX/Windows thread wrappers, mutexes, condition variables |
| `xml/` | Thin wrapper around bundled expat; `easyxml.hxx` is the main API |
| `bucket/` | Geospatial tile-bucket addressing for terrain data |
| `magvar/` | WMM magnetic variation model |
| `ephemeris/` | Celestial body positions (sun, moon, planets) |
| `environment/` | Atmosphere, metar, precipitation simulation |
| `sound/` | OpenAL/AeonWave audio engine abstraction |

### Scene subsystem (`scene/`)

Only linked when `SIMGEAR_HEADLESS` is OFF. Built on OpenSceneGraph:

| Directory | Role |
|-----------|------|
| `model/` | AC3D/glTF model loading, animation, LOD, particles |
| `tgdb/` | Terrain database streaming and tile geometry generation |
| `sky/` | Skybox, clouds, celestial rendering |
| `material/` | Surface material/texture properties |
| `dem/` | Digital elevation model raster support (requires GDAL) |
| `tsync/` | Terrain tile synchronization/download |
| `viewer/` | Camera, view management |

### Property system pattern

The `SGPropertyNode` tree (`simgear/props/`) is the central shared-state mechanism. Most subsystems read/write named properties rather than calling each other directly. Property paths follow the `/category/subcategory/leaf` convention established by FlightGear.

### Nasal bindings

`simgear/nasal/cppbind/` provides the bridge between C++ objects and Nasal scripts. Ghost types (`nasal::Ghost<>`) expose C++ classes to Nasal; `NasalContext` / `NasalCallContext` handle call marshalling.

## CI

GitLab CI (`.gitlab-ci.yml`) builds on Linux (Ninja/RelWithDebInfo), Windows (MSVC 2022), and macOS (universal arm64+x86_64). A passing SimGear build triggers downstream FlightGear CI on the same branch. Test results are exported as JUnit XML artifacts.

## Boost-free build (`SIMGEAR_NO_BOOST`)

The `claude/remove-boost-dependencies-meYK7` branch adds an option to build
SimGear without Boost, using C++20 std replacements under
`simgear/compat/`. Enable with `cmake -DSIMGEAR_NO_BOOST=ON` (passes
`-DSG_NO_BOOST` to the compiler).

### Compat header inventory

| Header | Replaces | Notes |
|---|---|---|
| `optional.hxx`         | `boost/optional.hpp`               | typedef alias for `std::optional<T>` |
| `functional_hash.hxx`  | `boost/functional/hash.hpp`        | classic `0x9e3779b9` combine; see Hash note below |
| `tokenizer.hxx`        | `boost/tokenizer.hpp`              | subset: `char_separator`, `tokenizer::const_iterator`, `current_token()` |
| `call_traits.hxx`      | `boost/call_traits.hpp`            | `param_type` specialisations |
| `mpl_has_xxx.hxx`      | `boost/mpl/has_xxx.hpp`            | macro using `std::void_t` |
| `iterator_facade.hxx`  | `boost/iterator/iterator_facade.hpp` | CRTP with arrow_proxy for proxy refs |
| `iterator_adaptor.hxx` | `boost/iterator/iterator_adaptor.hpp` | inherits iterator_facade with use_default resolution |
| `algorithm_string.hxx` | `boost/algorithm/string/*` + `boost/range.hpp` | `iterator_range`, `equals`, `first_finder`, `make_split_iterator` |

Call-site pattern (every production file uses this):
```cpp
#ifdef SG_NO_BOOST
#  include <simgear/compat/foo.hxx>
#else
#  include <boost/foo.hpp>
#endif
```

### Verification tools (repo root)

**`compare_boost_paths.py`** — diff Boost code paths between two branches
without needing Boost installed. Strips `#ifdef SG_NO_BOOST` blocks
(treating the macro as undefined) and normalises preprocessor-directive
whitespace, then diffs the result.
```bash
python3 compare_boost_paths.py --base next --new claude/remove-boost-dependencies-meYK7
```

**`run_benchmark.sh`** — build correctness + performance comparison.
Installs `libboost-dev` via apt (Boost ≥ 1.34 required; 1.83 provided on
Ubuntu 24.04), compiles `compat_benchmark.cxx` twice, diffs the
deterministic correctness output, then prints a side-by-side ns/op table.
No OSG needed (compat layer has no OSG dependency).
```bash
bash run_benchmark.sh
```

### Key findings

- **Functional output identical** for tokenizer, optional, equals,
  `split_iterator`, `iterator_facade` (50 diffable lines, zero diffs).
- **Hash values differ by design.** Boost 1.83 on 64-bit platforms with
  `__int128` uses a Murmur2-based `hash_combine`:
  `k *= m; k ^= k >> 47; k *= m; h ^= k; h *= m; h += 0xe6546b64`.
  The compat uses the classic 32-bit formula:
  `seed ^= hash(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2)`. Individual
  `hash_value<int>` matches (identity cast), but any `hash_combine` /
  `hash_range` output will differ. Both satisfy the hash contract.
- **Performance** (g++ 13.3, -O2, x86-64, compat / Boost ratio):
  tokenizer 0.72×, `hash_value<int>` 0.39×, `split_iterator` 0.77×,
  `equals` 0.89×, others within ±10 %. Compat is never meaningfully
  slower.

### Residual Boost-path diffs (found by compare_boost_paths.py)

Three files show real (but functionally equivalent) differences even when
`SG_NO_BOOST` is undefined:

- `simgear/canvas/events/KeyboardEvent.cxx` — `#if BOOST_VERSION >= 104800`
  gained extra `!defined(SG_NO_BOOST) && defined(BOOST_VERSION) &&`
  guards. Evaluates the same when Boost is in use.
- `simgear/nasal/cppbind/NasalHash.hxx` — comment appended to a
  `#include` line; stripped by the preprocessor.
- `simgear/scene/material/EffectBuilder.hxx` — two `using` declarations
  moved within the same namespace scope; ODR-compatible.
