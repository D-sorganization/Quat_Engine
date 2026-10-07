"""Fork pull-request code never reaches the self-hosted fleet (RM#1989).

Applies the vendored ``scripts/fork_pr_runner_guard.py`` to this repository's
real workflows, so a new or edited job that can run fork PR code on
``d-sorg-fleet`` fails the suite before it can merge.
"""

from __future__ import annotations

from pathlib import Path

from scripts import fork_pr_runner_guard as guard

WORKFLOWS_DIR = Path(__file__).resolve().parents[1] / ".github" / "workflows"


def test_workflows_directory_exists() -> None:
    """Guard against a vacuous pass from a mistyped workflows path."""
    assert any(WORKFLOWS_DIR.glob("*.y*ml"))


def test_no_job_runs_fork_pr_code_on_self_hosted() -> None:
    """Every fleet-capable job is fork-guarded or routes forks to hosted runners."""
    assert guard.find_violations(WORKFLOWS_DIR) == []
