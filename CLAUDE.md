# Agent Instructions

## Commands
* Build: `cmake -S . -B build && cmake --build build`
* Test: `ctest --test-dir build`

## Code Style
* Use modern C++ practices
* Include docstrings

## Hook bypass policy

**Never use `git commit --no-verify` or `git push --no-verify` unless the hook itself is broken** (tooling not installed, hook script crashes). It is *not* an acceptable workaround for a hook that flags real issues.

### When a hook fails on something you didn't touch

The hook is scoped to *your diff*. If `fleet-fast-guardrails` or any other guardrail reports a violation in a file you didn't change, that's a regression — file an issue against `Repository_Management`. Bypassing locally doesn't help: the same checks run in CI's `quality-gate` and will block the PR.

### When the hook is legitimately broken

Open an issue in `Repository_Management`. If you must bypass once to land an urgent fix, include the hook error in the commit body and link the tracking issue. **Do not normalize `--no-verify` as a workaround.**

### Enforcement

Branch protection requires the CI `quality-gate` check on every PR. That check runs the same lint, format, type, and security gates as the hooks. `--no-verify` only delays feedback — it cannot land code that would have failed the hook.

For the canonical hook contract, see [`Repository_Management/docs/FLEET_HOOK_STANDARDS.md`](https://github.com/D-sorganization/Repository_Management/blob/main/docs/FLEET_HOOK_STANDARDS.md).
<!-- ci-trigger 1779934295 -->
