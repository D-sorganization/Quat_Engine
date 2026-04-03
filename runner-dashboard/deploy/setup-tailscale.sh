#!/usr/bin/env bash
# setup-tailscale.sh — Install Tailscale and expose the Runner Dashboard
# Run with: sudo bash deploy/setup-tailscale.sh
set -euo pipefail

DASHBOARD_PORT="${DASHBOARD_PORT:-8321}"

echo "=== Runner Dashboard — Tailscale Setup ==="

# ---------------------------------------------------------------
# 1. Install Tailscale (if not already installed)
# ---------------------------------------------------------------
if ! command -v tailscale &>/dev/null; then
  echo "[1/4] Installing Tailscale..."
  curl -fsSL https://tailscale.com/install.sh | sh
else
  echo "[1/4] Tailscale already installed."
fi

# ---------------------------------------------------------------
# 2. Start / authenticate Tailscale
# ---------------------------------------------------------------
echo "[2/4] Starting Tailscale..."
if ! tailscale status &>/dev/null; then
  echo "  Please authenticate when the browser opens."
  tailscale up
else
  echo "  Tailscale already connected."
fi

TS_IP=$(tailscale ip -4 2>/dev/null || echo "unknown")
echo "  Tailscale IPv4: ${TS_IP}"

# ---------------------------------------------------------------
# 3. Open firewall for the dashboard port (iptables)
# ---------------------------------------------------------------
echo "[3/4] Ensuring port ${DASHBOARD_PORT} is open..."
if command -v ufw &>/dev/null; then
  ufw allow "${DASHBOARD_PORT}/tcp" 2>/dev/null || true
elif command -v iptables &>/dev/null; then
  iptables -C INPUT -p tcp --dport "${DASHBOARD_PORT}" -j ACCEPT 2>/dev/null ||
    iptables -A INPUT -p tcp --dport "${DASHBOARD_PORT}" -j ACCEPT
fi

# ---------------------------------------------------------------
# 4. Print access info
# ---------------------------------------------------------------
echo "[4/4] Done!"
echo ""
echo "  Dashboard URL (Tailscale): http://${TS_IP}:${DASHBOARD_PORT}"
echo "  Dashboard URL (local):     http://localhost:${DASHBOARD_PORT}"
echo ""
echo "  To start the dashboard:"
echo "    cd runner-dashboard && python3 backend/server.py"
echo ""
echo "  Other devices on your Tailnet can now access the dashboard at:"
echo "    http://${TS_IP}:${DASHBOARD_PORT}"
