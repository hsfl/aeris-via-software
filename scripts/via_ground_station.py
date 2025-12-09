#!/usr/bin/env python3
"""
VIA Ground Station Receiver

Connects to a receiver Teensy (running radio_receive_test or similar)
and captures VIA measurement data transmitted via radio.

Protocol:
  - Header: "VIA:4106:" (payload type and size)
  - Data:   60-byte chunks of raw measurement data
  - Footer: ":END"

Directory structure (same as OBC):
~/Aeris/data/via/YYYYMMDD.HHMM/
├── VIA.YYYYMMDD.HHMM.log          (full session log)
├── VIA.YYYYMMDD.HHMM.SS.txt       (measurement data)
└── ...

Usage:
    python3 via_ground_station.py /dev/ttyACM0
    python3 via_ground_station.py /dev/ttyACM0 -v    # Verbose mode

Controls:
    Ctrl+C to exit
"""

import serial
import sys
import argparse
import time
from datetime import datetime
from pathlib import Path


def create_session_directory():
    """Create timestamped session directory"""
    base_dir = Path.home() / "Aeris" / "data" / "via"
    session_timestamp = datetime.now().strftime("%Y%m%d.%H%M")
    session_dir = base_dir / session_timestamp
    session_dir.mkdir(parents=True, exist_ok=True)
    return session_dir, session_timestamp


def generate_measurement_filename(session_timestamp):
    """Generate measurement filename with seconds"""
    measurement_timestamp = datetime.now().strftime("%Y%m%d.%H%M.%S")
    return f"VIA.{measurement_timestamp}.txt"


def generate_log_filename(session_timestamp):
    """Generate session log filename"""
    return f"VIA.{session_timestamp}.log"


def print_progress_bar(current, total, label, width=30):
    """Print a progress bar that updates in place"""
    percent = current / total if total > 0 else 0
    filled = int(width * percent)
    bar = '#' * filled + '-' * (width - filled)
    sys.stdout.write(f"\r  {label}: [{bar}] {current}/{total}    ")
    sys.stdout.flush()


def finish_progress_bar(label, total, actual=None):
    """Complete a progress bar with checkmark"""
    if actual is None:
        actual = total
    width = 30
    bar = '#' * width
    sys.stdout.write(f"\r  {label}: [{bar}] {actual}/{total} OK\n")
    sys.stdout.flush()


def ground_station(port, verbose=False, baud=9600):
    """Ground station receiver main loop

    Args:
        port: Serial port path
        verbose: Show full raw data output
        baud: Baud rate (default 9600 for Artemis receiver)
    """

    # Create session directory
    session_dir, session_timestamp = create_session_directory()
    log_filename = generate_log_filename(session_timestamp)
    log_file_path = session_dir / log_filename

    print("=" * 55)
    print("  VIA Ground Station Receiver")
    print("=" * 55)
    print(f"  Port:         {port}")
    print(f"  Baud:         {baud}")
    print(f"  Session dir:  {session_dir}")
    print(f"  Log file:     {log_filename}")
    print(f"  Verbose:      {'ON' if verbose else 'OFF'}")
    print("=" * 55)
    print()
    print("Waiting for VIA data...")
    print("Ctrl+C to exit")
    print()

    # Open serial port
    ser = serial.Serial(port, baud, timeout=0.1)

    # Open session log
    log_file = open(log_file_path, 'w', buffering=1)

    # Reception state
    receiving_data = False
    data_buffer = bytearray()
    expected_size = 0
    measurement_count = 0

    # For accumulating partial messages
    line_buffer = ""

    try:
        while True:
            # Read available data
            if ser.in_waiting > 0:
                raw_data = ser.read(ser.in_waiting)

                # Log raw data
                try:
                    text = raw_data.decode('utf-8', errors='ignore')
                    log_file.write(text)
                except:
                    pass

                # If we're in data reception mode, accumulate binary
                if receiving_data:
                    data_buffer.extend(raw_data)

                    if not verbose:
                        print_progress_bar(len(data_buffer), expected_size, "Radio RX")
                    else:
                        # Print hex dump
                        hex_str = ' '.join(f'{b:02X}' for b in raw_data)
                        print(f"  RX: {hex_str}")

                    # Check for end marker in buffer
                    if b':END' in data_buffer:
                        # Extract data before :END
                        end_idx = data_buffer.find(b':END')
                        measurement_data = bytes(data_buffer[:end_idx])

                        if not verbose:
                            finish_progress_bar("Radio RX", expected_size, len(measurement_data))

                        # Save measurement
                        measurement_count += 1
                        filename = generate_measurement_filename(session_timestamp)
                        filepath = session_dir / filename

                        with open(filepath, 'wb') as f:
                            f.write(b"===VIA START===\n")
                            # Write as hex dump (16 bytes per line)
                            for i in range(0, len(measurement_data), 16):
                                chunk = measurement_data[i:i+16]
                                hex_line = ' '.join(f'{b:02X}' for b in chunk)
                                f.write(hex_line.encode() + b'\n')
                            f.write(b"===VIA STOP===\n")

                        print(f"\nMeasurement #{measurement_count} saved: {filename}")
                        print(f"  Size: {len(measurement_data)} bytes")
                        print(f"  Path: {filepath}")
                        print()
                        print("Waiting for next transmission...")
                        print()

                        # Reset state
                        receiving_data = False
                        data_buffer = bytearray()
                        expected_size = 0

                    continue

                # Not receiving data - look for header
                try:
                    text = raw_data.decode('utf-8', errors='ignore')
                    line_buffer += text

                    # Process complete lines
                    while '\n' in line_buffer:
                        line, line_buffer = line_buffer.split('\n', 1)
                        line = line.strip()

                        if verbose and line:
                            print(f"RX: {line}")

                        # Check for VIA header
                        if line.startswith('VIA:'):
                            # Parse "VIA:4106:"
                            parts = line.split(':')
                            if len(parts) >= 2:
                                try:
                                    expected_size = int(parts[1])
                                    receiving_data = True
                                    data_buffer = bytearray()

                                    print()
                                    print("-" * 40)
                                    print(f"  Receiving VIA measurement")
                                    print(f"  Expected size: {expected_size} bytes")
                                    print("-" * 40)

                                except ValueError:
                                    pass

                        # Check for Hello World test
                        elif 'Hello World' in line:
                            print(f"Test message received: {line}")

                        # Show RSSI if present
                        elif 'RSSI' in line:
                            print(f"  Signal: {line}")

                except:
                    pass

            # Small delay
            time.sleep(0.01)

    except KeyboardInterrupt:
        pass

    finally:
        log_file.close()
        ser.close()
        print("\n")
        print("Ground station closed")
        print(f"Data saved in: {session_dir}")
        print(f"Measurements received: {measurement_count}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="VIA Ground Station Receiver",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 via_ground_station.py /dev/ttyACM0           # Normal mode
  python3 via_ground_station.py /dev/ttyACM0 -v        # Verbose mode
  python3 via_ground_station.py /dev/ttyACM0 -b 115200 # Different baud

The receiver Teensy should be running radio_receive_test.ino or similar.
        """
    )
    parser.add_argument("port", help="Serial port (e.g., /dev/ttyACM0)")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Verbose mode - show full raw data output")
    parser.add_argument("-b", "--baud", type=int, default=9600,
                        help="Baud rate (default: 9600)")

    args = parser.parse_args()
    ground_station(args.port, verbose=args.verbose, baud=args.baud)
