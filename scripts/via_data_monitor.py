#!/usr/bin/env python3
"""
VIA Data Monitor - Session-based Data Extraction

Creates timestamped session folders with:
- Full debug log for entire session
- Individual CSV/TXT pairs per measurement

Directory structure:
~/via_outputlogs/YYYYMMDD.HHMM/
├── VIA.YYYYMMDD.HHMM.log          (full session log)
├── VIA.YYYYMMDD.HHMM.SS.csv       (measurement #1 - CSV)
├── VIA.YYYYMMDD.HHMM.SS.txt       (measurement #1 - TXT)
├── VIA.YYYYMMDD.HHMM.SS.csv       (measurement #2 - CSV)
├── VIA.YYYYMMDD.HHMM.SS.txt       (measurement #2 - TXT)
└── ...

CSV format: 21,01,06,10,B0,01,00,06
TXT format: 21 01 06 10 B0 01 00 06

Usage:
    python3 via_data_monitor.py /dev/ttyACM0
"""

import serial
import sys
from datetime import datetime
from pathlib import Path

def create_session_directory():
    """Create timestamped session directory ~/via_outputlogs/YYYYMMDD.HHMM/"""
    base_dir = Path.home() / "via_outputlogs"
    session_timestamp = datetime.now().strftime("%Y%m%d.%H%M")
    session_dir = base_dir / session_timestamp
    session_dir.mkdir(parents=True, exist_ok=True)
    return session_dir, session_timestamp

def generate_measurement_filenames(session_timestamp):
    """Generate measurement filenames with seconds: VIA.YYYYMMDD.HHMM.SS.{csv,txt}"""
    measurement_timestamp = datetime.now().strftime("%Y%m%d.%H%M.%S")
    base_name = f"VIA.{measurement_timestamp}"
    return f"{base_name}.csv", f"{base_name}.txt"

def generate_log_filename(session_timestamp):
    """Generate session log filename: VIA.YYYYMMDD.HHMM.log"""
    return f"VIA.{session_timestamp}.log"

def monitor_serial(port):
    """Monitor serial port and save raw hex byte data + full session log"""

    # Create session directory
    session_dir, session_timestamp = create_session_directory()
    log_filename = generate_log_filename(session_timestamp)
    log_file_path = session_dir / log_filename

    print(f"📁 Session directory: {session_dir}")
    print(f"📝 Session log: {log_filename}")
    print(f"🔌 Monitoring serial port: {port}")
    print(f"📊 Saving raw hex byte data (CSV + TXT) + full log")
    print(f"⏳ Waiting for measurements...\n")

    ser = serial.Serial(port, 115200, timeout=1)

    # Open session log file
    log_file = open(log_file_path, 'w', buffering=1)  # Line buffered

    reading_data = False
    hex_data_txt = []  # Space-separated for TXT
    hex_data_csv = []  # Comma-separated for CSV
    current_csv_file = None
    current_txt_file = None
    line_count = 0

    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()

                # Write ALL lines to session log
                log_file.write(line + '\n')

                # Check for start of hex data (after "Reading full 4106-byte measurement...")
                if "Reading full 4106-byte measurement" in line:
                    reading_data = True
                    hex_data_txt = []
                    hex_data_csv = []
                    line_count = 0
                    csv_name, txt_name = generate_measurement_filenames(session_timestamp)
                    current_csv_file = session_dir / csv_name
                    current_txt_file = session_dir / txt_name
                    print(f"\n📊 Measurement detected:")
                    print(f"   CSV: {csv_name}")
                    print(f"   TXT: {txt_name}")
                    continue

                # Check for end of measurement data
                if "Full 4106 bytes received" in line and reading_data:
                    # Save CSV file (comma-separated)
                    if hex_data_csv and current_csv_file:
                        with open(current_csv_file, 'w') as f:
                            f.write("===VIA START===\n")
                            f.write('\n'.join(hex_data_csv) + '\n')
                            f.write("===VIA STOP===\n")
                        print(f"✅ Saved CSV: {current_csv_file.name} ({len(hex_data_csv)} lines)")

                    # Save TXT file (space-separated)
                    if hex_data_txt and current_txt_file:
                        with open(current_txt_file, 'w') as f:
                            f.write("===VIA START===\n")
                            f.write('\n'.join(hex_data_txt) + '\n')
                            f.write("===VIA STOP===\n")
                        print(f"✅ Saved TXT: {current_txt_file.name} ({len(hex_data_txt)} lines)")

                    reading_data = False
                    hex_data_txt = []
                    hex_data_csv = []
                    current_csv_file = None
                    current_txt_file = None
                    line_count = 0
                    continue

                # Capture hex byte lines (lines with hex patterns like "00 01 02 03")
                if reading_data:
                    # Check if line contains hex bytes (pattern: XX XX XX where X is hex digit)
                    if line and all(c in '0123456789ABCDEFabcdef ' for c in line):
                        # Save TXT format (space-separated, original)
                        hex_data_txt.append(line)
                        # Save CSV format (comma-separated)
                        csv_line = line.replace(' ', ',')
                        hex_data_csv.append(csv_line)
                        line_count += 1

    except KeyboardInterrupt:
        print("\n\n⏹  Monitoring stopped")
        log_file.close()
        ser.close()
        print(f"📁 Session saved to: {session_dir}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 via_data_monitor.py <port>")
        print("\nExample:")
        print("  python3 via_data_monitor.py /dev/ttyACM0")
        print("\nThen in another terminal:")
        print("  screen /dev/ttyACM0 115200")
        print("\nOr use the console wrapper:")
        print("  ./via_console.sh")
        sys.exit(1)

    port = sys.argv[1]
    monitor_serial(port)
