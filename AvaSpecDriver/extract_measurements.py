#!/usr/bin/env python3
"""
Extract CSV/TXT measurements from VIA log files

Takes a VIA session log file and extracts clean CSV/TXT data files
for each measurement found in the log.

Usage:
    python3 extract_measurements.py <log_file>

Example:
    python3 extract_measurements.py ~/via_outputlogs/20251108.1845/VIA.20251108.1845.log
"""

import sys
import re
from pathlib import Path
from datetime import datetime

def extract_measurements_from_log(log_file_path):
    """Extract measurements from log and create CSV/TXT files"""

    log_path = Path(log_file_path)
    if not log_path.exists():
        print(f"❌ Log file not found: {log_file_path}")
        return

    session_dir = log_path.parent

    print(f"📁 Processing: {log_path.name}")
    print(f"📂 Output dir: {session_dir}")
    print()

    with open(log_path, 'r') as f:
        lines = f.readlines()

    reading_measurement = False
    hex_lines = []
    measurement_count = 0

    for line in lines:
        line = line.strip()

        # Detect start of measurement
        if "Reading full 4106-byte measurement" in line:
            reading_measurement = True
            hex_lines = []
            continue

        # Detect end of measurement
        if "Full 4106 bytes received" in line and reading_measurement:
            if hex_lines:
                measurement_count += 1

                # Generate filenames with incrementing counter
                base_timestamp = log_path.stem  # VIA.20251108.1845
                csv_name = f"{base_timestamp}.{measurement_count:02d}.csv"
                txt_name = f"{base_timestamp}.{measurement_count:02d}.txt"

                csv_path = session_dir / csv_name
                txt_path = session_dir / txt_name

                # Write CSV (comma-separated)
                with open(csv_path, 'w') as f:
                    f.write("===VIA START===\n")
                    for hex_line in hex_lines:
                        csv_line = hex_line.replace(' ', ',')
                        f.write(csv_line + '\n')
                    f.write("===VIA STOP===\n")

                # Write TXT (space-separated)
                with open(txt_path, 'w') as f:
                    f.write("===VIA START===\n")
                    for hex_line in hex_lines:
                        f.write(hex_line + '\n')
                    f.write("===VIA STOP===\n")

                print(f"✅ Measurement {measurement_count}:")
                print(f"   CSV: {csv_name}")
                print(f"   TXT: {txt_name}")

            reading_measurement = False
            hex_lines = []
            continue

        # Capture hex lines
        if reading_measurement:
            # Check if line is all hex digits and spaces
            if line and all(c in '0123456789ABCDEFabcdef ' for c in line):
                hex_lines.append(line)

    print()
    print(f"📊 Extracted {measurement_count} measurements")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 extract_measurements.py <log_file>")
        print("\nExample:")
        print("  python3 extract_measurements.py ~/via_outputlogs/20251108.1845/VIA.20251108.1845.log")
        sys.exit(1)

    log_file = sys.argv[1]
    extract_measurements_from_log(log_file)
