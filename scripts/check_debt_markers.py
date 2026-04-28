#!/usr/bin/env python3
"""Require issue references for TODO/FIXME comments and lint suppressions."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

TRACKED_SUFFIXES = {
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".py",
    ".toml",
    ".yaml",
    ".yml",
}
TRACKED_FILENAMES = {
    ".clang-format",
    ".pre-commit-config.yaml",
    "CMakeLists.txt",
}
SKIP_PREFIXES = (
    "assessments/",
    "docs/assessments/",
)
SKIP_FILES = {
    "scripts/check_debt_markers.py",
}

MARKER_RE = re.compile(
    r"\b(TODO|FIXME|type:\s*ignore|type-ignore|noqa|NOLINT(?:NEXTLINE|BEGIN|END)?|"
    r"cppcheck-suppress|pragma:\s*no\s*cover)\b",
    re.IGNORECASE,
)
ISSUE_RE = re.compile(r"(?:^|\s)#\d+\b|github\.com/[^/\s]+/[^/\s]+/issues/\d+\b")


def tracked_files() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files"],
        check=True,
        capture_output=True,
        text=True,
    )
    paths: list[Path] = []
    for raw_path in result.stdout.splitlines():
        normalized = raw_path.replace("\\", "/")
        if normalized in SKIP_FILES or normalized.startswith(SKIP_PREFIXES):
            continue
        path = Path(normalized)
        if path.name in TRACKED_FILENAMES or path.suffix in TRACKED_SUFFIXES:
            paths.append(path)
    return paths


def main() -> int:
    failures: list[str] = []
    marker_count = 0

    for path in tracked_files():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            text = path.read_text(encoding="utf-8-sig")

        for line_number, line in enumerate(text.splitlines(), start=1):
            if not MARKER_RE.search(line):
                continue
            marker_count += 1
            if not ISSUE_RE.search(line):
                failures.append(
                    f"{path}:{line_number}: debt marker lacks issue reference"
                )

    if failures:
        print(
            "TODO/FIXME comments and suppression markers must include an issue "
            "reference such as #158 or a GitHub issue URL."
        )
        print("\n".join(failures))
        return 1

    print(f"Tracked debt markers all reference issues ({marker_count} found).")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(exc.returncode)
