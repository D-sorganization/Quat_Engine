#!/usr/bin/env python3
"""Runner Dashboard v4.0 — Fleet monitoring with heavy tests, daily reports, and mobile UI."""

import json
import os
import re
import subprocess
import glob
from datetime import datetime
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse, parse_qs

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

PORT = int(os.environ.get("DASHBOARD_PORT", 8321))
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN", "")
REPORTS_DIR = os.environ.get(
    "REPORTS_DIR",
    os.path.expanduser("~/progress-tracking/reports"),
)
HEAVY_TEST_REPOS = json.loads(
    os.environ.get(
        "HEAVY_TEST_REPOS",
        json.dumps(
            [
                {
                    "owner": "d-sorganization",
                    "repo": "UpstreamDrift",
                    "workflow": "heavy_tests.yml",
                    "display_name": "UpstreamDrift",
                }
            ]
        ),
    )
)

FRONTEND_DIR = Path(__file__).resolve().parent.parent / "frontend"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _json_response(handler, data, status=200):
    body = json.dumps(data).encode()
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.end_headers()
    handler.wfile.write(body)


def _error(handler, msg, status=400):
    _json_response(handler, {"error": msg}, status)


def _github_api(method, url, body=None):
    """Call the GitHub REST API using curl."""
    cmd = [
        "curl",
        "-s",
        "-X", method,
        "-H", "Accept: application/vnd.github+json",
        "-H", f"Authorization: Bearer {GITHUB_TOKEN}",
    ]
    if body is not None:
        cmd += ["-H", "Content-Type: application/json", "-d", json.dumps(body)]
    cmd.append(f"https://api.github.com{url}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if result.returncode != 0:
        return None
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return None


def _parse_report_metrics(text):
    """Extract key metrics from a markdown progress report."""
    metrics = {}
    # Look for patterns like "Score: 8/10" or "Assessment: 85%"
    score_match = re.search(r"(?:score|assessment)[:\s]+(\d+(?:\.\d+)?)[/\s]*(\d+)?", text, re.I)
    if score_match:
        metrics["score"] = score_match.group(1)
        if score_match.group(2):
            metrics["score_max"] = score_match.group(2)

    # Look for "Tests: 42 passed" or "Tests passed: 42"
    tests_match = re.search(r"tests?\s*(?:passed)?[:\s]+(\d+)\s*passed", text, re.I)
    if tests_match:
        metrics["tests_passed"] = int(tests_match.group(1))

    # Look for "Coverage: 85%"
    cov_match = re.search(r"coverage[:\s]+(\d+(?:\.\d+)?)%", text, re.I)
    if cov_match:
        metrics["coverage"] = float(cov_match.group(1))

    # Look for "Issues: 3 open"
    issues_match = re.search(r"issues?[:\s]+(\d+)\s*open", text, re.I)
    if issues_match:
        metrics["open_issues"] = int(issues_match.group(1))

    return metrics


# ---------------------------------------------------------------------------
# Runners API  (existing v3 endpoints kept)
# ---------------------------------------------------------------------------


def _get_runners():
    """List self-hosted runners across the configured org."""
    data = _github_api("GET", "/orgs/d-sorganization/actions/runners")
    if data and "runners" in data:
        return data["runners"]
    return []


def _get_org_repos():
    """List repos for the org with PR / issue counts and CI badge info."""
    repos = _github_api("GET", "/orgs/d-sorganization/repos?per_page=100&sort=updated")
    if not repos:
        return []
    results = []
    for r in repos:
        results.append(
            {
                "name": r.get("name"),
                "full_name": r.get("full_name"),
                "html_url": r.get("html_url"),
                "open_issues_count": r.get("open_issues_count", 0),
                "language": r.get("language"),
                "updated_at": r.get("updated_at"),
                "default_branch": r.get("default_branch", "main"),
            }
        )
    return results


# ---------------------------------------------------------------------------
# Reports API  (v4)
# ---------------------------------------------------------------------------


def _list_reports():
    """Return a list of available daily report dates (newest first)."""
    reports_path = Path(REPORTS_DIR)
    if not reports_path.is_dir():
        return []
    files = sorted(reports_path.glob("*.md"), reverse=True)
    results = []
    for f in files:
        date_match = re.search(r"(\d{4}-\d{2}-\d{2})", f.stem)
        date_str = date_match.group(1) if date_match else f.stem
        results.append({"date": date_str, "filename": f.name})
    return results


def _read_report(date_str):
    """Read a specific daily report by date string."""
    reports_path = Path(REPORTS_DIR)
    # Try exact filename first, then glob
    candidates = list(reports_path.glob(f"*{date_str}*.md"))
    if not candidates:
        return None, {}
    report_file = candidates[0]
    text = report_file.read_text(encoding="utf-8", errors="replace")
    metrics = _parse_report_metrics(text)
    return text, metrics


def _get_chart_path(date_str):
    """Return path to assessment chart image for a given date, if it exists."""
    reports_path = Path(REPORTS_DIR)
    for ext in ("png", "jpg", "svg"):
        candidates = list(reports_path.glob(f"*{date_str}*chart*.{ext}"))
        if candidates:
            return candidates[0]
    return None


# ---------------------------------------------------------------------------
# Heavy Tests API  (v4)
# ---------------------------------------------------------------------------


def _list_heavy_test_repos():
    """Return the configured heavy-test repos."""
    return HEAVY_TEST_REPOS


def _dispatch_heavy_test(owner, repo, workflow, ref="main", python_version="3.12"):
    """Trigger a GitHub Actions workflow_dispatch for heavy tests."""
    body = {"ref": ref, "inputs": {"python_version": python_version}}
    result = _github_api(
        "POST",
        f"/repos/{owner}/{repo}/actions/workflows/{workflow}/dispatches",
        body,
    )
    # workflow_dispatch returns 204 on success (empty body)
    return result is None or result == ""


def _run_heavy_test_docker(owner, repo, python_version="3.12", branch="main"):
    """Run heavy tests locally via docker-compose. Returns the process output."""
    compose_file = Path.cwd() / "docker-compose.heavy-test.yml"
    if not compose_file.exists():
        # Fall back to Dockerfile.heavy_test in repo root
        dockerfile = Path(__file__).resolve().parent.parent.parent / "Dockerfile.heavy_test"
        if not dockerfile.exists():
            return {"error": "No docker-compose.heavy-test.yml or Dockerfile.heavy_test found"}

        cmd = [
            "docker",
            "build",
            "-f", str(dockerfile),
            "--build-arg", f"PYTHON_VERSION={python_version}",
            "--build-arg", f"BRANCH={branch}",
            "-t", f"heavy-test-{repo}",
            ".",
        ]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            return {
                "stdout": result.stdout[-5000:],  # last 5k chars
                "stderr": result.stderr[-2000:],
                "returncode": result.returncode,
            }
        except subprocess.TimeoutExpired:
            return {"error": "Docker build timed out after 600s"}

    env = os.environ.copy()
    env["PYTHON_VERSION"] = python_version
    env["TEST_BRANCH"] = branch
    cmd = ["docker-compose", "-f", str(compose_file), "run", "--rm", "heavy-tests"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=600, env=env)
        return {
            "stdout": result.stdout[-5000:],
            "stderr": result.stderr[-2000:],
            "returncode": result.returncode,
        }
    except subprocess.TimeoutExpired:
        return {"error": "Docker run timed out after 600s"}


def _recent_heavy_test_runs(owner, repo, workflow):
    """Fetch recent workflow runs for the heavy test workflow."""
    data = _github_api(
        "GET",
        f"/repos/{owner}/{repo}/actions/workflows/{workflow}/runs?per_page=10",
    )
    if data and "workflow_runs" in data:
        return [
            {
                "id": r["id"],
                "status": r["status"],
                "conclusion": r.get("conclusion"),
                "created_at": r["created_at"],
                "html_url": r["html_url"],
                "head_branch": r.get("head_branch"),
            }
            for r in data["workflow_runs"]
        ]
    return []


# ---------------------------------------------------------------------------
# HTTP Handler
# ---------------------------------------------------------------------------


class DashboardHandler(SimpleHTTPRequestHandler):
    """Serve the frontend and handle API routes."""

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path.rstrip("/")
        qs = parse_qs(parsed.query)

        # --- API routes ---
        if path == "/api/runners":
            _json_response(self, _get_runners())
        elif path == "/api/repos":
            _json_response(self, _get_org_repos())
        elif path == "/api/reports":
            date = qs.get("date", [None])[0]
            if date:
                text, metrics = _read_report(date)
                if text is None:
                    _error(self, "Report not found", 404)
                else:
                    _json_response(self, {"date": date, "content": text, "metrics": metrics})
            else:
                _json_response(self, _list_reports())
        elif re.match(r"/api/reports/[\d-]+/chart", path):
            date = path.split("/")[3]
            chart = _get_chart_path(date)
            if chart and chart.exists():
                ext = chart.suffix.lstrip(".")
                mime = {"png": "image/png", "jpg": "image/jpeg", "svg": "image/svg+xml"}.get(ext, "application/octet-stream")
                data = chart.read_bytes()
                self.send_response(200)
                self.send_header("Content-Type", mime)
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)
            else:
                _error(self, "Chart not found", 404)
        elif path == "/api/heavy-tests/repos":
            _json_response(self, _list_heavy_test_repos())
        elif path == "/api/heavy-tests/runs":
            owner = qs.get("owner", [""])[0]
            repo = qs.get("repo", [""])[0]
            workflow = qs.get("workflow", [""])[0]
            if not all([owner, repo, workflow]):
                _error(self, "owner, repo, workflow required")
            else:
                _json_response(self, _recent_heavy_test_runs(owner, repo, workflow))
        elif path == "" or path == "/":
            # Serve the frontend
            self.path = "/index.html"
            self.directory = str(FRONTEND_DIR)
            super().do_GET()
        else:
            # Try serving static files from frontend dir
            self.directory = str(FRONTEND_DIR)
            super().do_GET()

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path.rstrip("/")
        length = int(self.headers.get("Content-Length", 0))
        body = json.loads(self.rfile.read(length)) if length > 0 else {}

        if path == "/api/heavy-tests/dispatch":
            owner = body.get("owner", "")
            repo = body.get("repo", "")
            workflow = body.get("workflow", "")
            ref = body.get("ref", "main")
            python_version = body.get("python_version", "3.12")
            if not all([owner, repo, workflow]):
                _error(self, "owner, repo, workflow required")
                return
            ok = _dispatch_heavy_test(owner, repo, workflow, ref, python_version)
            _json_response(self, {"dispatched": True, "ref": ref, "python_version": python_version})

        elif path == "/api/heavy-tests/docker":
            owner = body.get("owner", "")
            repo = body.get("repo", "")
            python_version = body.get("python_version", "3.12")
            branch = body.get("branch", "main")
            result = _run_heavy_test_docker(owner, repo, python_version, branch)
            _json_response(self, result)

        else:
            _error(self, "Not found", 404)

    def translate_path(self, path):
        """Override to serve from FRONTEND_DIR."""
        if hasattr(self, "directory"):
            root = self.directory
        else:
            root = str(FRONTEND_DIR)
        path = urlparse(path).path
        path = path.split("?", 1)[0]
        # Prevent directory traversal
        path = os.path.normpath(path)
        parts = path.split("/")
        parts = [p for p in parts if p and p != ".."]
        return os.path.join(root, *parts) if parts else root

    def log_message(self, format, *args):
        ts = datetime.now().strftime("%H:%M:%S")
        print(f"[{ts}] {args[0]}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    server = HTTPServer(("0.0.0.0", PORT), DashboardHandler)
    print(f"Runner Dashboard v4.0 listening on http://0.0.0.0:{PORT}")
    print(f"  Reports dir : {REPORTS_DIR}")
    print(f"  Heavy repos : {[r['display_name'] for r in HEAVY_TEST_REPOS]}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
        server.server_close()


if __name__ == "__main__":
    main()
