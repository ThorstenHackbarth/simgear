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
