# QuatEngine Development Guide

## Testing First

QuatEngine changes should follow `RED -> GREEN -> REFACTOR`.

- `RED`: add or extend a native `ctest` executable before changing engine behavior.
- `GREEN`: implement the smallest code change that makes the new test pass.
- `REFACTOR`: clean up names, duplication, or structure while keeping the suite green.

For math and engine work, tests should be deterministic and cheap enough to run on every pull request.

## Required Test Shapes

- Quaternion and vector math changes need invariant-style tests, not only example snapshots.
- Gameplay logic changes need at least one realistic flow test in addition to unit coverage.
- Renderer-adjacent changes should separate pure configuration/state tests from context-dependent rendering behavior.
- Bug fixes should add a regression test that fails before the fix and passes after it.

## Local Commands

### Native test suite

```bash
cmake -S . -B build -DQE_BUILD_DEMO=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

### Coverage-enabled run

```bash
cmake -S . -B build -DQE_BUILD_DEMO=OFF -DQE_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
gcovr --root . --filter src --exclude tests --print-summary
```

### Focused suites

```bash
ctest --test-dir build --output-on-failure -L unit
ctest --test-dir build --output-on-failure -L integration
```

## Review Expectations

- New behavior should not merge without matching tests.
- When updating fixtures or expected values, explain what behavior changed and why the old expectation was no longer correct.
- If a renderer test cannot be executed headlessly, cover the pure logic around it and document the remaining manual step.
