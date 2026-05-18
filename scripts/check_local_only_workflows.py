#!/usr/bin/env python3
"""Fail when GitHub Actions workflows can route to hosted runners."""

from __future__ import annotations

import logging
from pathlib import Path

logger = logging.getLogger(__name__)

WORKFLOW_DIR = Path(".github") / "workflows"
BANNED = (
    "ubuntu-latest",
    "windows-latest",
    "macos-latest",
    "force_cloud",
    "mode=cloud",
    "Routing to GitHub-hosted",
    "using GitHub-hosted",
    "runner=ubuntu-latest",
    "runner=windows-latest",
    "runner=macos-latest",
)

# Files allowlisted from the hosted-runner scan. The tripwire workflow
# intentionally runs on a hosted runner; everything else must stay local.
LEGACY_HOSTED_RUNNER_ALLOWLIST = {
    ".github/workflows/local-only-runner-guard.yml",
}


def main() -> int:
    logger.info("Starting GitHub Actions workflow validation")
    failures: list[str] = []
    if not WORKFLOW_DIR.exists():
        logger.debug("Workflow directory does not exist, skipping validation")
        return 0

    for path in sorted(WORKFLOW_DIR.rglob("*")):
        if path.suffix not in {".yml", ".yaml"}:
            continue

        if path.as_posix() in LEGACY_HOSTED_RUNNER_ALLOWLIST:
            continue
        logger.debug("Validating workflow file", extra={"path": str(path)})
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            logger.debug(
                "Workflow file required UTF-8-sig encoding", extra={"path": str(path)}
            )
            text = path.read_text(encoding="utf-8-sig")
        for line_number, line in enumerate(text.splitlines(), start=1):
            for token in BANNED:
                if token in line:
                    failure_msg = (
                        f"{path}:{line_number}: banned hosted-runner token {token!r}"
                    )
                    logger.warning(
                        "Found banned hosted-runner token",
                        extra={
                            "file": str(path),
                            "line_number": line_number,
                            "token": token,
                        },
                    )
                    failures.append(failure_msg)

    if failures:
        logger.error(
            "Workflow validation failed - hosted-runner tokens detected",
            extra={"failure_count": len(failures)},
        )
        print(
            "GitHub-hosted runner routing is forbidden. "
            "Use local self-hosted runners only."
        )
        print("\n".join(failures))
        return 1

    logger.info("Workflow validation passed - local-only runners confirmed")
    print("Workflow runner routing is local-only.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
