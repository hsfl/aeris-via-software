#!/bin/bash
# VIA Unified Console
#
# Interactive console with automatic session logging
# All data automatically saved to timestamped session folders
#
# Usage: ./VIA.sh [port]
#        Default port: /dev/ttyACM0

PORT="${1:-/dev/ttyACM0}"

# Just run the interactive Python script - it handles everything
exec python3 scripts/via_interactive.py "$PORT"
