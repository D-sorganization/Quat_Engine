# Development Roadmap

## Phase 1: Math Foundation ✅

### Delivered

- `Vec3` — 3D vector with full operator suite, dot/cross product, normalization, lerp
- `Quaternion` — Hamilton product, SLERP/NLERP, axis-angle, Euler, 2-vector rotation
- `Mat4` — 4x4 column-major matrix, perspective/look-at/TRS, quaternion-to-matrix
- `Transform` — Component wrapping position + quaternion + scale with interpolation
- Test suite covering 40+ assertions including SLERP edge cases

### SLERP Implementation Notes

The SLERP implementation follows the standard formula:

```
slerp(a, b, t) = a * sin((1-t)θ) / sin(θ) + b * sin(tθ) / sin(θ)
```

With important engineering refinements:

1. **Shortest path correction**: When dot(a,b) < 0, negate b to avoid "long way around"
2. **Numerical stability**: Falls back to NLERP when sin(θ) ≈ 0 (θ > 0.9995 threshold)
3. **Constant angular velocity**: Verified in tests — evenly-spaced t values produce evenly-spaced angles

---

## Phase 2: Rendering Pipeline (Next)

### Objectives

- SDL2 window creation and event loop
- OpenGL 3.3+ core profile context
- Basic shader pipeline (vertex + fragment)
- Wireframe cube rendering (first mesh)
- FPS counter and frame timing

### Dependencies to Add

- **SDL2** — Window management, input, OpenGL context
- **GLAD** or **GLEW** — OpenGL function loading
- Consider **vcpkg** for dependency management on Windows

### Key Decisions

- OpenGL 3.3 core profile (wide compatibility, modern API)
- Column-major matrices (already matches our Mat4 layout)
- Right-handed coordinate system (OpenGL default)

---

## Phase 3: Camera & Controls

### Objectives

- FPS camera: mouse look (quaternion pitch/yaw, no gimbal lock)
- TPS camera: orbit around target with SLERP smoothing
- Keyboard movement (WASD + space/shift for vertical)
- Camera spring/smooth follow

### Technical Notes

- Camera orientation stored as Quaternion — never decompose to Euler angles
- Mouse delta → axis-angle quaternion → compose with current rotation
- View matrix derived from Transform::to_matrix() inverse

---

## Phase 4: Mesh & Lighting

### Objectives

- OBJ file loader (vertices, normals, UVs)
- Vertex Array Objects (VAO), Vertex Buffer Objects (VBO)
- Phong lighting (ambient + diffuse + specular)
- Multiple light sources
- Basic texture mapping

---

## Phase 5: Game Mechanics

### From Force Field to 3D

- Port force field mechanics (projectiles, health, weapons)
- 3D collision detection (sphere-sphere, ray-AABB)
- Bot AI in 3D navigation
- HUD overlay rendering

---

## Phase 6: Advanced / Unreal Migration

### Options

- Continue with custom engine for full educational value
- Port game logic to Unreal Engine 5 (C++)
- The math library (Vec3, Quaternion, Mat4) maps directly to UE5's FVector, FQuat, FMatrix
