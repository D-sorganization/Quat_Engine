# A-N Assessment - QuatEngine - 2026-04-19

Run time: 2026-04-19T08:02:24.4668879Z UTC
Sync status: pull-blocked
Sync notes: ff-only pull failed: fatal: couldn't find remote ref codex/an-assessment-2026-04-14

Overall grade: C (76/100)

## Coverage Notes
- Reviewed tracked first-party files from git ls-files, excluding cache, build, vendor, virtualenv, temp, and generated output directories.
- Reviewed 148 tracked files, including 86 code files, 28 test files, 3 CI files, 2 config/build files, and 33 docs/onboarding files.
- This is a read-only static assessment of committed files. TDD history and confirmed Law of Demeter semantics require commit-history review and deeper call-graph analysis; this report distinguishes those limits from confirmed file evidence.

## Category Grades
### A. Architecture and Boundaries: B (82/100)
Assesses source organization and boundary clarity from tracked first-party layout.
- Evidence: `148 tracked first-party files`
- Evidence: `59 files under source-like directories`

### B. Build and Dependency Management: C (72/100)
Assesses committed build, dependency, and tool configuration.
- Evidence: `CMakeLists.txt`
- Evidence: `Dockerfile.heavy_test`

### C. Configuration and Environment Hygiene: C (78/100)
Checks whether runtime and developer configuration is explicit.
- Evidence: `CMakeLists.txt`
- Evidence: `Dockerfile.heavy_test`

### D. Contracts, Types, and Domain Modeling: B (82/100)
Design by Contract evidence includes validation, assertions, typed models, explicit raised errors, and invariants.
- Evidence: `src/contracts.py`
- Evidence: `src/game/WaveSystem.h`
- Evidence: `src/game/tps/CharacterClass.h`
- Evidence: `src/game/tps/JumpSystem.h`
- Evidence: `src/game/tps/LevelSystem.h`
- Evidence: `src/game/tps/LockOnSystem.h`
- Evidence: `src/game/tps/MeleeSystem.h`
- Evidence: `src/game/tps/MutantTypes.h`
- Evidence: `src/game/tps/TPSScene.h`
- Evidence: `src/game/tps/TPSWeapons.h`

### E. Reliability and Error Handling: C (76/100)
Reliability is graded from test presence plus explicit validation/error-handling signals.
- Evidence: `docs/assessments/Assessment_C_Test_Coverage.md`
- Evidence: `tests/__init__.py`
- Evidence: `tests/test_architecture_dbc.py`
- Evidence: `tests/test_behavior.cpp`
- Evidence: `tests/test_framework.h`
- Evidence: `src/contracts.py`
- Evidence: `src/game/WaveSystem.h`
- Evidence: `src/game/tps/CharacterClass.h`
- Evidence: `src/game/tps/JumpSystem.h`
- Evidence: `src/game/tps/LevelSystem.h`

### F. Function, Module Size, and SRP: F (55/100)
Evaluates function size, script/module size, and single responsibility using static size signals.
- Evidence: `src/game/tps/LevelSystem.h (671 lines)`
- Evidence: `src/renderer/Mesh.h (712 lines)`
- Evidence: `src/tps_main.cpp (604 lines)`
- Evidence: `tests/test_math_core.cpp (615 lines)`
- Evidence: `tests/test_weapons.cpp (504 lines)`

### G. Testing and TDD Posture: B (82/100)
TDD history cannot be confirmed statically; grade reflects committed automated test posture.
- Evidence: `docs/assessments/Assessment_C_Test_Coverage.md`
- Evidence: `tests/__init__.py`
- Evidence: `tests/test_architecture_dbc.py`
- Evidence: `tests/test_behavior.cpp`
- Evidence: `tests/test_framework.h`
- Evidence: `tests/test_game.cpp`
- Evidence: `tests/test_game_extended.cpp`
- Evidence: `tests/test_math.cpp`
- Evidence: `tests/test_math_core.cpp`
- Evidence: `tests/test_math_extended.cpp`
- Evidence: `tests/test_particles.cpp`
- Evidence: `tests/test_postprocess.cpp`

### H. CI/CD and Automation: C (78/100)
Checks for tracked CI/CD workflow files.
- Evidence: `.github/workflows/ci.yml`
- Evidence: `.github/workflows/heavy-integration-tests.yml`
- Evidence: `.github/workflows/spec-check.yml`

### I. Security and Secret Hygiene: B (82/100)
Secret scan is regex-based; findings require manual confirmation.
- Evidence: No direct tracked-file evidence found for this category.

### J. Documentation and Onboarding: B (82/100)
Checks docs, README, onboarding, and release documents.
- Evidence: `AGENTS.md`
- Evidence: `Dockerfile.heavy_test`
- Evidence: `LICENSE`
- Evidence: `README.md`
- Evidence: `SPEC.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-03-30.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-03-31.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-04-02.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-04-04.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-04-09.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-04-10.md`
- Evidence: `docs/assessments/A-N_Assessment_2026-04-11.md`

### K. Maintainability, DRY, and Duplication: B (80/100)
DRY is assessed through duplicate filename clusters and TODO/FIXME density as static heuristics.
- Evidence: `src/input/InputManager.h`

### L. API Surface and Law of Demeter: D (68/100)
Law of Demeter is approximated with deep member-chain hints; confirmed violations require semantic review.
- Evidence: `src/game/tps/TPSGameState.h`
- Evidence: `src/game/tps/TPSScene.h`
- Evidence: `src/tps_main.cpp`
- Evidence: `tests/test_tps_ai.cpp`
- Evidence: `tests/test_tps_gamestate.cpp`

### M. Observability and Operability: C (74/100)
Checks for logging, metrics, monitoring, and operational artifacts.
- Evidence: `docs/assessments/Assessment_L_Logging.md`
- Evidence: `src/core/Logger.h`

### N. Governance, Licensing, and Release Hygiene: C (74/100)
Checks ownership, release, contribution, security, and license metadata.
- Evidence: `LICENSE`
- Evidence: `docs/assessments/Assessment_F_Security.md`

## Explicit Engineering Practice Review
- TDD: Automated tests are present, but red-green-refactor history is not confirmable from static files.
- DRY: No repeated filename clusters met the static threshold.
- Design by Contract: Validation/contract signals were found in tracked code.
- Law of Demeter: Deep member-chain hints were found and should be semantically reviewed.
- Function size and SRP: Large modules or coarse long-definition signals were found.

## Key Risks
- Large modules/scripts reduce maintainability and SRP clarity.

## Prioritized Remediation Recommendations
1. Split the largest modules by responsibility and add characterization tests before refactoring.

## Actionable Issue Candidates
### Split oversized modules by responsibility
- Severity: medium
- Problem: Oversized files found: src/game/tps/LevelSystem.h (671 lines); src/renderer/Mesh.h (712 lines); src/tps_main.cpp (604 lines); tests/test_math_core.cpp (615 lines); tests/test_weapons.cpp (504 lines)
- Evidence: Category F lists files over 500 lines or coarse long-definition signals.
- Impact: Large modules obscure ownership, complicate review, and weaken SRP.
- Proposed fix: Add characterization tests, then split cohesive responsibilities into smaller modules.
- Acceptance criteria: Largest files are reduced or justified; extracted modules have focused tests.
- Expectations: SRP, function size, module size, maintainability
