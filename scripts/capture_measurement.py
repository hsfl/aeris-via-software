#!/usr/bin/env python3
"""
Capture VIA Measurement Data from Serial Port

This script connects to the Teensy serial port, sends a VIA_MEASURE command,
and captures the CSV data output to a file on the computer.

Usage:
    python3 capture_measurement.py /dev/ttyACM0 output.csv
"""

import serial
import sys
import time

def capture_measurement(port, output_file):
    """Send VIA_MEASURE command and capture CSV data"""

    print(f"Opening serial port: {port}")
    ser = serial.Serial(port, 115200, timeout=10)
    time.sleep(1)  # Wait for serial to stabilize

    # Clear any pending data
    ser.reset_input_buffer()

    print(f"\nSending command: VIA_MEASURE")
    ser.write(b'VIA_MEASURE\n')
    ser.flush()

    print("Waiting for measurement to complete...")
    print("(This may take ~15 seconds)\n")

    # Wait for CSV data to start
    csv_started = False
    csv_data = []

    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            # Print all output to terminal
            print(line)

            # Check for CSV data markers
            if "CSV DATA OUTPUT:" in line:
                csv_started = True
                csv_data = []  # Reset
                continue

            if "END CSV DATA" in line:
                break

            # Capture CSV lines
            if csv_started and line and ',' in line:
                csv_data.append(line)

    ser.close()

    # Save to file
    if csv_data:
        with open(output_file, 'w') as f:
            f.write('\n'.join(csv_data) + '\n')
        
        print(f"\n✅ Measurement data saved to: {output_file}")
        print(f"   Total rows: {len(csv_data)}")
    else:
        print("\n❌ No CSV data captured!")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 capture_measurement.py <port> <output_file>")
        print("\nExample:")
        print("  python3 capture_measurement.py /dev/ttyACM0 spectrum.csv")
        sys.exit(1)

    port = sys.argv[1]
    output_file = sys.argv[2]

    capture_measurement(port, output_file)
