# Contributing to QuatEngine

Thank you for your interest in contributing to QuatEngine! This document provides guidelines for participating in this project.

## Code of Conduct

Be respectful, constructive, and inclusive in all interactions.

## How to Contribute

### Reporting Issues

- Use GitHub Issues to report bugs or request features.
- Provide a clear description, steps to reproduce, and expected vs. actual behavior.
- Include your environment (OS, compiler version, GPU).

### Submitting Changes

1. **Fork** the repository and create a feature branch:
   ```bash
   git checkout -b feat/your-feature-name
   ```
2. **Write** clear, focused commits following [Conventional Commits](https://www.conventionalcommits.org/):
   ```
   feat: add dual-quaternion skinning
   fix: correct SLERP boundary condition
   docs: update build instructions for macOS
   test: add unit test for quaternion normalization
   ```
3. **Build** and **test** locally before pushing:
   ```bash
   cmake -S . -B build && cmake --build build
   ctest --test-dir build
   ```
4. **Push** your branch and open a Pull Request.
5. Ensure CI passes and respond to review feedback promptly.

## Development Setup

### Prerequisites

- CMake >= 3.16
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- SDL2 development libraries
- OpenGL 3.3 capable GPU

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run Tests

```bash
ctest --test-dir build
```

## Code Style

- Use modern C++17 practices.
- Include docstrings for all public APIs.
- Follow the existing `.clang-format` style:
  ```bash
  find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
  ```
- Keep functions focused and under 50 lines where possible.
- Prefer `const` correctness and RAII.

## Testing

- Add unit tests for new algorithms in `tests/`.
- Add integration tests for new rendering features.
- Ensure all tests pass before submitting a PR.

## Documentation

- Update `README.md` if user-facing behavior changes.
- Update `CLAUDE.md` if agent-relevant build/test commands change.
- Update `CHANGELOG.md` under the `## [Unreleased]` section.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

If you have questions, open a Discussion or reach out via GitHub Issues.