#!/usr/bin/env python3
"""
VIA Automatic Measurement Monitor

This script monitors the VIA serial console and automatically saves
each measurement to timestamped files in ~/via_outputlogs/

Files are saved as: ~/via_outputlogs/VIA.YYYYMMDD.HHMMSS.txt

Usage:
    python3 via_monitor.py /dev/ttyACM0

Then in another terminal, connect with screen and use VIA_MEASURE normally:
    screen /dev/ttyACM0 115200
"""

import serial
import sys
import os
from datetime import datetime
from pathlib import Path

def ensure_output_directory():
    """Create ~/via_outputlogs/ directory if it doesn't exist"""
    output_dir = Path.home() / "via_outputlogs"
    output_dir.mkdir(exist_ok=True)
    return output_dir

def generate_filename():
    """Generate timestamp filename: VIA.YYYYMMDD.HHMMSS.txt"""
    timestamp = datetime.now().strftime("%Y%m%d.%H%M%S")
    return f"VIA.{timestamp}.txt"

def monitor_serial(port):
    """Monitor serial port and automatically save measurements"""

    output_dir = ensure_output_directory()
    print(f"📁 Output directory: {output_dir}")
    print(f"🔌 Monitoring serial port: {port}")
    print(f"⏳ Waiting for measurements...\n")

    ser = serial.Serial(port, 115200, timeout=1)

    csv_started = False
    csv_data = []
    current_file = None

    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()

                # Check for CSV data markers
                if "CSV DATA OUTPUT:" in line:
                    csv_started = True
                    csv_data = []
                    current_file = output_dir / generate_filename()
                    print(f"\n📊 Measurement detected, saving to: {current_file.name}")
                    continue

                if "END CSV DATA" in line and csv_started:
                    # Save the captured data
                    if csv_data and current_file:
                        with open(current_file, 'w') as f:
                            f.write('\n'.join(csv_data) + '\n')
                        print(f"✅ Saved {len(csv_data)} rows to {current_file}")

                    csv_started = False
                    csv_data = []
                    current_file = None
                    continue

                # Capture CSV lines
                if csv_started and line and ',' in line:
                    csv_data.append(line)

    except KeyboardInterrupt:
        print("\n\n⏹  Monitoring stopped")
        ser.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 via_monitor.py <port>")
        print("\nExample:")
        print("  python3 via_monitor.py /dev/ttyACM0")
        print("\nThen in another terminal:")
        print("  screen /dev/ttyACM0 115200")
        sys.exit(1)

    port = sys.argv[1]
    monitor_serial(port)
