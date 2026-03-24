# QuatEngine

A modern C++ 3D game engine built from the ground up, with quaternion-based rotation and SLERP interpolation at its core. A completely separate project from the Python raycaster games.

## Architecture

```
QuatEngine/
├── CMakeLists.txt          # Build system (CMake 4.2+, auto-downloads SDL2)
├── src/
│   ├── math/               # Vec3, Quaternion (SLERP), Mat4
│   ├── core/               # Transform component
│   └── renderer/           # OpenGL rendering (GLLoader, Shader, Mesh, Camera)
├── shaders/                # GLSL 3.30 shaders (vertex + fragment)
├── tests/                  # Math + engine tests
├── assets/                 # Models, textures (Phase 4)
└── docs/development/       # Design documents
```

## Current: Phase 2 — SDL2 + OpenGL + Quaternion Camera

### What You See

- 🎮 **Real-time 3D window** (1280×720, OpenGL 3.3 core)
- 🔄 **Quaternion-based FPS camera** — no gimbal lock, SLERP-smoothed rotation
- 🧊 **Blinn-Phong lit cube** spinning with quaternion rotation
- 📊 **SLERP vs NLERP comparison** — two side-by-side cubes showing the difference
- 🌫️ **Distance fog** fading to dark background
- 📐 **Ground grid** for spatial reference
- 📈 **FPS counter** in title bar

### Controls

| Key            | Action                    |
| -------------- | ------------------------- |
| WASD           | Move                      |
| Mouse          | Look around (quaternion!) |
| Space / LShift | Move up / down            |
| 1 / 2          | SLERP smoothing OFF / ON  |
| F              | Toggle wireframe          |
| Escape         | Quit                      |

## Building

### Prerequisites

- Visual Studio 2026 Community (with "Desktop development with C++" workload)
- CMake 4.2+ (ships with Visual Studio)
- Internet connection for first build (CMake downloads SDL2 automatically)

### Command Line (Developer PowerShell)

```powershell
cd C:\Users\diete\Repositories\QuatEngine
cmake -B build -G "Visual Studio 18 2026"
cmake --build build --config Release
.\build\Release\qe_demo.exe
```

### Visual Studio IDE

1. Open Visual Studio → "Open a local folder" → select `QuatEngine/`
2. Visual Studio auto-detects `CMakeLists.txt`
3. Select `qe_demo.exe` as startup project
4. Build and Run (F5)

## Testing

QuatEngine uses native C++ tests registered with `ctest`.

```powershell
cmake -S . -B build -DQE_BUILD_DEMO=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

To generate gcov-compatible coverage on GCC/Clang toolchains:

```powershell
cmake -S . -B build -DQE_BUILD_DEMO=OFF -DQE_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
gcovr --root . --filter src --exclude tests --print-summary
```

Test labels are available for more targeted runs:

- `unit` for math/core/gameplay invariants
- `integration` for higher-level gameplay flows
- `render` for renderer-adjacent checks that need the renderer target available

## Roadmap

| Phase | Focus                                        | Status      |
| ----- | -------------------------------------------- | ----------- |
| 1     | Math library (Vec3, Quaternion, SLERP, Mat4) | ✅ Complete |
| 2     | SDL2 + OpenGL + Quaternion Camera            | ✅ Complete |
| 3     | FPS/TPS camera modes + input system          | 🔲 Planned  |
| 4     | Mesh loading (OBJ/glTF) + textures           | 🔲 Planned  |
| 5     | 3D game mechanics (FPS/TPS shooter)          | 🔲 Planned  |
| 6     | Unreal Engine migration (optional)           | 🔲 Future   |

## Key Concepts

### Quaternions & SLERP

All rotation in QuatEngine uses quaternions (`qe::math::Quaternion`):

- **No gimbal lock** — quaternions represent rotations without singularities
- **SLERP** — smooth, constant-angular-velocity rotation blending
- **NLERP** — fast approximation (visible comparison in demo)
- **Shortest path** — automatically takes the shorter rotation arc
- **Camera** — mouse input → axis-angle → quaternion composition (never Euler angles)

### Namespace: `qe::`

- `qe::math` — Vec3, Quaternion, Mat4
- `qe::core` — Transform
- `qe::renderer` — Camera, Shader, Mesh, GLLoader

## License

Private — D-sorganization
