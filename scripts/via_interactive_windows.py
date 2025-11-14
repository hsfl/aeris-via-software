#!/usr/bin/env python3
"""
VIA Interactive Console with Automatic Data Logging (Windows Version)

Single unified solution that provides:
- Interactive serial console (like screen)
- Automatic session folder creation
- Full debug log
- Clean CSV/TXT extraction per measurement

Directory structure:
~/via_outputlogs/YYYYMMDD.HHMM/
├── VIA.YYYYMMDD.HHMM.log          (full session log)
├── VIA.YYYYMMDD.HHMM.SS.csv       (measurement #1 - CSV)
├── VIA.YYYYMMDD.HHMM.SS.txt       (measurement #1 - TXT)
└── ...

Usage:
    python via_interactive_windows.py COM3

Controls:
    Type commands normally (VIA_MEASURE, VIA_STATUS, etc.)
    Ctrl+C to exit
"""

import serial
import serial.tools.list_ports
import sys
import msvcrt
import threading
from datetime import datetime
from pathlib import Path

def create_session_directory():
    """Create timestamped session directory"""
    base_dir = Path.home() / "via_outputlogs"
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
    print("  VIA Interactive Console (Windows)")
    print("═══════════════════════════════════════════════════")
    print(f"  Port:         {port}")
    print(f"  Session dir:  {session_dir}")
    print(f"  Log file:     {log_filename}")
    print("═══════════════════════════════════════════════════")
    print()
    print("Auto-saving: Full log + CSV/TXT per measurement")
    print("Type commands normally, Ctrl+C to exit")
    print()

    # Open serial port
    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
    except serial.SerialException as e:
        print(f"ERROR: Could not open {port}: {e}")
        print("\nAvailable COM ports:")
        ports = serial.tools.list_ports.comports()
        for p in ports:
            print(f"  - {p.device}: {p.description}")
        sys.exit(1)

    # Open session log
    log_file = open(log_file_path, 'w', buffering=1)

    # Data capture state
    reading_data = False
    hex_data_txt = []
    hex_data_csv = []
    current_csv_file = None
    current_txt_file = None

    # Flag to control threads
    running = True

    def read_serial():
        """Thread to read from serial port"""
        nonlocal reading_data, hex_data_txt, hex_data_csv, current_csv_file, current_txt_file

        while running:
            try:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)

                    # Try to decode as UTF-8, but handle binary data intelligently
                    try:
                        text = data.decode('utf-8')
                        display_text = text
                    except UnicodeDecodeError:
                        # If it's not valid UTF-8, it might be binary data
                        # Try latin-1 which maps bytes 1:1 to characters
                        text = data.decode('latin-1')
                        # For display, only show printable characters
                        display_text = ''.join(c if c.isprintable() or c in '\n\r\t' else '.' for c in text)

                    # Echo to console (handle Windows console encoding issues)
                    try:
                        sys.stdout.write(display_text)
                        sys.stdout.flush()
                    except (UnicodeEncodeError, OSError):
                        # Last resort: ASCII-only with replacement
                        safe_text = display_text.encode('ascii', errors='replace').decode('ascii')
                        sys.stdout.write(safe_text)
                        sys.stdout.flush()

                    # Write to log file (preserve original text, handle encoding)
                    try:
                        log_file.write(text)
                        log_file.flush()
                    except (UnicodeEncodeError, OSError):
                        # If log fails, write ASCII version
                        safe_log = text.encode('ascii', errors='replace').decode('ascii')
                        log_file.write(safe_log)
                        log_file.flush()

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
                            # Extract only hex data (ignore chunk progress messages)
                            hex_parts = []
                            for word in line.split():
                                # Check if this word is a hex byte (2 chars, all hex)
                                if len(word) == 2 and all(c in '0123456789ABCDEFabcdef' for c in word):
                                    hex_parts.append(word)

                            if hex_parts:
                                hex_line = ' '.join(hex_parts)
                                hex_data_txt.append(hex_line)
                                csv_line = ','.join(hex_parts)
                                hex_data_csv.append(csv_line)
            except Exception as e:
                # Log the error but continue
                if running:
                    try:
                        print(f"\nERROR: Serial read error: {e}")
                    except:
                        pass
                # Don't break - keep reading

    # Start serial reading thread
    serial_thread = threading.Thread(target=read_serial, daemon=True)
    serial_thread.start()

    try:
        while True:
            # Check for keyboard input (non-blocking on Windows)
            if msvcrt.kbhit():
                char = msvcrt.getch()

                # Ctrl+C to exit
                if char == b'\x03':  # Ctrl+C
                    break

                # Handle Enter key
                if char == b'\r':
                    ser.write(b'\r\n')
                    sys.stdout.write('\n')
                    sys.stdout.flush()
                else:
                    # Send to serial port
                    ser.write(char)
                    # Echo locally (some terminals don't echo in raw mode)
                    try:
                        sys.stdout.write(char.decode('utf-8', errors='ignore'))
                        sys.stdout.flush()
                    except:
                        pass

    except KeyboardInterrupt:
        pass

    finally:
        running = False
        serial_thread.join(timeout=1)
        log_file.close()
        ser.close()
        print("\n\nSession closed")
        print(f"Data saved in: {session_dir}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python via_interactive_windows.py <port>")
        print("\nExample:")
        print("  python via_interactive_windows.py COM3")
        print("\nTo find your COM port, check Device Manager > Ports (COM & LPT)")
        sys.exit(1)

    port = sys.argv[1]
    interactive_console(port)
