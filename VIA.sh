#!/bin/bash
# VIA Unified Console
#
# Interactive console with automatic session logging
# All data automatically saved to timestamped session folders
#
# Usage: ./VIA.sh [port] [-v]
#        Default port: /dev/ttyACM0
#        -v: Verbose mode (full raw data output)

# Parse arguments - find port and flags separately
PORT="/dev/ttyACM0"
VERBOSE=""

for arg in "$@"; do
    if [[ "$arg" == "-v" || "$arg" == "--verbose" ]]; then
        VERBOSE="-v"
    elif [[ "$arg" != -* ]]; then
        PORT="$arg"
    fi
done

# Just run the interactive Python script - it handles everything
exec python3 scripts/via_interactive.py "$PORT" $VERBOSE
