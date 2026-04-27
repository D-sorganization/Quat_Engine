# Contributing to ForceFieldEngine

Thank you for your interest in contributing to the ForceFieldEngine! We welcome all contributions, from bug reports and documentation updates to new features.

## Developer Certificate of Origin (DCO)

We enforce the Developer Certificate of Origin (DCO) on all commits. This ensures that contributors have the right to submit the code they are contributing.

By contributing to this repository, you agree to the [DCO](https://developercertificate.org/).
Please sign-off your commits using the `-s` flag in Git:

```bash
git commit -s -m "feat: your feature"
```

## Local Development

We follow the standard organization workflow. Please refer to `SPEC.md` for the technical specifications and architectural invariants of this repository.

1. **Prerequisites**:
   - Install required dependencies as specified in the lockfile.
   - Run `pre-commit install` to set up Git hooks.

2. **Quality Gates**:
   - `ruff check .` (Linting)
   - `ruff check --fix .`
   - `ruff format .`
   - `black .`
   - `mypy .`

Ensure all these checks pass locally before opening a Pull Request.

## Pull Request Guidelines

1. Prefix your branches appropriately (e.g., `feat/`, `fix/`, `chore/`, `docs/`).
2. Use Conventional Commits (`feat:`, `fix:`, `docs:`, etc.).
3. Reference the issue number you are addressing (e.g., `Closes #123`).
4. Ensure CI/CD tests and quality gates pass.
