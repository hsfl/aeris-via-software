#!/usr/bin/env python3
"""
VIA Interactive Console with Automatic Data Logging

Single unified solution that provides:
- Interactive serial console (like screen)
- Automatic session folder creation
- Full debug log
- Clean CSV/TXT extraction per measurement

Directory structure:
~/Aeris/data/via/YYYYMMDD.HHMM/
├── VIA.YYYYMMDD.HHMM.log          (full session log)
├── VIA.YYYYMMDD.HHMM.SS.csv       (measurement #1 - CSV)
├── VIA.YYYYMMDD.HHMM.SS.txt       (measurement #1 - TXT)
└── ...

Usage:
    python3 via_interactive.py /dev/ttyACM0

Controls:
    Type commands normally (VIA_MEASURE, VIA_STATUS, etc.)
    Ctrl+C to exit
"""

import serial
import sys
import select
import termios
import tty
from datetime import datetime
from pathlib import Path

def create_session_directory():
    """Create timestamped session directory"""
    base_dir = Path.home() / "Aeris" / "data" / "via"
    session_timestamp = datetime.now().strftime("%Y%m%d.%H%M")
    session_dir = base_dir / session_timestamp
    session_dir.mkdir(parents=True, exist_ok=True)
    return session_dir, session_timestamp

def generate_measurement_filenames(session_timestamp):
    """Generate measurement filenames with seconds"""
    measurement_timestamp = datetime.now().strftime("%Y%m%d.%H%M.%S")
    return f"VIA.{measurement_timestamp}.csv", f"VIA.{measurement_timestamp}.txt"

def generate_log_filename(session_timestamp):
    """Generate session log filename"""
    return f"VIA.{session_timestamp}.log"

def interactive_console(port):
    """Interactive console with automatic logging"""

    # Create session directory
    session_dir, session_timestamp = create_session_directory()
    log_filename = generate_log_filename(session_timestamp)
    log_file_path = session_dir / log_filename

    print("═══════════════════════════════════════════════════")
    print("  VIA Interactive Console")
    print("═══════════════════════════════════════════════════")
    print(f"  Port:         {port}")
    print(f"  Session dir:  {session_dir}")
    print(f"  Log file:     {log_filename}")
    print("═══════════════════════════════════════════════════")
    print()
    print("📊 Auto-saving: Full log + CSV/TXT per measurement")
    print("⌨️  Type commands normally, Ctrl+C to exit")
    print()

    # Open serial port
    ser = serial.Serial(port, 115200, timeout=0.1)

    # Open session log
    log_file = open(log_file_path, 'w', buffering=1)

    # Data capture state
    reading_data = False
    hex_data_txt = []
    hex_data_csv = []
    current_csv_file = None
    current_txt_file = None

    # Save terminal settings
    old_settings = termios.tcgetattr(sys.stdin)

    try:
        # Set terminal to raw mode for character-by-character input
        tty.setraw(sys.stdin.fileno())

        while True:
            # Check for user keyboard input
            if select.select([sys.stdin], [], [], 0)[0]:
                char = sys.stdin.read(1)

                # Ctrl+C to exit
                if ord(char) == 3:  # Ctrl+C
                    break

                # Send to serial port
                ser.write(char.encode())

            # Check for serial data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                text = data.decode('utf-8', errors='ignore')

                # Echo to console
                sys.stdout.write(text)
                sys.stdout.flush()

                # Write to log file
                log_file.write(text)

                # Process line-by-line for hex data extraction
                for line in text.split('\n'):
                    line = line.strip()

                    # Detect measurement start
                    if "Reading full 4106-byte measurement" in line:
                        reading_data = True
                        hex_data_txt = []
                        hex_data_csv = []
                        csv_name, txt_name = generate_measurement_filenames(session_timestamp)
                        current_csv_file = session_dir / csv_name
                        current_txt_file = session_dir / txt_name

                    # Detect measurement end
                    elif "Full 4106 bytes received" in line and reading_data:
                        # Save CSV
                        if hex_data_csv and current_csv_file:
                            with open(current_csv_file, 'w') as f:
                                f.write("===VIA START===\n")
                                f.write('\n'.join(hex_data_csv) + '\n')
                                f.write("===VIA STOP===\n")

                        # Save TXT
                        if hex_data_txt and current_txt_file:
                            with open(current_txt_file, 'w') as f:
                                f.write("===VIA START===\n")
                                f.write('\n'.join(hex_data_txt) + '\n')
                                f.write("===VIA STOP===\n")

                        reading_data = False
                        hex_data_txt = []
                        hex_data_csv = []

                    # Capture hex lines during measurement
                    elif reading_data and line:
                        if all(c in '0123456789ABCDEFabcdef ' for c in line):
                            hex_data_txt.append(line)
                            csv_line = line.replace(' ', ',')
                            hex_data_csv.append(csv_line)

    except KeyboardInterrupt:
        pass

    finally:
        # Restore terminal settings
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
        log_file.close()
        ser.close()
        print("\n\n✅ Session closed")
        print(f"📁 Data saved in: {session_dir}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 via_interactive.py <port>")
        print("\nExample:")
        print("  python3 via_interactive.py /dev/ttyACM0")
        sys.exit(1)

    port = sys.argv[1]
    interactive_console(port)
