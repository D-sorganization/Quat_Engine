# Monolithic Files Audit — QuatEngine

**Issue:** #114 — Split oversized modules by responsibility
**Wave:** 34 (fleet old-issue triage)
**Date:** 2026-04-19

## Purpose

Identify modules whose line count suggests multiple responsibilities have
accreted into a single translation unit or header, and propose responsibility-
oriented splits. QuatEngine is predominantly C++, so the audit focuses on
`.h`, `.hpp`, and `.cpp` files; Python is included for completeness but no
files exceed the warning threshold.

## Method

```
find . -type f \( -name "*.py" -o -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
  -not -path "./node_modules/*" -not -path "./.git/*" \
  -exec wc -l {} \; | sort -rn | head -20
```

## Tier definitions

- **Tier A (critical, >600 LOC):** split before further feature work; add
  characterization tests first to pin current behavior.
- **Tier B (warning, 400-599 LOC):** schedule a split in the next 1-2 sprints;
  characterization tests recommended but not blocking.
- **Tier C (watch, 300-399 LOC):** monitor; split only if a new responsibility
  is about to be added.

## Top 15 files

| Rank | LOC | File | Tier | Notes |
|-----:|----:|------|------|-------|
| 1 | 711 | `src/renderer/Mesh.h` | A | Mesh data + loaders + GPU upload likely fused |
| 2 | 670 | `src/game/tps/LevelSystem.h` | A | Level state, streaming, spawn rules |
| 3 | 614 | `tests/test_math_core.cpp` | A | Test file; split by math subsystem |
| 4 | 603 | `src/tps_main.cpp` | A | Entry point + glue; extract subsystem init |
| 5 | 503 | `tests/test_weapons.cpp` | B | Test file; split per weapon family |
| 6 | 491 | `src/game/tps/PlayerController.h` | B | Input + movement + camera cohabiting |
| 7 | 490 | `tests/test_math.cpp` | B | Overlaps with test_math_core |
| 8 | 464 | `tests/test_game_extended.cpp` | B | Broad grab-bag test file |
| 9 | 456 | `src/game/tps/MeleeSystem.h` | B | Combat + animation + hitboxes |
| 10 | 448 | `tests/test_math_extended.cpp` | B | Third math test file — consolidate and re-shard |
| 11 | 446 | `src/game/tps/TPSWeapons.h` | B | Weapon data + firing logic + ammo |
| 12 | 443 | `tests/test_particles.cpp` | B | OK; monitor |
| 13 | 414 | `src/game/tps/TPSGameState.h` | B | State machine + persistence + HUD hooks |
| 14 | 411 | `src/game/tps/MutantTypes.h` | B | Enemy definitions + AI stubs |
| 15 | 404 | `src/game/tps/TPSCombat.h` | B | Damage resolution + effects + feedback |

## Proposed splits

### Tier A

**`src/renderer/Mesh.h` (711)**
- `Mesh.h` — POD vertex/index structs and `Mesh` class surface.
- `MeshLoader.h/.cpp` — OBJ/GLTF loading, file IO.
- `MeshGpuUpload.h/.cpp` — VAO/VBO/IBO creation and draw helpers.
- **Characterization tests first:** snapshot-load a known OBJ and record vertex
  counts, bounding box, material indices; snapshot GPU resource IDs via a
  mockable gl layer.

**`src/game/tps/LevelSystem.h` (670)**
- `LevelDefinition.h` — static level descriptors.
- `LevelRuntime.h/.cpp` — live state, tick, transitions.
- `LevelStreaming.h/.cpp` — async load/unload, residency.
- `LevelSpawnRules.h/.cpp` — spawn tables, difficulty curves.
- **Characterization tests first:** deterministic seed replay of
  level-tick outputs over N frames.

**`tests/test_math_core.cpp` (614)**
- Split by math subsystem: `test_math_vector.cpp`, `test_math_quaternion.cpp`,
  `test_math_matrix.cpp`, `test_math_transform.cpp`. Consolidate with
  `test_math.cpp` and `test_math_extended.cpp` to eliminate overlap.

**`src/tps_main.cpp` (603)**
- Keep `main()` slim; extract:
  - `AppBootstrap.cpp` — window, GL, audio init.
  - `SubsystemWiring.cpp` — DI/registration of renderer, game, input.
  - `MainLoop.cpp` — frame timing, shutdown.
- **Characterization tests first:** golden-log of subsystem init order.

### Tier B (condensed)

- **`PlayerController.h`** -> `PlayerInput`, `PlayerMovement`, `PlayerCameraRig`.
- **`MeleeSystem.h`** -> `MeleeAnimation`, `MeleeHitbox`, `MeleeResolver`.
- **`TPSWeapons.h`** -> `WeaponData` (pure data), `WeaponFiring`, `AmmoPool`.
- **`TPSGameState.h`** -> `GameStateMachine`, `GameStatePersistence`, `HudBinding`.
- **`MutantTypes.h`** -> `MutantArchetype` (data), `MutantAIStubs` (behavior seed).
- **`TPSCombat.h`** -> `DamageResolver`, `CombatEffects`, `CombatFeedback`.
- Test grab-bags (`test_weapons.cpp`, `test_game_extended.cpp`): re-shard by
  subsystem matching the production split.

## Characterization-first policy

Before splitting any Tier A C++ module, the splitter MUST:

1. Add a characterization test that exercises the module through its public
   API and records observable outputs (return values, emitted events, GPU
   resource bindings via a seam) to a golden file.
2. Verify the characterization test is deterministic across 3 runs.
3. Land the characterization test in a standalone PR before the split PR.
4. Keep the characterization test green across the split — it is the contract
   that the refactor preserved behavior.

This guards against silent behavioral drift when responsibilities are teased
apart, which is the primary risk for legacy game-engine code where coupling
is often implicit via call order.

## Python

No Python file exceeds 300 LOC in this repo. No action required for Python.

## Next steps

- Open follow-up issues per Tier A entry, each blocked on its characterization-
  test PR.
- Re-run this audit quarterly; track the top-15 LOC number as a rough
  health metric (target: max file < 500 LOC within two quarters).
