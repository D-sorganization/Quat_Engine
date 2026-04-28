# Dependency License Audit

This document records the open-source licenses of all external dependencies
used by QuatEngine.

## Methodology

- Licenses identified via upstream repository `LICENSE` files.
- SPDX identifiers used where applicable.
- No dependencies are vendored; all are fetched at build time or provided by
  the host system.

## Direct Dependencies

| Dependency | Version | License (SPDX) | Source |
|------------|---------|----------------|--------|
| SDL2 | 2.30.10 | Zlib | https://github.com/libsdl-org/SDL |
| OpenGL | 3.3 (system) | Various vendor licenses* | System driver / GPU vendor |

\* OpenGL is provided by the operating-system graphics driver and is not
distributed with QuatEngine. End-users are bound by their GPU vendor’s
license terms (e.g., NVIDIA, AMD, Intel).

## Build / Test Dependencies

| Dependency | Version | License (SPDX) | Source |
|------------|---------|----------------|--------|
| CMake | 3.20+ | BSD-3-Clause | https://gitlab.kitware.com/cmake/cmake |
| gcovr | 7.2+ | BSD-3-Clause | https://github.com/gcovr/gcovr |
| Python (bandit) | any | PSF-2.0 | https://github.com/PyCQA/bandit |

## Deviation / Risk Notes

- **SDL2 (Zlib)**: Permissive license compatible with MIT. No copyleft
  obligations.
- **OpenGL (proprietary / system)**: Not distributed; no redistribution risk.
- **CMake / gcovr (BSD-3-Clause)**: Build-time only; no runtime impact.

## Audit Date

2026-04-27