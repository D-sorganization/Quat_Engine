#!/usr/bin/env bash
# start-dashboard.sh — Quick start for the Runner Dashboard
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "${SCRIPT_DIR}/backend/server.py" "$@"
