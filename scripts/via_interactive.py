#!/usr/bin/env python3
"""
VIA Interactive Console with Automatic Data Logging

Single unified solution that provides:
- Interactive serial console (like screen)
- Automatic session folder creation
- Full debug log
- Clean CSV/TXT extraction per measurement
- Progress bars for data collection (clean output)
- Verbose mode (-v) for full raw data output

Directory structure:
~/Aeris/data/via/YYYYMMDD.HHMM/
├── VIA.YYYYMMDD.HHMM.log          (full session log)
├── VIA.YYYYMMDD.HHMM.SS.csv       (measurement #1 - CSV)
├── VIA.YYYYMMDD.HHMM.SS.txt       (measurement #1 - TXT)
└── ...

Usage:
    python3 via_interactive.py /dev/ttyACM0
    python3 via_interactive.py /dev/ttyACM0 -v    # Verbose mode

Controls:
    Type commands normally (measure, status, help, etc.)
    Type 'sd on' or 'sd off' to toggle SD card reading
    Ctrl+C to exit
"""

import serial
import sys
import select
import termios
import tty
import time
import argparse
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


def print_progress_bar(current, total, label, width=30):
    """Print a progress bar that updates in place"""
    percent = current / total if total > 0 else 0
    filled = int(width * percent)
    bar = '█' * filled + '░' * (width - filled)
    # Clear line and print progress (no newline)
    sys.stdout.write(f"\r  {label}: [{bar}] {current}/{total}    ")
    sys.stdout.flush()


def finish_progress_bar(label, total, actual=None):
    """Complete a progress bar with checkmark"""
    if actual is None:
        actual = total
    width = 30
    bar = '█' * width
    # Clear line completely and print final result
    sys.stdout.write(f"\r  {label}: [{bar}] {actual}/{total} ✓\n")
    sys.stdout.flush()


def interactive_console(port, verbose=False):
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
    print(f"  Verbose:      {'ON' if verbose else 'OFF'}")
    print("═══════════════════════════════════════════════════")
    print()
    print("Auto-saving: Full log + CSV/TXT per measurement")
    print("Type commands normally, Ctrl+C to exit")
    if not verbose:
        print("Use -v flag for full raw data output")
    print()

    # Open serial port
    ser = serial.Serial(port, 115200, timeout=0.1)

    # Open session log
    log_file = open(log_file_path, 'w', buffering=1)

    # Wait briefly for initial boot message and consume it
    time.sleep(0.5)
    if ser.in_waiting > 0:
        boot_data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        log_file.write(boot_data)

    # Data capture state
    in_measurement = False  # Track if we're in a measurement cycle
    measurement_just_ended = False  # Skip separator right after "Measurement Complete!"
    reading_hex = False
    reading_csv = False
    reading_sd = False
    hex_data_txt = []
    hex_data_csv = []
    csv_data = []
    hex_line_count = 0
    csv_line_count = 0
    sd_line_count = 0
    current_csv_file = None
    current_txt_file = None

    # SD card toggle (client-side skip of SD data display)
    sd_enabled = True

    # Expected counts (from VIA protocol)
    EXPECTED_HEX_LINES = 257  # 4106 bytes / 16 bytes per line
    EXPECTED_CSV_LINES = 2048  # 2048 pixels
    EXPECTED_SD_LINES = 2048   # SD card also outputs 2048 lines

    # Save terminal settings
    old_settings = termios.tcgetattr(sys.stdin)

    # Line buffer for processing
    line_buffer = ""

    try:
        # Set terminal to raw mode for character-by-character input
        tty.setraw(sys.stdin.fileno())

        # Show initial prompt (after raw mode is set)
        sys.stdout.write("VIA> ")
        sys.stdout.flush()

        while True:
            # Check for user keyboard input
            if select.select([sys.stdin], [], [], 0)[0]:
                char = sys.stdin.read(1)

                # Ctrl+C to exit
                if ord(char) == 3:  # Ctrl+C
                    break

                # Local echo for typed characters
                if char == '\r':
                    sys.stdout.write('\n')
                elif char == '\x7f':  # Backspace
                    sys.stdout.write('\b \b')
                else:
                    sys.stdout.write(char)
                sys.stdout.flush()

                # Send to serial port
                ser.write(char.encode())

            # Check for serial data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                text = data.decode('utf-8', errors='ignore')

                # Always write to log file (full raw data)
                log_file.write(text)

                # Add to line buffer for processing
                line_buffer += text

                # Process complete lines
                while '\n' in line_buffer:
                    line, line_buffer = line_buffer.split('\n', 1)
                    line_clean = line.strip()

                    # ─────────────────────────────────────────────────────
                    # Section detection and state transitions
                    # ─────────────────────────────────────────────────────

                    # Detect measurement start
                    if "Starting Measurement" in line:
                        in_measurement = True
                        # Use \r to ensure we start at column 0
                        sys.stdout.write(f"\r\n{line_clean}\n")
                        sys.stdout.flush()
                        continue

                    # Detect measurement end
                    if "Measurement Complete" in line:
                        in_measurement = False
                        reading_sd = False
                        measurement_just_ended = True  # Skip the next separator
                        # Use \r to ensure we start at column 0
                        sys.stdout.write(f"\r{line_clean}\n\n")
                        sys.stdout.flush()
                        continue

                    # Start of hex data section
                    if "Reading full 4106-byte measurement" in line:
                        reading_hex = True
                        reading_csv = False
                        reading_sd = False
                        hex_data_txt = []
                        hex_data_csv = []
                        csv_data = []
                        hex_line_count = 0
                        csv_name, txt_name = generate_measurement_filenames(session_timestamp)
                        current_csv_file = session_dir / csv_name
                        current_txt_file = session_dir / txt_name
                        if verbose:
                            sys.stdout.write(f"\r{line_clean}\n")
                            sys.stdout.flush()
                        continue

                    # End of hex data section
                    if "Full 4106 bytes received" in line and reading_hex:
                        if not verbose:
                            finish_progress_bar("Hex data", EXPECTED_HEX_LINES, hex_line_count)
                        reading_hex = False
                        # Save TXT file
                        if hex_data_txt and current_txt_file:
                            with open(current_txt_file, 'w') as f:
                                f.write("===VIA START===\n")
                                f.write('\n'.join(hex_data_txt) + '\n')
                                f.write("===VIA STOP===\n")
                        if verbose:
                            sys.stdout.write(f"\r{line_clean}\n")
                            sys.stdout.flush()
                        continue

                    # Start of CSV data section
                    if "CSV DATA OUTPUT:" in line:
                        reading_csv = True
                        reading_hex = False
                        csv_line_count = 0
                        csv_data = []
                        if verbose:
                            sys.stdout.write(f"\r{line_clean}\n")
                            sys.stdout.flush()
                        continue

                    # End of CSV data section
                    if "END CSV DATA" in line and reading_csv:
                        if not verbose:
                            finish_progress_bar("CSV data", EXPECTED_CSV_LINES, csv_line_count)
                        reading_csv = False
                        # Save CSV file
                        if csv_data and current_csv_file:
                            with open(current_csv_file, 'w') as f:
                                f.write("Pixel,Intensity\n")
                                f.write('\n'.join(csv_data) + '\n')
                        if verbose:
                            sys.stdout.write(f"\r{line_clean}\n")
                            sys.stdout.flush()
                        continue

                    # Start of SD card section (if present)
                    if "SD Card" in line and "DATA" in line.upper():
                        reading_sd = True
                        sd_line_count = 0
                        if sd_enabled and verbose:
                            sys.stdout.write(f"\r{line_clean}\n")
                            sys.stdout.flush()
                        continue

                    # End of SD card section
                    if reading_sd and "END SD" in line.upper():
                        if sd_enabled and not verbose:
                            finish_progress_bar("SD card", sd_line_count if sd_line_count > 0 else EXPECTED_SD_LINES)
                        reading_sd = False
                        if verbose:
                            sys.stdout.write(f"\r{line_clean}\n")
                            sys.stdout.flush()
                        continue

                    # ─────────────────────────────────────────────────────
                    # Data line processing
                    # ─────────────────────────────────────────────────────

                    # Process hex data lines
                    if reading_hex and line_clean:
                        if all(c in '0123456789ABCDEFabcdef ' for c in line_clean):
                            hex_data_txt.append(line_clean)
                            csv_line = line_clean.replace(' ', ',')
                            hex_data_csv.append(csv_line)
                            hex_line_count += 1
                            if verbose:
                                sys.stdout.write(f"\r{line_clean}\n")
                                sys.stdout.flush()
                            else:
                                print_progress_bar(hex_line_count, EXPECTED_HEX_LINES, "Hex data")
                            continue

                    # Process CSV data lines
                    if reading_csv and line_clean:
                        # Skip header line
                        if line_clean == "Pixel,Intensity":
                            if verbose:
                                sys.stdout.write(f"\r{line_clean}\n")
                                sys.stdout.flush()
                            continue
                        # Check if it's a data line (number,number)
                        if ',' in line_clean and line_clean.replace(',', '').replace('-', '').isdigit():
                            csv_data.append(line_clean)
                            csv_line_count += 1
                            if verbose:
                                sys.stdout.write(f"\r{line_clean}\n")
                                sys.stdout.flush()
                            else:
                                print_progress_bar(csv_line_count, EXPECTED_CSV_LINES, "CSV data")
                            continue

                    # Process SD card data lines
                    if reading_sd and line_clean:
                        if ',' in line_clean and line_clean.replace(',', '').replace('-', '').isdigit():
                            sd_line_count += 1
                            if sd_enabled:
                                if verbose:
                                    sys.stdout.write(f"\r{line_clean}\n")
                                    sys.stdout.flush()
                                else:
                                    print_progress_bar(sd_line_count, EXPECTED_SD_LINES, "SD card")
                            continue

                    # ─────────────────────────────────────────────────────
                    # Pass through all other lines (prompts, status, etc.)
                    # ─────────────────────────────────────────────────────

                    # Skip separator line right after "Measurement Complete!"
                    if measurement_just_ended and not verbose:
                        if line_clean.startswith('═') or line_clean == '':
                            measurement_just_ended = False
                            continue
                        measurement_just_ended = False

                    # In non-verbose mode, suppress header separators (════) everywhere
                    # These only appear around measurement start/end
                    if not verbose and line_clean.startswith('═'):
                        continue

                    # In non-verbose mode, suppress intermediate measurement lines
                    if in_measurement and not verbose:
                        # Skip status lines, separators, and empty lines during measurement
                        # Check for emoji prefixes and separator characters
                        first_char = line_clean[0] if line_clean else ''
                        if (line_clean.startswith('─') or
                            first_char in '🛑📡⚙🔬⏳✅💾' or
                            line_clean == ''):
                            continue

                    # Use \r to ensure prompt starts at column 0
                    sys.stdout.write(f"\r{line_clean}\n")
                    sys.stdout.flush()

                # Output any remaining partial line (like "VIA> " prompt)
                if line_buffer and not in_measurement:
                    # Check if it looks like a prompt (ends with > or similar)
                    if line_buffer.rstrip().endswith('>'):
                        sys.stdout.write(f"\r{line_buffer}")
                        sys.stdout.flush()
                        line_buffer = ""

    except KeyboardInterrupt:
        pass

    finally:
        # Restore terminal settings
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
        log_file.close()
        ser.close()
        print("\n\nSession closed")
        print(f"Data saved in: {session_dir}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="VIA Interactive Console with Automatic Data Logging",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 via_interactive.py /dev/ttyACM0           # Normal mode (progress bars)
  python3 via_interactive.py /dev/ttyACM0 -v        # Verbose mode (full raw output)
  python3 via_interactive.py /tmp/ttyVIA0           # Virtual serial port

In-console commands:
  measure      - Take a measurement
  help         - Show available commands
  status       - Show system status
  Ctrl+C       - Exit
        """
    )
    parser.add_argument("port", help="Serial port (e.g., /dev/ttyACM0 or /tmp/ttyVIA0)")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Verbose mode - show full raw data output")

    args = parser.parse_args()
    interactive_console(args.port, verbose=args.verbose)
