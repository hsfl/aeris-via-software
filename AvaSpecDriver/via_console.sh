#!/bin/bash
# VIA Console with Automatic Logging (Session-based)
#
# This script starts a screen session with logging in a timestamped session folder
# Use with via_data_monitor.py to also capture clean CSV/TXT data files
#
# Usage: ./via_console.sh [port]
#        Default port: /dev/ttyACM0

PORT="${1:-/dev/ttyACM0}"
BASE_DIR="$HOME/via_outputlogs"
SESSION_TIMESTAMP=$(date +%Y%m%d.%H%M)
SESSION_DIR="$BASE_DIR/$SESSION_TIMESTAMP"
LOGFILE="$SESSION_DIR/VIA.$SESSION_TIMESTAMP.log"

# Create session directory if it doesn't exist
mkdir -p "$SESSION_DIR"

echo "═══════════════════════════════════════════════════"
echo "  VIA Console with Automatic Logging"
echo "═══════════════════════════════════════════════════"
echo "  Port:         $PORT"
echo "  Session dir:  $SESSION_DIR"
echo "  Log file:     VIA.$SESSION_TIMESTAMP.log"
echo "═══════════════════════════════════════════════════"
echo ""
echo "Starting screen session with logging enabled..."
echo "To exit: Ctrl+A then K (kill window)"
echo ""
echo "TIP: Run via_data_monitor.py in another terminal"
echo "     to also capture clean CSV/TXT data files"
echo ""

# Start screen with logging enabled
# -L: Turn on output logging
# -Logfile: Specify log file location
screen -L -Logfile "$LOGFILE" "$PORT" 115200
