# SPEC.md — Repository Specification Document

<!--
  TEMPLATE VERSION: 1.0.0
  LAST UPDATED: 2026-03-31

  This is the canonical specification template for all repositories in the
  D-sorganization fleet. Every repo MUST have a SPEC.md at its root.

  INSTRUCTIONS:
  1. Copy this template to the root of your repository as SPEC.md
  2. Fill in every section — leave nothing as "[TODO]"
  3. Keep this document updated with every PR that changes functionality
  4. CI will block merges if SPEC.md is stale (source changed but spec didn't)

  AUDIENCE: This document is designed for both human developers AND AI agents.
  Write clearly, use concrete examples, and avoid ambiguity.
-->

## 1. Identity

| Field | Value |
|-------|-------|
| **Repository Name** | `QuatEngine` |
| **GitHub URL** | `https://github.com/D-sorganization/QuatEngine` |
| **Owner** | D-sorganization |
| **Primary Language(s)** | C++17 |
| **License** | MIT |
| **Spec Version** | 1.0.18 |
| **Last Spec Update** | 2026-05-22 |

## 2. Purpose & Mission

QuatEngine is a modern C++ 3D game engine built from first principles with a focus on gimbal-lock-free rotations using quaternion mathematics and smooth SLERP interpolation. The engine combines educational value with practical functionality, supporting both third-person shooter (TPS) and first-person shooter (FPS) game modes with Blinn-Phong lighting, particle systems, and post-processing effects.

## 3. Goals & Non-Goals

### Goals

- Implement robust quaternion mathematics for gimbal-lock-free 3D rotation
- Develop SLERP-smoothed camera interpolation for fluid viewing angles
- Build SDL2 and OpenGL 3.3 rendering pipeline
- Support Blinn-Phong lighting with fog effects
- Implement both TPS (third-person) and FPS (first-person) game modes
- Include particle system and post-processing pipeline
- Provide visual comparison of NLERP vs. SLERP interpolation
- Achieve A-tier educational clarity in code and architecture
- Support cross-platform compilation (Linux, macOS, Windows via CMake)

### Non-Goals

- Not a production AAA game engine (educational focus takes precedence)
- Not a general-purpose rendering framework (game-specific features prioritized)
- Not a replacement for Unreal, Unity, or Godot
- Not intended for mobile or WebGL targets
- No built-in mesh loading or asset pipeline (Phase 4 dependent; OBJLoader exists but is renderer-only, not a general pipeline)

## 4. Architecture Overview

### System Context

QuatEngine is a standalone, self-contained engine with no external fleet dependencies. It depends on SDL2 (auto-fetched via CMake FetchContent) and OpenGL. Optionally, the project roadmap includes potential migration to Unreal Engine as Phase 6, but this is exploratory and not a dependency.

The runtime currently exposes gameplay input only; it does not ship editable
application widgets such as spin boxes, combo boxes, sliders, or text-entry
fields. Mouse-wheel input is therefore reserved for gameplay controls
(camera zoom plus TPS weapon selection) rather than mutable UI values.

### Module Map

```
QuatEngine/
├── src/
│   ├── math/                    # Vector, quaternion, matrix math
│   │   ├── Vec3.h
│   │   ├── Quaternion.h
│   │   └── Mat4.h
│   ├── core/                    # Component system, transforms, utilities
│   │   ├── Transform.h
│   │   ├── ConfigManager.h      # Runtime config defaults, dotenv, and environment overrides
│   │   ├── EngineConfig.h       # Accessors for resolved engine configuration values
│   │   ├── Rng.h               # Shared xorshift32 PRNG
│   │   ├── Logger.h            # Structured logging (DEBUG/INFO/WARN/ERROR)
│   │   ├── Metrics.h           # In-process observability counters
│   │   └── Readiness.h         # Embeddable /alive and /ready status records
│   ├── renderer/                # Graphics pipeline
│   │   ├── GLLoader.h
│   │   ├── Shader.h
│   │   ├── Mesh.h
│   │   ├── MeshPrimitives.h    # Procedural mesh factory functions
│   │   ├── OBJParser.h         # Pure-C++ OBJ text parser (no GL/SDL)
│   │   ├── OBJLoader.h         # GPU-upload wrapper around OBJParser
│   │   └── Camera.h
│   ├── input/                   # SDL2 input handling
│   │   └── InputManager.h
│   ├── game/
│   │   ├── tps/                 # Third-person shooter subsystem
│   │   │   ├── TPSController.h
│   │   │   ├── Combat.h
│   │   │   ├── AI.h
│   │   │   ├── LevelTypes.h     # Level data contracts and objective types
│   │   │   ├── LevelLoader.h    # JSON level loading and parsing
│   │   │   └── LevelSystem.h    # LevelManager state machine facade
│   │   └── fps/                 # First-person shooter subsystem
│   │       └── FPSController.h
│   ├── demo/                    # FPS demo composition/runtime modules
│   │   ├── App.h
│   │   ├── Bootstrap.cpp
│   │   ├── RuntimeSession.h
│   │   ├── RuntimeSystems.cpp
│   │   └── Rendering.cpp
│   └── main.cpp                 # Thin composition root for qe_demo
├── shaders/                      # GLSL 3.30 shader collection (10 files)
│   ├── basic.vert
│   ├── basic.frag
│   ├── blinn_phong.frag
│   ├── post_process.frag
│   └── ...
├── .benchmarks/                  # Deterministic native benchmark probes
├── tests/                        # 22 C++ test files + shared framework
│   └── test_framework.h          # Shared assertion macros and test runner
├── CMakeLists.txt               # CMake build configuration
├── conanfile.txt                # (Optional) Conan package manager
└── .github/workflows/           # CI/CD pipelines
    ├── ci.yml                   # Standard CI (Ubuntu, coverage)
    └── heavy-integration-tests.yml
```

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| Math Library | `src/math/` | Vec3, Quaternion, Mat4 with SLERP, matrix ops |
| Transform Component | `src/core/Transform.h` | Game object positioning and rotation |
| Runtime Configuration | `src/core/ConfigManager.h`, `src/core/EngineConfig.h` | Centralized engine defaults with dotenv and environment-variable overrides |
| PRNG | `src/core/Rng.h` | Deterministic xorshift32 random number generator |
| Logger | `src/core/Logger.h` | Structured logging with compile-time and runtime level control |
| Metrics | `src/core/Metrics.h` | In-process counters with Prometheus text serialization for health/readiness probes |
| Readiness | `src/core/Readiness.h` | Embeddable `/alive` and `/ready` status records for host processes |
| GL Renderer | `src/renderer/GLLoader.h` | OpenGL initialization and context management |
| Shader System | `src/renderer/Shader.h` | GLSL compilation, linking, and uniform management |
| Mesh | `src/renderer/Mesh.h` | Geometry data (vertices, indices, normals, UVs) |
| Mesh Primitives | `src/renderer/MeshPrimitives.h` | Procedural cube, sphere, grid, floor, and related primitive mesh factories |
| Camera | `src/renderer/Camera.h` | View matrix, projection, SLERP interpolation |
| Input Manager | `src/input/InputManager.h` | SDL2 keyboard/mouse event handling |
| Demo Bootstrap | `src/demo/Bootstrap.cpp` | Window, GL, asset setup, runtime boot, cleanup |
| Demo Runtime | `src/demo/RuntimeSystems.cpp` | Input orchestration, wave/session updates, title updates |
| Demo Rendering | `src/demo/Rendering.cpp` | World, particle, and HUD rendering passes |
| Demo Session Helpers | `src/demo/RuntimeSession.h` | Pure runtime helpers used by demo systems and tests |
| TPS Subsystem | `src/game/tps/` | Third-person controller, combat, AI, level data contracts, JSON loading, and level management |
| FPS Subsystem | `src/game/fps/FPSController.h` | First-person shooter mechanics |
| Particle System | `src/renderer/` | Particle emission, physics, rendering |
| Post-Processing | `src/renderer/` | Screen-space effects (bloom, blur, tone mapping) |

## 5. Desired Functionality

### Core Features

| # | Feature | Status | Description |
|---|---------|--------|-------------|
| F1 | Quaternion Math Library | ✅ | Full quaternion arithmetic, SLERP, conjugate, normalization |
| F2 | SLERP Camera Rotation | ✅ | Smooth interpolated camera movement using SLERP |
| F3 | SDL2 + OpenGL Rendering | ✅ | Modern OpenGL 3.3 context with SDL2 windowing |
| F4 | Blinn-Phong Lighting + Fog | ✅ | Per-pixel lighting, specular highlights, distance fog |
| F5 | FPS Camera Mode | ✅ | First-person camera with mouse look and WASD movement |
| F6 | TPS Game Mode | 🔄 | Third-person controller with combat mechanics, AI, level progression |
| F7 | Particle System | ✅ | Particle emission, lifetime management, physics simulation |
| F8 | Post-Processing Pipeline | ✅ | Screen-space effects: bloom, blur, color grading |
| F9 | NLERP vs. SLERP Visualization | ✅ | Comparison tool showing interpolation differences |
| F10 | Health / Readiness Surface | ✅ | Header-only `/alive` and `/ready` status helpers for launchers and embedding hosts |
| F11 | Core Observability Counters | ✅ | Header-only health/readiness counters with Prometheus-compatible text export |
| F12 | Scroll-wheel value safety | ✅ | The engine ships no editable value widgets; mouse-wheel input is limited to gameplay zoom and TPS weapon selection, with source-contract coverage guarding future UI drift |
| F13 | Runtime Configuration Overrides | ✅ | Engine defaults for windowing, rendering, camera, gameplay, and physics can be loaded from `.env` files or process environment variables through `ConfigManager` |

### API / Interface Contract

**Main Game Loop:**

```cpp
int main() {
    Engine engine("QuatEngine", 1280, 720);

    // Create scene
    GameObject player = engine.CreateGameObject();
    player.GetComponent<Transform>().SetRotation(
        Quaternion::FromEuler(0, 45, 0)  // Gimbal-lock free
    );

    // Game mode selection
    FPSController fps(player);
    // or
    TPSController tps(player);

    // Render loop
    while (engine.IsRunning()) {
        engine.Update();
        engine.Render();
    }
    return 0;
}
```

**Quaternion API:**

```cpp
Quaternion q1 = Quaternion::FromEuler(pitch, yaw, roll);
Quaternion q2 = Quaternion(axis, angle);
Quaternion interpolated = Quaternion::SLERP(q1, q2, t);  // t ∈ [0, 1]
Vec3 rotated = q1.Rotate(vec);
```

**Camera SLERP:**

```cpp
camera.RotateTo(targetQuaternion, duration_seconds);  // Smooth interpolation
```

**Health / Readiness API:**

QuatEngine does not run a network service by itself. Embedding hosts can expose
these stable status records as their `/alive` and `/ready` endpoints without
adding a web framework to the engine:

```cpp
auto alive = qe::core::alive_status();
auto ready = qe::core::ready_status({
    .configuration_loaded = true,
    .assets_available = true,
    .renderer_available = true,
});

std::string payload = qe::core::to_json(ready);
```

**Metrics API:**

```cpp
auto metrics = qe::core::Metrics::snapshot();
std::string body = qe::core::to_prometheus(metrics);
```

## 6. Data & Configuration

### Input Data

| Input | Format | Source | Schema |
|-------|--------|--------|--------|
| Level Configuration | JSON | `assets/levels/` | Level geometry, spawn points, AI pathing |
| Shader Source | GLSL 3.30 | `shaders/` | Vertex/fragment shaders compiled at runtime |
| Mesh Data | Binary (OBJ/custom) | `assets/models/` | Geometry, normals, UVs (Phase 4) |
| Input Bindings | JSON | `config/input.json` | Keyboard/mouse to action mappings |

### Output Data

| Output | Format | Destination | Description |
|--------|--------|-------------|-------------|
| Rendered Frame | Framebuffer | Screen | Real-time 3D scene via OpenGL |
| Screenshot | PNG | `screenshots/` | User-captured game viewport |
| Debug Logs | TXT | `logs/` | Frame time, memory, shader compilation |
| Metrics Snapshot | Prometheus text | Host-exposed endpoint or diagnostics sink | Health/readiness counter totals |

### Configuration

Configuration is managed via:
- **CMake variables**: Engine features, SDL2 version, OpenGL profile
- **JSON config files**: Input bindings, graphics settings (resolution, FOV, lighting)
- **Environment variables**: Debug logging level, asset paths, and `QE_*` runtime overrides for window size/title, OpenGL version, MSAA, timing, clear color, fog, camera defaults, projectile settings, kill score, gamepad look speed, and gravity
- **Dotenv files**: Optional `.env` files can be parsed before environment overrides; `.env.example` documents supported keys and valid value ranges
- **Shader compilation flags**: Optimization level, extensions enabled

## 7. Testing Specification

### Testing Strategy

Three-tier testing with unit tests for math (vectors, quaternions), integration tests for renderer systems, and heavy integration tests for full game loops. Labels distinguish quick unit tests (run in every CI) from slow integration/render tests (run selectively). Coverage tracked via gcov/gcovr. Demo runtime helpers are kept in native ctest coverage so `main.cpp` can stay a thin composition root. Python pytest coverage includes architecture/contract checks plus Hypothesis-backed properties that compile a temporary C++ probe against the real math headers.
Runtime configuration has dedicated native coverage in `tests/test_config.cpp` for defaults, dotenv-style stream parsing, validation failures, and parse errors.

### Test Organization

| Category | Location | Framework | Markers |
|----------|----------|-----------|---------|
| Unit | `tests/unit/` | ctest | `unit` |
| Integration | `tests/integration/` | ctest | `integration` |
| Render | `tests/render/` | ctest | `render` |
| Property | `tests/test_math_properties.py` | pytest + Hypothesis | generated math invariants |
| Benchmark | `.benchmarks/` | ctest | deterministic benchmark probes |

### Coverage Requirements

| Scope | Minimum | Current | Enforced By |
|-------|---------|---------|-------------|
| Math library | 90% | 95%+ | CI (`gcov` report, blocking at merge) |
| Renderer core | 75% | 80%+ | CI |
| Game subsystems | 60% | 70%+ | CI |

### Required Test Scenarios

- [ ] Quaternion multiplication produces correct rotated vectors
- [ ] SLERP interpolation smoothly transitions between two quaternions
- [ ] NLERP visualization correctly compares against SLERP
- [ ] Camera SLERP maintains constant angular velocity
- [ ] Blinn-Phong shader computes lighting correctly with multiple light sources
- [ ] Particle system respects emission rates and lifetime constraints
- [ ] TPS controller responds to input and updates position correctly
- [ ] FPS controller implements proper mouse look with no gimbal lock
- [ ] Post-processing effects apply without framebuffer corruption
- [ ] 26 non-render test files execute via ctest with 100% pass rate on C++17 compiler
- [x] Deterministic math benchmark probe builds and runs in CI without machine-specific timing thresholds
- [x] Patrol behavior wraps negative-time progression consistently for both position and facing rotation

## 8. Quality Standards

### Code Quality Tools

| Tool | Version | Purpose | Blocking? |
|------|---------|---------|-----------|
| clang-tidy | Latest | Static analysis and C++ best practices | Yes |
| clang-format | Latest | Code formatting | Yes |
| valgrind | Latest | Memory leak detection | Selective (integration tests) |
| gcov/gcovr | Latest | Code coverage analysis | Yes |

### Design Principles

- **TDD**: Enforced for math library; render tests use integration testing
- **Design by Contract (DbC)**: Yes — preconditions on vector/quat operations (e.g., unit quaternions)
- **DRY**: Yes — shader utilities, math operations, TargetBehavior patrol progression helpers, `math::PI` constant, and `PowerUpManager::get_effect_value` helper are centralized
- **Orthogonality**: Yes — math, rendering, and game logic are decoupled and independently testable
- **Demo boundary**: `src/main.cpp` stays a composition root; bootstrap, runtime, and render behavior live under `src/demo/`
- **Complexity documentation**: Core math, parser, procedural mesh, and gameplay helper APIs document Big-O behavior at their public boundaries so callers can reason about fixed-cost helpers versus input-size-dependent loops.

### CI/CD Pipeline

| Workflow | Trigger | Purpose | Blocking? |
|----------|---------|---------|-----------|
| `ci.yml` | Push/PR | Ubuntu build, CMake, unit tests, coverage (gcov) | Yes |
| `heavy-integration-tests.yml` | Push/PR | Full render tests, game loop validation | Yes |
| All tests on C++17 | Push/PR | Validate C++17 standard compliance | Yes |

PR workflows prefer the local self-hosted runner fleet when available and fall
back to GitHub-hosted Linux when it is not. Self-hosted Linux jobs no longer
assume passwordless `sudo` for dependency setup.

## 9. Dependencies

### Runtime Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| SDL2 | 2.30.10 | Window management, event handling, input |
| OpenGL | 3.3+ | GPU rendering API |
| C++ Standard Library | C++17 | STL containers, algorithms |

### Development Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| CMake | 3.20+ | Build system |
| ctest | 3.20+ | C++ testing framework |
| clang-format | Latest | Code style enforcement |
| clang-tidy | Latest | Static analysis |
| gcov | Latest | Coverage reporting |
| gcovr | Latest | Coverage HTML generation |

### Fleet Dependencies

| Repo | Relationship | Description |
|------|-------------|-------------|
| None | — | QuatEngine has no dependencies on other fleet repositories |

## 10. Deployment & Operations

### How to Run

```bash
# Prerequisites
- C++17 compiler (GCC, Clang, MSVC)
- CMake 3.20+
- Linux, macOS, or Windows

# Installation (SDL2 fetched automatically via FetchContent)
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j4

# Run engine executable
./QuatEngine

# Running tests
ctest                              # All tests
ctest -L unit                      # Unit tests only (fast)
ctest -L integration               # Integration tests
ctest -L render                    # Render tests

# Generate coverage report
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
cmake --build .
ctest
gcovr --print-summary --html coverage/
```

### Build Artifacts

| Artifact | Format | Destination |
|----------|--------|-------------|
| Executable | Binary | `build/QuatEngine` |
| Test executable | Binary | `build/tests/QuatEngine_tests` |
| Coverage reports | HTML | `build/coverage/` |
| Shader artifacts | SPIR-V (optional) | `build/shaders/` |

## 11. Roadmap & Open Issues

### Current Phase

**Phase 1:** ✅ Complete — Math library (vectors, quaternions, matrices) fully implemented and tested.

**Phase 2:** ✅ Complete — SDL2 + OpenGL rendering, camera system with SLERP, Blinn-Phong lighting, post-processing.

**Phase 3:** 🔄 In Progress — FPS/TPS game modes. FPS controller and basic TPS mechanics drafted; AI and level progression in development.

**Phase 4:** ❌ Not Started — Mesh loading from OBJ/FBX (requires asset pipeline).

**Phase 5:** ❌ Not Started — Advanced game mechanics (physics, inventory, dialogue).

**Phase 6:** ❌ Exploratory — Optional migration to Unreal Engine (experimental, not committed).

### Planned Work

| Priority | Item | Issue/PR | Target Date |
|----------|------|----------|-------------|
| P0 | Complete TPS AI behavior trees | TBD | 2026-04-30 |
| P1 | Implement TPS level progression and spawning | TBD | 2026-05-15 |
| P2 | Add rigid body physics integration | TBD | 2026-06-01 |
| P3 | Implement mesh loading (Phase 4) | TBD | 2026-06-30 |
| P4 | Evaluate Unreal Engine integration (Phase 6, exploratory) | TBD | Q3 2026 |

### Known Limitations

- Mesh loading not implemented — hardcoded geometry only (Phase 4 pending)
- No physics engine — collisions are bounding-box approximations
- TPS game mode incomplete — AI and level progression in progress
- No networked multiplayer
- Shader pipeline is forward-rendering only (no deferred shading)
- Post-processing effects limited to screen-space techniques
- No asset hot-reloading during development

## 12. Change Log

| Date | Version | Changes |
|------|---------|---------|
| 2026-09-10 | #1611 | Add maintainable Mermaid C4 architecture map contract and CI validation. |
| 2026-03-31 | 1.0.2 | Added self-hosted runner fallback documentation and made CI dependency setup tolerant of runners without passwordless sudo |
| 2026-04-28 | 1.0.16 | Observability: added `src/core/Metrics.h` with health/readiness counters and Prometheus-compatible text serialization, wired readiness probes to counters, and added focused native metrics tests (closes #135) |
| 2026-05-22 | 1.0.18 | Runtime configuration: added `src/core/ConfigManager.h` and runtime-backed `EngineConfig.h` accessors for dotenv and environment overrides, documented supported `QE_*` keys in `.env.example`, and added `tests/test_config.cpp` coverage for defaults, parsing, and validation |
| 2026-04-28 | 1.0.15 | Documentation: added Big-O complexity annotations to public math, renderer parser/generator, and gameplay helper APIs so input-size-dependent work is explicit (closes #147) |
| 2026-04-28 | 1.0.14 | Benchmarking: added deterministic `.benchmarks` math probe for quaternion SLERP/rotate and Vec3 normalize/cross workloads with finite checksum validation, wired behind `QE_BUILD_BENCHMARKS`, and added CI benchmark execution (closes #146) |
| 2026-04-28 | 1.0.15 | Reliability: replaced `Scoring.h` debug-only public input assertions with release-active `std::invalid_argument` validation for negative scores/bonuses and non-finite timing or multiplier inputs; added focused negative-path coverage in `tests/test_game_extended.cpp` (closes #129) |
| 2026-04-06 | 1.0.4 | Refactored `TargetBehavior` to share common factory initialization and patrol progression helpers across both position and rotation paths, and added regression coverage for negative-time patrol wrapping plus shared factory defaults |
| 2026-04-10 | 1.0.5 | DRY: extracted `math::Constants.h` (PI, TWO_PI) shared by Scene.h, TPSScene.h, ParticleSystem.h, removing inline duplicates; added `PowerUpManager::get_effect_value` helper eliminating repeated iteration pattern across four multiplier getters (closes #93) |
| 2026-04-11 | 1.0.7 | Refactor: decomposed 7 oversized functions (78-148 LOC) across RuntimeSystems.cpp, Rendering.cpp, tps_main.cpp, TPSScene.h, TPSCombat.h, and OBJLoader.h into focused helpers; public signatures unchanged (closes #92) |
| 2026-04-11 | 1.0.6 | TDD: replaced placeholder `tests/test_architecture_dbc.py` with real layered-architecture invariants (math ← core ← renderer/input ← game ← demo), DbC decorator coverage for `src/contracts.py`, and Weapons.h public-contract pins; added nine negative-path tests to `tests/test_weapons.cpp` covering empty-ammo fire, reload-while-reloading, reload-at-full, reload-on-infinite-ammo, invalid switch index, can_fire during reload/cooldown, and switch_weapon cancelling an in-progress reload (closes #94) |
| 2026-04-14 | 1.0.8 | DbC: replaced all runtime `assert` statements in `TPSWeapons.h` and `WaveSystem.h` with explicit `if (!cond) throw std::invalid_argument/std::logic_error(...)` guards that remain active in NDEBUG/release builds; added 11 negative-path tests to `test_tps_weapons.cpp` and 9 to `test_wave.cpp` verifying every guard throws on invalid input (closes #104) |
| 2026-04-28 | 1.0.12 | Observability: added `src/core/Readiness.h` with embeddable `/alive` and `/ready` status helpers plus JSON serialization; added `tests/test_readiness.cpp` coverage for alive, ready, not-ready, and payload behavior (closes #157) |
| 2026-04-28 | 1.0.13 | Testing: added Hypothesis-backed property tests for quaternion/vector invariants through a temporary C++ math probe and wired Hypothesis into CI Python tests (closes #144) |
| 2026-04-14 | 1.0.9 | OBJLoader error handling: extracted `OBJParser.h` (pure-C++, no GL/SDL) from `OBJLoader.h`, replacing silent `std::stoi` throws and zero-fill on bad streams with explicit per-line error counting and face skipping; added `parse_content`/`parse_stream` public API; added `tests/test_objloader.cpp` with 18 tests covering triangle/quad parse, v//vn/vt/vt+vn formats, missing normals, malformed tokens, out-of-range indices, zero index, incomplete vertex records, face-before-vertices, fewer-than-3-vertex faces, and good-face-after-bad-face; total ctest count raised to 24 (closes #105) |
| 2026-04-22 | 1.0.11 | Refactor: split TPS level data contracts and JSON loading out of `LevelSystem.h` into `LevelTypes.h` and `LevelLoader.h`; added `test_tps_level_modules.cpp` to pin standalone include behavior and level loading without the state machine (addresses #119) |
| 2026-03-28 | 1.0.0 | Initial specification |
| 2026-03-30 | 1.0.1 | A-N Assessment remediation: add .env to .gitignore, add MIT LICENSE, add DbC assertions to Combat.h/Scoring.h, add Camera::set_aspect/set_smoothing interface methods (LoD), update main.cpp to use new Camera interface |

---

<!--
  SPEC MAINTENANCE RULES:

  1. WHEN TO UPDATE: Any PR that adds, removes, or changes functionality
     described in this spec MUST include a corresponding spec update.

  2. WHO UPDATES: The PR author (human or agent) is responsible.

  3. CI ENFORCEMENT: The spec-check workflow will flag PRs where source
     files changed but SPEC.md did not. This is a blocking check.

  4. REVIEW: Spec changes should be reviewed with the same rigor as code.

  5. VERSION: Bump the Spec Version field when making substantive changes.
     Use semver: major (structure change), minor (new features), patch (corrections).
-->
