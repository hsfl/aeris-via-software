#!/bin/bash
# test_file_transfer.sh
# Simple test script for VIA file transfer functionality

# Configuration
SERIAL_PORT="${1:-/dev/ttyACM0}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/file_transfer.py"

echo "========================================"
echo "VIA File Transfer Test Script"
echo "========================================"
echo "Serial Port: $SERIAL_PORT"
echo ""

# Check if Python script exists
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo "❌ Error: file_transfer.py not found!"
    echo "Expected location: $PYTHON_SCRIPT"
    exit 1
fi

# Check if serial port exists
if [ ! -e "$SERIAL_PORT" ]; then
    echo "❌ Error: Serial port $SERIAL_PORT not found!"
    echo ""
    echo "Available serial ports:"
    ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  None found"
    echo ""
    echo "Usage: $0 [serial_port]"
    echo "Example: $0 /dev/ttyACM0"
    exit 1
fi

# Check permissions
if [ ! -r "$SERIAL_PORT" ] || [ ! -w "$SERIAL_PORT" ]; then
    echo "⚠️  Warning: You may not have permission to access $SERIAL_PORT"
    echo "Try: sudo chmod 666 $SERIAL_PORT"
    echo ""
fi

# Check for pyserial
echo "Checking dependencies..."
python3 -c "import serial" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "❌ Error: pyserial not installed"
    echo "Install with: pip install pyserial"
    exit 1
fi
echo "✅ Dependencies OK"
echo ""

# Test 1: List files
echo "========================================"
echo "Test 1: Listing files on SD card"
echo "========================================"
python3 "$PYTHON_SCRIPT" "$SERIAL_PORT" list

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ File listing failed!"
    echo "Please check:"
    echo "  1. Teensy is connected and powered"
    echo "  2. Firmware is uploaded"
    echo "  3. SD card is inserted"
    exit 1
fi

echo ""
echo "✅ File listing successful!"
echo ""

# Interactive prompt for file download
read -p "Do you want to download a file? (y/n): " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    read -p "Enter filename to download: " filename

    if [ -n "$filename" ]; then
        echo ""
        echo "========================================"
        echo "Test 2: Downloading file: $filename"
        echo "========================================"
        python3 "$PYTHON_SCRIPT" "$SERIAL_PORT" get "$filename"

        if [ $? -eq 0 ]; then
            echo ""
            echo "✅ File download successful!"

            if [ -f "$filename" ]; then
                echo ""
                echo "File info:"
                ls -lh "$filename"
                echo ""
                echo "First 10 lines:"
                head -n 10 "$filename" 2>/dev/null || echo "(Binary file)"
            fi
        else
            echo ""
            echo "❌ File download failed!"
        fi
    fi
fi

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"
echo ""
echo "To use interactive mode:"
echo "  python3 $PYTHON_SCRIPT $SERIAL_PORT"
echo ""
