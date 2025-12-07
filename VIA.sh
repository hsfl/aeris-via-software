#!/bin/bash
# VIA Unified Console
#
# Interactive console with automatic session logging
# All data automatically saved to timestamped session folders
#
# Usage: ./VIA.sh [port] [-v] [--sim]
#        Default port: /dev/ttyACM0
#        -v: Verbose mode (full raw data output)
#        --sim: Simulation mode (run native binary)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATIVE_BIN="$SCRIPT_DIR/AvaSpecDriver/native/via_native"

# Parse arguments - find port and flags separately
PORT="/dev/ttyACM0"
VERBOSE=""
SIMULATION=""

for arg in "$@"; do
    if [[ "$arg" == "-v" || "$arg" == "--verbose" ]]; then
        VERBOSE="-v"
    elif [[ "$arg" == "--sim" || "$arg" == "--native" ]]; then
        SIMULATION="--native"
    elif [[ "$arg" != -* ]]; then
        PORT="$arg"
    fi
done

# Run the interactive Python script
if [[ -n "$SIMULATION" ]]; then
    if [[ ! -x "$NATIVE_BIN" ]]; then
        echo "Error: Native binary not found at $NATIVE_BIN"
        echo "Build it with: cd AvaSpecDriver/native && make"
        exit 1
    fi
    exec python3 scripts/via_interactive.py --native "$NATIVE_BIN" $VERBOSE
else
    exec python3 scripts/via_interactive.py "$PORT" $VERBOSE
fi
