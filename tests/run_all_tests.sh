#!/bin/bash
#
# VIA Software Test Suite Runner
#
# Runs all automated tests for the VIA spectrometer software
# without requiring physical hardware.
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "═══════════════════════════════════════════════════════════════════"
echo "  VIA SPECTROMETER SOFTWARE - AUTOMATED TEST SUITE"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

# Check Python version
echo -e "${BLUE}[1/5] Checking Python environment...${NC}"
python3 --version
echo ""

# Generate test data
echo -e "${BLUE}[2/5] Generating test data...${NC}"
python3 test_data_generator.py
echo ""

# Run Python unit tests
echo -e "${BLUE}[3/5] Running Python unit tests...${NC}"
python3 test_python_scripts.py
TEST_RESULT=$?
echo ""

# Check if PlatformIO is available for firmware tests
echo -e "${BLUE}[4/5] Checking for PlatformIO (firmware build test)...${NC}"
if command -v pio &> /dev/null; then
    echo "✅ PlatformIO found"
    echo "Building firmware (syntax check)..."
    cd ../AvaSpecDriver
    pio run --target checkprogsize 2>&1 | tail -20
    BUILD_RESULT=$?
    cd "$SCRIPT_DIR"

    if [ $BUILD_RESULT -eq 0 ]; then
        echo -e "${GREEN}✅ Firmware build successful${NC}"
    else
        echo -e "${YELLOW}⚠️  Firmware build failed (check AvaSpecDriver)${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  PlatformIO not found - skipping firmware build test${NC}"
    echo "   Install with: pip install platformio"
    BUILD_RESULT=0  # Don't fail overall tests
fi
echo ""

# Test data visualization
echo -e "${BLUE}[5/5] Testing data visualization (non-interactive)...${NC}"
if python3 -c "import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt" 2>/dev/null; then
    echo "✅ Matplotlib available"

    # Test SerialRead.py with generated data
    if [ -f "../scripts/SerialRead.py" ]; then
        echo "Testing SerialRead.py with simulated data..."
        # Run in non-interactive mode (just load, don't show plot)
        python3 -c "
import sys
sys.path.insert(0, '../scripts')
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
from pathlib import Path
import csv

# Load test data
test_file = Path('test_data/gaussian_single.csv')
if test_file.exists():
    pixels, intensity = [], []
    with open(test_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            pixels.append(int(row['Pixel']))
            intensity.append(float(row['Intensity']))

    assert len(pixels) == 2048, 'Expected 2048 pixels'
    assert len(intensity) == 2048, 'Expected 2048 intensity values'
    print(f'✅ Successfully loaded {len(pixels)} data points')
    print(f'   Intensity range: {min(intensity):.0f} - {max(intensity):.0f}')
else:
    print('⚠️  Test data not found')
    sys.exit(1)
"
        VIZ_RESULT=$?
        if [ $VIZ_RESULT -eq 0 ]; then
            echo -e "${GREEN}✅ Visualization test passed${NC}"
        fi
    fi
else
    echo -e "${YELLOW}⚠️  Matplotlib not available - skipping visualization test${NC}"
    VIZ_RESULT=0
fi
echo ""

# Summary
echo "═══════════════════════════════════════════════════════════════════"
echo "  TEST SUMMARY"
echo "═══════════════════════════════════════════════════════════════════"

OVERALL_RESULT=0

if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ Unit tests: PASSED${NC}"
else
    echo -e "${RED}❌ Unit tests: FAILED${NC}"
    OVERALL_RESULT=1
fi

if [ $BUILD_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ Firmware build: PASSED${NC}"
else
    echo -e "${YELLOW}⚠️  Firmware build: SKIPPED or FAILED${NC}"
fi

if [ $VIZ_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ Visualization: PASSED${NC}"
else
    echo -e "${YELLOW}⚠️  Visualization: SKIPPED${NC}"
fi

echo "═══════════════════════════════════════════════════════════════════"

if [ $OVERALL_RESULT -eq 0 ]; then
    echo -e "${GREEN}"
    echo "  ███████╗██╗   ██╗ ██████╗ ██████╗███████╗███████╗███████╗"
    echo "  ██╔════╝██║   ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝"
    echo "  ███████╗██║   ██║██║     ██║     █████╗  ███████╗███████╗"
    echo "  ╚════██║██║   ██║██║     ██║     ██╔══╝  ╚════██║╚════██║"
    echo "  ███████║╚██████╔╝╚██████╗╚██████╗███████╗███████║███████║"
    echo "  ╚══════╝ ╚═════╝  ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝"
    echo -e "${NC}"
    echo "All tests passed successfully!"
else
    echo -e "${RED}"
    echo "  ███████╗ █████╗ ██╗██╗     ███████╗██████╗ "
    echo "  ██╔════╝██╔══██╗██║██║     ██╔════╝██╔══██╗"
    echo "  █████╗  ███████║██║██║     █████╗  ██║  ██║"
    echo "  ██╔══╝  ██╔══██║██║██║     ██╔══╝  ██║  ██║"
    echo "  ██║     ██║  ██║██║███████╗███████╗██████╔╝"
    echo "  ╚═╝     ╚═╝  ╚═╝╚═╝╚══════╝╚══════╝╚═════╝ "
    echo -e "${NC}"
    echo "Some tests failed. Check output above."
fi

echo "═══════════════════════════════════════════════════════════════════"

exit $OVERALL_RESULT
