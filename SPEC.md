# SPEC.md — Repository Specification Document

<!--
  TEMPLATE VERSION: 1.0.0
  LAST UPDATED: 2026-03-28

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
| **Current Version** | N/A |
| **Spec Version** | 1.0.0 |
| **Last Spec Update** | 2026-03-28 |

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
- No built-in mesh loading or asset pipeline (Phase 4 dependent)

## 4. Architecture Overview

### System Context

QuatEngine is a standalone, self-contained engine with no external fleet dependencies. It depends on SDL2 (auto-fetched via CMake FetchContent) and OpenGL. Optionally, the project roadmap includes potential migration to Unreal Engine as Phase 6, but this is exploratory and not a dependency.

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
│   │   ├── Rng.h               # Shared xorshift32 PRNG
│   │   └── Logger.h            # Structured logging (DEBUG/INFO/WARN/ERROR)
│   ├── renderer/                # Graphics pipeline
│   │   ├── GLLoader.h
│   │   ├── Shader.h
│   │   ├── Mesh.h
│   │   └── Camera.h
│   ├── input/                   # SDL2 input handling
│   │   └── InputManager.h
│   ├── game/
│   │   ├── tps/                 # Third-person shooter subsystem
│   │   │   ├── TPSController.h
│   │   │   ├── Combat.h
│   │   │   ├── AI.h
│   │   │   └── Levels.h
│   │   └── fps/                 # First-person shooter subsystem
│   │       └── FPSController.h
│   └── main.cpp
├── shaders/                      # GLSL 3.30 shader collection (10 files)
│   ├── basic.vert
│   ├── basic.frag
│   ├── blinn_phong.frag
│   ├── post_process.frag
│   └── ...
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
| PRNG | `src/core/Rng.h` | Deterministic xorshift32 random number generator |
| Logger | `src/core/Logger.h` | Structured logging with compile-time and runtime level control |
| GL Renderer | `src/renderer/GLLoader.h` | OpenGL initialization and context management |
| Shader System | `src/renderer/Shader.h` | GLSL compilation, linking, and uniform management |
| Mesh | `src/renderer/Mesh.h` | Geometry data (vertices, indices, normals, UVs) |
| Camera | `src/renderer/Camera.h` | View matrix, projection, SLERP interpolation |
| Input Manager | `src/input/InputManager.h` | SDL2 keyboard/mouse event handling |
| TPS Subsystem | `src/game/tps/` | Third-person controller, combat, AI, level management |
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

### Configuration

Configuration is managed via:
- **CMake variables**: Engine features, SDL2 version, OpenGL profile
- **JSON config files**: Input bindings, graphics settings (resolution, FOV, lighting)
- **Environment variables**: Debug logging level, asset paths
- **Shader compilation flags**: Optimization level, extensions enabled

## 7. Testing Specification

### Testing Strategy

Three-tier testing with unit tests for math (vectors, quaternions), integration tests for renderer systems, and heavy integration tests for full game loops. Labels distinguish quick unit tests (run in every CI) from slow integration/render tests (run selectively). Coverage tracked via gcov/gcovr.

### Test Organization

| Category | Location | Framework | Markers |
|----------|----------|-----------|---------|
| Unit | `tests/unit/` | ctest | `unit` |
| Integration | `tests/integration/` | ctest | `integration` |
| Render | `tests/render/` | ctest | `render` |

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
- [ ] 22 test files execute via ctest with 100% pass rate on C++17 compiler

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
- **DRY**: Yes — shader utilities and math operations centralized
- **Orthogonality**: Yes — math, rendering, and game logic are decoupled and independently testable

### CI/CD Pipeline

| Workflow | Trigger | Purpose | Blocking? |
|----------|---------|---------|-----------|
| `ci.yml` | Push/PR | Ubuntu build, CMake, unit tests, coverage (gcov) | Yes |
| `heavy-integration-tests.yml` | Push/PR | Full render tests, game loop validation | Yes |
| All tests on C++17 | Push/PR | Validate C++17 standard compliance | Yes |

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
| 2026-03-28 | 1.0.0 | Initial specification |

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
