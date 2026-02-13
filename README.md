# ForceFieldEngine

A modern C++ 3D game engine built from the ground up, evolving from a Python raycaster to a full mesh-based renderer with quaternion rotation and SLERP interpolation.

## Architecture

```
ForceFieldEngine/
├── CMakeLists.txt          # Build system
├── src/
│   ├── math/               # Vec3, Quaternion (SLERP), Mat4
│   ├── core/               # Transform, Camera (Phase 2)
│   └── renderer/           # OpenGL mesh rendering (Phase 2)
├── tests/                  # Comprehensive math + engine tests
├── assets/                 # Models, textures (Phase 3)
├── shaders/                # GLSL shaders (Phase 2)
└── docs/development/       # Design documents
```

## Phase 1: Math Foundation (Current)

- ✅ `Vec3` — 3D vector with dot, cross, normalize, lerp
- ✅ `Quaternion` — Hamilton product, SLERP/NLERP, axis-angle, Euler, rotation
- ✅ `Mat4` — 4x4 transform matrix, perspective, look-at, TRS
- ✅ `Transform` — Position + Quaternion rotation + Scale, with interpolation
- ✅ 40+ unit tests covering all edge cases

## Building

### Prerequisites

- Visual Studio 2022 Community (with "Desktop development with C++" workload)
- CMake 3.20+ (ships with Visual Studio)

### Command Line (Developer PowerShell)

```powershell
# From the ForceFieldEngine directory:
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Visual Studio IDE

1. Open Visual Studio → "Open a local folder" → select `ForceFieldEngine/`
2. Visual Studio auto-detects `CMakeLists.txt`
3. Select `test_math.exe` as startup project
4. Build and Run (F5)

## Roadmap

| Phase | Focus                                          | Status      |
| ----- | ---------------------------------------------- | ----------- |
| 1     | Math library (Vec3, Quaternion, SLERP, Mat4)   | ✅ Complete |
| 2     | SDL2 window + OpenGL context + basic rendering | 🔲 Planned  |
| 3     | FPS/TPS camera with quaternion rotation        | 🔲 Planned  |
| 4     | Mesh loading (OBJ/glTF) + lighting             | 🔲 Planned  |
| 5     | Force Field game mechanics in 3D               | 🔲 Planned  |
| 6     | Unreal Engine migration (optional)             | 🔲 Future   |

## Key Concepts

### Quaternions & SLERP

This engine uses quaternions for all rotation instead of Euler angles:

- **No gimbal lock** — quaternions represent rotations without singularities
- **SLERP** (Spherical Linear Interpolation) — smooth, constant-velocity rotation blending
- **NLERP** — fast approximation for small angular differences
- **Shortest path** — automatically takes the shorter rotation arc

## License

Private — D-sorganization
