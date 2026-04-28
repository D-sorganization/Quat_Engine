# Contributing to QuatEngine

Thanks for contributing to QuatEngine. Keep changes focused, preserve the
engine's C++17 and quaternion-first design, and follow the local testing
workflow before opening or updating a pull request.

## Before You Start

- Read [`SPEC.md`](SPEC.md) for the repository's current architecture,
  functionality, and documentation contract.
- If you change behavior, add or update the matching native test first when
  practical.
- Keep fixes narrow. One issue should usually map to one pull request.

## Reporting Issues

- Use GitHub Issues for bugs, regressions, or feature requests.
- Include a clear description, reproduction steps, and expected versus actual
  behavior.
- When relevant, include your platform, compiler, and renderer context.

## Developer Certificate of Origin

QuatEngine enforces the Developer Certificate of Origin (DCO) on all commits.
Sign every commit with `-s`:

```bash
git commit -s -m "fix: short summary"
```

By contributing, you agree to the [DCO](https://developercertificate.org/).

## Local Development

### Prerequisites

- CMake 3.20 or newer
- A C++17 compiler
- An internet connection on the first demo build so CMake can fetch SDL2

### Configure and Build

Build the native test suite without the demo targets when you only need fast
headless validation:

```bash
cmake -S . -B build -DQE_BUILD_DEMO=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Build the demo applications when your change touches renderer, SDL2, or runtime
composition code:

```bash
cmake -S . -B build
cmake --build build
```

### Focused Validation

Run a smaller test slice when you are iterating locally, then run the relevant
full validation before you ask for review:

```bash
ctest --test-dir build --output-on-failure -L unit
ctest --test-dir build --output-on-failure -L integration
ctest --test-dir build --output-on-failure -L tps
```

Enable coverage only on GCC or Clang builds:

```bash
cmake -S . -B build -DQE_BUILD_DEMO=OFF -DQE_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
gcovr --root . --filter src --exclude tests --print-summary
```

## Pull Requests

- Use a clear branch name such as `fix/...`, `feat/...`, or `docs/...`.
- Use Conventional Commit prefixes such as `fix:`, `feat:`, `docs:`, or `ci:`.
- Link the issue in the PR body with `Closes #<number>` when the PR should
  close it.
- Update [`SPEC.md`](SPEC.md) when the PR changes documented functionality,
  architecture, or operating expectations.
- Do not merge until required GitHub checks pass.

## Testing Expectations

- Math and engine changes need deterministic native tests.
- Gameplay changes should include a realistic flow or regression test, not only
  a happy-path example.
- Renderer-adjacent changes should separate pure logic coverage from anything
  that depends on a graphics context.
- Documentation-only changes should still leave the repository in a clean diff
  state.
