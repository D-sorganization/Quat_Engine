# QuatEngine

[![CI](https://github.com/D-sorganization/QuatEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/D-sorganization/QuatEngine/actions/workflows/ci.yml)

A modern C++17 3D game engine built from first principles, using **quaternion mathematics** for gimbal-lock-free rotation and **SLERP interpolation** for smooth camera movement. Combines educational clarity with a playable FPS/TPS shooter built on SDL2 and OpenGL 3.3.

## What It Does

- **Quaternion-only rotation** -- every rotation in the engine (camera, entities, particles) uses quaternions. No Euler-angle decomposition, no gimbal lock.
- **SLERP-smoothed camera** -- constant-angular-velocity interpolation for fluid first-person and third-person camera modes.
- **Blinn-Phong lighting** with distance fog, rim lighting, and up to 8 dynamic point lights.
- **Particle system** where each particle carries a quaternion orientation updated via axis-angle composition.
- **Post-processing pipeline** -- CRT scanlines, chromatic aberration, vignette, film grain.
- **Wave-based FPS shooter** with 5 weapons, combo scoring, power-ups, and 7 AI behavior types.
- **TPS game mode** with third-person orbit camera, melee combat, character classes, and level progression.

## Architecture

```
QuatEngine/
├── src/
│   ├── math/                    # Core math library (header-only)
│   │   ├── Vec3.h               #   3D vector: dot, cross, normalize, lerp
│   │   ├── Quaternion.h         #   Quaternion: SLERP, NLERP, axis-angle, Euler
│   │   └── Mat4.h               #   4x4 matrix: TRS, perspective, look-at
│   ├── core/                    # Engine foundation
│   │   ├── Transform.h          #   Position + quaternion rotation + scale
│   │   ├── Entity.h             #   Game object with health, bounds, state
│   │   ├── AABB.h               #   Axis-aligned bounding box
│   │   ├── Projectile.h         #   Projectile physics
│   │   ├── EngineConfig.h       #   Centralized configuration constants
│   │   └── Readiness.h          #   Embeddable /alive and /ready status surface
│   ├── renderer/                # OpenGL 3.3 rendering (requires SDL2)
│   │   ├── GLLoader.h           #   Minimal GL function pointer loader
│   │   ├── Shader.h             #   GLSL compilation and uniform management
│   │   ├── Mesh.h               #   VAO/VBO/EBO wrapper + primitive generators
│   │   ├── Camera.h             #   Dual-mode FPS/TPS camera with SLERP
│   │   ├── ParticleSystem.h     #   Quaternion-driven particle effects
│   │   ├── PostProcess.h        #   Full-screen post-processing pipeline
│   │   ├── HUD.h                #   Heads-up display (crosshair, bars, combos)
│   │   ├── Texture.h            #   Procedural texture generators
│   │   └── OBJLoader.h          #   Basic OBJ mesh loader
│   ├── input/                   # Input handling
│   │   ├── InputManager.h       #   Keyboard + mouse + gamepad abstraction
│   │   ├── InputAction.h        #   Action mapping
│   │   └── Gamepad.h            #   SDL2 gamepad wrapper
│   ├── game/                    # FPS game systems
│   │   ├── Combat.h             #   Hit detection, damage
│   │   ├── Weapons.h            #   5-weapon loadout with quaternion spread
│   │   ├── WaveSystem.h         #   Progressive wave difficulty
│   │   ├── TargetBehavior.h     #   7 AI movement patterns (orbit, figure-8, etc.)
│   │   ├── PowerUp.h            #   Collectible power-up system
│   │   ├── Scoring.h            #   Combo multiplier scoring
│   │   └── Scene.h              #   Static decoration management
│   └── game/tps/                # TPS game subsystem
│       ├── TPSGameState.h       #   TPS game loop and state machine
│       ├── PlayerController.h   #   Third-person player movement
│       ├── MutantAI.h           #   Enemy AI with behavior trees
│       ├── CharacterClass.h     #   Vanguard / Recon / Titan classes
│       ├── MeleeSystem.h        #   Melee combat with combos
│       ├── LevelSystem.h        #   Level progression and spawning
│       └── ...                  #   Animation, HUD, weapons, etc.
├── shaders/                     # GLSL 3.30 shaders
├── tests/                       # 23 test files (unit + integration)
├── CMakeLists.txt               # Build system
└── SPEC.md                      # Full specification document
```

### Key Classes

| Class | Namespace | Purpose |
|-------|-----------|---------|
| `Vec3` | `qe::math` | 3D vector with dot, cross, normalize, lerp |
| `Quaternion` | `qe::math` | Hamilton-convention quaternion with SLERP/NLERP |
| `Mat4` | `qe::math` | Column-major 4x4 matrix (OpenGL layout) |
| `Transform` | `qe::core` | Position + rotation (quaternion) + scale, cached matrix |
| `HealthStatus` | `qe::core` | Machine-readable `/alive` and `/ready` status records for hosts |
| `Camera` | `qe::renderer` | Dual FPS/TPS camera with SLERP smoothing |
| `Shader` | `qe::renderer` | GLSL compile/link and uniform setters |
| `Mesh` | `qe::renderer` | GPU geometry with primitive generators (cube, sphere, etc.) |
| `ParticleSystem` | `qe::renderer` | Quaternion-oriented particle emitter |
| `PostProcess` | `qe::renderer` | FBO-based screen-space effects |
| `InputManager` | `qe::input` | Unified keyboard/mouse/gamepad input |
| `WaveSystem` | `qe::game` | Wave progression with scaling difficulty |
| `WeaponManager` | `qe::game` | 5-weapon loadout with quaternion-based spread |

## Building

### Prerequisites

- **C++17 compiler** (GCC 8+, Clang 7+, MSVC 2019+)
- **CMake 3.20+**
- Internet connection on first build (CMake downloads SDL2 automatically via FetchContent)

### Linux / macOS

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
./qe_demo          # FPS shooter
./qe_tps_game      # TPS game
```

### Windows (Developer PowerShell)

```powershell
cmake -B build
cmake --build build --config Release
.\build\Release\qe_demo.exe
.\build\Release\qe_tps_game.exe
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `QE_BUILD_DEMO` | `ON` | Build the demo applications (requires OpenGL) |
| `QE_ENABLE_COVERAGE` | `OFF` | Enable gcov coverage flags (GCC/Clang only) |

### Tests Only (no GPU required)

```bash
cmake -B build -DQE_BUILD_DEMO=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Test labels: `unit`, `integration`, `render`, `math`, `gameplay`, `tps`

```bash
ctest --test-dir build -L unit        # Fast math/gameplay unit tests
ctest --test-dir build -L math        # Math library only
ctest --test-dir build -L tps         # TPS subsystem tests
```

### Coverage Report

```bash
cmake -B build -DQE_BUILD_DEMO=OFF -DQE_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
gcovr --root . --filter src --exclude tests --print-summary
```

## Controls

### FPS Mode

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look (quaternion rotation) |
| Left Click | Shoot |
| Space / LShift | Move up / down |
| 1-5 | Select weapon |
| Q / E | Previous / next weapon |
| R | Reload |
| Tab | Toggle FPS/TPS camera |
| F | Toggle wireframe |
| Escape | Quit |

### TPS Mode

| Key | Action |
|-----|--------|
| WASD | Move character |
| Mouse | Orbit camera |
| Scroll | Zoom in/out |
| Left Click | Attack |
| Tab | Toggle FPS/TPS camera |

Gamepad (Xbox layout) is fully supported in both modes.

## Configuration

Engine defaults (window size, camera speed, fog, lighting, etc.) are centralized in `src/core/EngineConfig.h`. Modify constants there to tune the engine without searching through application code.

## Health and Readiness

QuatEngine is an embeddable C++ engine, not a network service, so it does not start an HTTP server. Hosts and launchers can expose `qe::core::alive_status()` as `/alive` and `qe::core::ready_status(...)` as `/ready`, then serialize either status with `qe::core::to_json(...)`.

## Key Concepts

### Quaternions and SLERP

All rotation in QuatEngine uses `qe::math::Quaternion`:

- **No gimbal lock** -- quaternions represent rotations as points on the 4D unit hypersphere
- **SLERP** -- Spherical Linear Interpolation provides constant angular velocity
- **NLERP** -- faster approximation, used as fallback when quaternions are nearly identical
- **Shortest path** -- automatically negates one quaternion to take the shorter arc
- **Camera** -- mouse input becomes axis-angle, composed via quaternion multiplication

```cpp
// Create rotation from axis-angle
auto q = Quaternion::from_axis_angle(Vec3::up(), radians);

// Smooth interpolation between orientations
auto blended = Quaternion::slerp(current, target, t);

// Rotate a point
Vec3 rotated = q.rotate(point);
```

### Namespace Layout

- `qe::math` -- Vec3, Quaternion, Mat4
- `qe::core` -- Transform, Entity, AABB, EngineConfig
- `qe::renderer` -- Camera, Shader, Mesh, ParticleSystem, PostProcess, HUD
- `qe::input` -- InputManager, Gamepad
- `qe::game` -- Combat, Weapons, WaveSystem, Scoring, PowerUp
- `qe::game::tps` -- TPS-specific subsystems

## Roadmap

| Phase | Focus | Status |
|-------|-------|--------|
| 1 | Math library (Vec3, Quaternion, SLERP, Mat4) | Complete |
| 2 | SDL2 + OpenGL + Quaternion Camera | Complete |
| 3 | FPS/TPS game modes + input system | In Progress |
| 4 | Mesh loading (OBJ/glTF) + textures | Planned |
| 5 | 3D game mechanics (physics, inventory) | Planned |
| 6 | Unreal Engine migration (exploratory) | Future |

## License

MIT -- see [SPEC.md](SPEC.md) for full specification.
