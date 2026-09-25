# Project Charter

> Drafted 2026-09-25 by the fleet charter sweep (Gemini) from README, git history, and open issues/PRs.
> The project-steward role keeps this current; owners should correct feature statuses.

## End Goal

QuatEngine provides an educational yet functional C++17 3D game engine built from first principles, demonstrating gimbal-lock-free rotation using pure quaternion mathematics and SLERP/NLERP camera interpolation on top of SDL2 and OpenGL 3.3. "Done" looks like a completely self-contained, cross-platform engine and demonstration suite featuring dual FPS and TPS game modes, Blinn-Phong lighting, quaternion-driven particles, post-processing shaders, embeddable health/metrics observability, and comprehensive deterministic test coverage without external service runtime dependencies.

## Non-Goals

- Not a production AAA game engine; educational clarity and architectural transparency take priority over commercial game scale.
- Not a general-purpose rendering framework or replacement for existing commercial engines such as Unreal, Unity, or Godot.
- No support for mobile, console, or WebGL build targets.
- Not a networked game service or HTTP daemon; the engine is strictly an embeddable local library and desktop runtime.
- No general-purpose asset import pipeline or runtime editor beyond native procedural primitives and simple OBJ text parsing.

## Features

| ID | Feature | Status | Tracking | Notes |
| --- | --- | --- | --- | --- |
| F1 | Quaternion and Vector Math Library | shipped | #226 | Header-only Vec3, Quaternion, and Mat4 with SLERP and NLERP |
| F2 | Dual-Mode FPS and TPS Camera | shipped | - | SLERP-smoothed camera with first-person and orbit viewing modes |
| F3 | OpenGL 3.3 Rendering Pipeline | shipped | - | Minimal GL function loader, GLSL shaders, and VAO/VBO/EBO wrappers |
| F4 | Blinn-Phong Lighting and Distance Fog | shipped | - | Up to 8 dynamic point lights with distance fog and rim lighting |
| F5 | Quaternion-Driven Particle System | shipped | - | Particle emitter updating orientations via axis-angle composition |
| F6 | Full-Screen Post-Processing Pipeline | shipped | - | FBO screen-space effects for scanlines, aberration, vignette, and grain |
| F7 | Unified Input Management System | shipped | #206 | Keyboard, mouse look, and SDL2 gamepad action mapping |
| F8 | Wave-Based FPS Gameplay Mode | shipped | #200 | Wave progression, 5 weapons with spread, scoring combos, and 7 AI types |
| F9 | Third-Person Shooter Gameplay Mode | shipped | - | Melee combat combos, character classes, and level progression |
| F10 | Mesh Primitives and OBJ Loading | shipped | #105 | Procedural geometry generators and pure C++ OBJ parser with loader |
| F11 | Runtime Configuration System | shipped | #223 | Engine defaults with dotenv loading and environment variable overrides |
| F12 | Embeddable Health and Metrics Surface | shipped | #201 | In-process alive and ready records with Prometheus counter metrics |
| F13 | Benchmark and Math Property Testing | shipped | #195 | Deterministic C++ benchmarks and Hypothesis property-based testing |

## Links

- Status (generated): [`STATUS.md`](STATUS.md)
- Steward playbook: Repository_Management `docs/fleet-project-steward.md`
