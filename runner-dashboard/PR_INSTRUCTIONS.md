# PR Instructions — Runner Dashboard v4

## What changed

This PR adds the **Runner Dashboard v4**, a four-tab web dashboard for monitoring
self-hosted GitHub Actions runners, organization repositories, heavy integration
tests, and daily progress reports.

## Files

- `runner-dashboard/backend/server.py` — Python HTTP server (v4) with endpoints for
  runners, repos, reports, and heavy test dispatch/Docker execution
- `runner-dashboard/frontend/index.html` — Single-page dashboard with four tabs,
  mobile-responsive CSS, and PWA meta tags
- `runner-dashboard/deploy/setup-tailscale.sh` — Tailscale installation and firewall
  setup for remote access
- `runner-dashboard/README.md` — Documentation for v4 features, configuration, and API
- `runner-dashboard/PR_INSTRUCTIONS.md` — This file

## How to test

1. `cd runner-dashboard && python3 backend/server.py`
2. Open http://localhost:8321
3. **Fleet tab**: Verify stat cards, collapsible sections, runner table
4. **Organization tab**: Verify repo list loads with issue counts
5. **Heavy Tests tab**: Verify UpstreamDrift shows with Python version selector
6. **Reports tab**: Verify daily reports load and render as markdown
7. **Mobile**: Use browser DevTools responsive mode to test mobile layout

## Requirements

- Python 3.10+
- `GITHUB_TOKEN` environment variable for API access
- Docker (optional, for local heavy test execution)
- Tailscale (optional, for remote access)
