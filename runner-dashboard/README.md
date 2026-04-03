# Runner Dashboard v4

A four-tab web dashboard for monitoring self-hosted GitHub Actions runners, organization repositories, heavy integration tests, and daily progress reports.

## Features

### Fleet Tab
- View all self-hosted runners with status (online/offline/busy)
- Stat cards for total, online, busy, and offline runners
- Collapsible runner table with labels and OS info

### Organization Tab
- Browse all repositories in the organization
- Open issue counts and language breakdown
- Links to each repository on GitHub

### Heavy Tests Tab
- Dispatch **UpstreamDrift** heavy integration tests via GitHub Actions (`workflow_dispatch`)
- Run tests locally via Docker with configurable Python version (3.10/3.11/3.12) and branch
- Live Docker output display
- Recent workflow run history table

### Daily Reports Tab
- Browse daily progress reports (markdown files)
- Full markdown rendering with tables, headers, and code blocks
- Key metrics extracted and displayed as stat cards
- Assessment chart images shown inline

### Mobile-Responsive Layout
- Touch-friendly tab bar with horizontal scrolling
- Responsive grid breakpoints for stat cards (4 → 2 columns)
- Compact header on small screens
- PWA meta tags for home screen installation

## Quick Start

```bash
# Set your GitHub token
export GITHUB_TOKEN="ghp_..."

# Optional: point to your reports directory
export REPORTS_DIR="$HOME/progress-tracking/reports"

# Start the dashboard
cd runner-dashboard
python3 backend/server.py
```

Open http://localhost:8321 in your browser.

## Configuration

| Environment Variable | Default | Description |
|---|---|---|
| `DASHBOARD_PORT` | `8321` | HTTP server port |
| `GITHUB_TOKEN` | (none) | GitHub PAT for API access |
| `REPORTS_DIR` | `~/progress-tracking/reports` | Path to daily report markdown files |
| `HEAVY_TEST_REPOS` | UpstreamDrift config (JSON) | JSON array of heavy test repo configs |

## Remote Access via Tailscale

To access the dashboard from other devices on your Tailnet:

```bash
sudo bash deploy/setup-tailscale.sh
```

## API Endpoints

| Method | Path | Description |
|---|---|---|
| GET | `/api/runners` | List self-hosted runners |
| GET | `/api/repos` | List organization repositories |
| GET | `/api/reports` | List available daily reports |
| GET | `/api/reports?date=YYYY-MM-DD` | Read a specific report with metrics |
| GET | `/api/reports/{date}/chart` | Serve assessment chart image |
| GET | `/api/heavy-tests/repos` | List configured heavy test repos |
| GET | `/api/heavy-tests/runs?owner=...&repo=...&workflow=...` | Recent workflow runs |
| POST | `/api/heavy-tests/dispatch` | Trigger GitHub Actions workflow |
| POST | `/api/heavy-tests/docker` | Run heavy tests locally in Docker |
