#!/bin/bash
#
# VIA Software Test Suite Runner
#
# Usage:
#   ./run_all_tests.sh        # Normal mode (test names + results)
#   ./run_all_tests.sh -v     # Verbose mode (full output)
#

# Parse arguments
VERBOSE=0
if [ "$1" == "-v" ] || [ "$1" == "--verbose" ]; then
    VERBOSE=1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Track results
TESTS_PASSED=0
TESTS_FAILED=0

echo "═══════════════════════════════════════════════════════════════════"
echo "  VIA SPECTROMETER - TEST SUITE"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

# ─────────────────────────────────────────────────────────────────────────────
# Stage 1: Generate test data
# ─────────────────────────────────────────────────────────────────────────────
echo -n "  Generating test data... "
if [ $VERBOSE -eq 1 ]; then
    echo ""
    python3 test_data_generator.py
    GEN_RESULT=$?
else
    python3 test_data_generator.py > /dev/null 2>&1
    GEN_RESULT=$?
fi
if [ $GEN_RESULT -eq 0 ]; then
    echo -e "${GREEN}ok${NC}"
else
    echo -e "${RED}FAILED${NC}"
fi
echo ""

# ─────────────────────────────────────────────────────────────────────────────
# Stage 2: Run unit tests
# ─────────────────────────────────────────────────────────────────────────────
echo "  Unit Tests:"
if [ $VERBOSE -eq 1 ]; then
    python3 test_python_scripts.py -v
else
    python3 test_python_scripts.py
fi
UNIT_RESULT=$?

if [ $UNIT_RESULT -eq 0 ]; then
    ((TESTS_PASSED++))
else
    ((TESTS_FAILED++))
fi
echo ""

# ─────────────────────────────────────────────────────────────────────────────
# Stage 3: Firmware build check
# ─────────────────────────────────────────────────────────────────────────────
echo -n "  Firmware build... "
if command -v pio &> /dev/null; then
    if [ $VERBOSE -eq 1 ]; then
        echo ""
        cd ../AvaSpecDriver
        pio run 2>&1 | tail -30
        BUILD_RESULT=$?
        cd "$SCRIPT_DIR"
    else
        cd ../AvaSpecDriver
        pio run > /dev/null 2>&1
        BUILD_RESULT=$?
        cd "$SCRIPT_DIR"
    fi

    if [ $BUILD_RESULT -eq 0 ]; then
        echo -e "${GREEN}ok${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAILED${NC}"
        ((TESTS_FAILED++))
    fi
else
    echo -e "${YELLOW}skipped${NC} (PlatformIO not found)"
    BUILD_RESULT=-1
fi
echo ""

# ─────────────────────────────────────────────────────────────────────────────
# Stage 4: Data format validation
# ─────────────────────────────────────────────────────────────────────────────
echo "  Data Formats:"
FORMAT_PASSED=1
for test_file in test_data/gaussian_single.csv test_data/multi_peak.csv test_data/flat_spectrum.csv; do
    filename=$(basename "$test_file")
    if [ -f "$test_file" ]; then
        LINE_COUNT=$(wc -l < "$test_file")
        if [ "$LINE_COUNT" -ne 2049 ]; then
            FORMAT_PASSED=0
            echo -e "    ${RED}✗${NC} $filename (wrong line count: $LINE_COUNT)"
        else
            echo -e "    ${GREEN}✓${NC} $filename"
        fi
    else
        FORMAT_PASSED=0
        echo -e "    ${RED}✗${NC} $filename (not found)"
    fi
done

if [ $FORMAT_PASSED -eq 1 ]; then
    ((TESTS_PASSED++))
else
    ((TESTS_FAILED++))
fi
echo ""

# ─────────────────────────────────────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────────────────────────────────────
echo "═══════════════════════════════════════════════════════════════════"
echo "  RESULTS"
echo "═══════════════════════════════════════════════════════════════════"

# Unit tests
if [ $UNIT_RESULT -eq 0 ]; then
    echo -e "  ${GREEN}✓${NC} Unit tests"
else
    echo -e "  ${RED}✗${NC} Unit tests"
fi

# Firmware build
if [ $BUILD_RESULT -eq 0 ]; then
    echo -e "  ${GREEN}✓${NC} Firmware build"
elif [ $BUILD_RESULT -eq -1 ]; then
    echo -e "  ${YELLOW}-${NC} Firmware build (skipped)"
else
    echo -e "  ${RED}✗${NC} Firmware build"
fi

# Data formats
if [ $FORMAT_PASSED -eq 1 ]; then
    echo -e "  ${GREEN}✓${NC} Data formats"
else
    echo -e "  ${RED}✗${NC} Data formats"
fi

echo ""

# Final result with ASCII art
if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}"
    echo "  ███████╗██╗   ██╗ ██████╗ ██████╗███████╗███████╗███████╗"
    echo "  ██╔════╝██║   ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝"
    echo "  ███████╗██║   ██║██║     ██║     █████╗  ███████╗███████╗"
    echo "  ╚════██║██║   ██║██║     ██║     ██╔══╝  ╚════██║╚════██║"
    echo "  ███████║╚██████╔╝╚██████╗╚██████╗███████╗███████║███████║"
    echo "  ╚══════╝ ╚═════╝  ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝"
    echo -e "${NC}"
    echo "  All $TESTS_PASSED test groups passed"
    echo ""
    exit 0
else
    echo -e "${RED}"
    echo "  ███████╗ █████╗ ██╗██╗     ███████╗██████╗ "
    echo "  ██╔════╝██╔══██╗██║██║     ██╔════╝██╔══██╗"
    echo "  █████╗  ███████║██║██║     █████╗  ██║  ██║"
    echo "  ██╔══╝  ██╔══██║██║██║     ██╔══╝  ██║  ██║"
    echo "  ██║     ██║  ██║██║███████╗███████╗██████╔╝"
    echo "  ╚═╝     ╚═╝  ╚═╝╚═╝╚══════╝╚══════╝╚═════╝ "
    echo -e "${NC}"
    echo "  $TESTS_FAILED failed, $TESTS_PASSED passed"
    echo ""
    echo "  Run with -v for details"
    echo ""
    exit 1
fi
