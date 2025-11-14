#!/usr/bin/env python3
"""
file_transfer.py
--------------------------------------------------
Simple file transfer client for VIA payload data.

This script connects to the Teensy 4.1 via serial port and allows
downloading files from the SD card using a simple command protocol.

Commands:
  - LIST_FILES: List all files on the SD card
  - GET_FILE <filename>: Download a specific file from SD card

Usage:
    python3 file_transfer.py /dev/ttyACM0           # Interactive mode
    python3 file_transfer.py /dev/ttyACM0 list      # List files
    python3 file_transfer.py /dev/ttyACM0 get measurements.txt  # Download file

Author: AERIS VIA Team
Date: 2025
"""

import sys
import serial
import time
from pathlib import Path
from typing import Optional

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------
BAUD_RATE = 115200
TIMEOUT = 5.0  # seconds
DEFAULT_PORT = "/dev/ttyACM0"  # Teensy typically appears here on Linux

# -----------------------------------------------------------------------------
# SERIAL CONNECTION
# -----------------------------------------------------------------------------

class VIAFileTransfer:
    """Handle serial communication and file transfer with VIA payload."""

    def __init__(self, port: str, baud: int = BAUD_RATE):
        """Initialize serial connection to VIA payload.

        Args:
            port: Serial port path (e.g., /dev/ttyACM0 or COM3)
            baud: Baud rate (default: 115200)
        """
        self.port = port
        self.baud = baud
        self.ser: Optional[serial.Serial] = None

    def connect(self) -> bool:
        """Establish serial connection.

        Returns:
            True if connection successful, False otherwise.
        """
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baud,
                timeout=TIMEOUT,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            time.sleep(2)  # Wait for connection to stabilize
            print(f"✅ Connected to {self.port} at {self.baud} baud")

            # Flush any existing data
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()

            return True
        except serial.SerialException as e:
            print(f"❌ Failed to connect to {self.port}: {e}")
            return False

    def disconnect(self):
        """Close serial connection."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Disconnected from serial port")

    def send_command(self, command: str):
        """Send a command to the VIA payload.

        Args:
            command: Command string to send
        """
        if not self.ser or not self.ser.is_open:
            print("❌ Serial port not connected!")
            return

        cmd_bytes = (command + "\n").encode('ascii')
        self.ser.write(cmd_bytes)
        self.ser.flush()
        print(f"📤 Sent: {command}")

    def read_line(self, timeout: Optional[float] = None) -> Optional[str]:
        """Read a line from serial port.

        Args:
            timeout: Optional timeout override

        Returns:
            Line as string, or None if timeout/error
        """
        if not self.ser or not self.ser.is_open:
            return None

        if timeout is not None:
            old_timeout = self.ser.timeout
            self.ser.timeout = timeout

        try:
            line = self.ser.readline().decode('ascii', errors='ignore').strip()
            return line if line else None
        except Exception as e:
            print(f"⚠️ Read error: {e}")
            return None
        finally:
            if timeout is not None:
                self.ser.timeout = old_timeout

    def list_files(self):
        """Request and display list of files on SD card."""
        print("\n" + "="*60)
        print("Requesting file list from VIA payload...")
        print("="*60)

        self.send_command("LIST_FILES")

        # Wait for LIST_START marker
        waiting = True
        start_time = time.time()

        while waiting and (time.time() - start_time) < TIMEOUT:
            line = self.read_line(timeout=1.0)
            if line:
                print(f"📥 {line}")
                if line == "LIST_START":
                    waiting = False
                elif line.startswith("ERROR"):
                    print(f"❌ {line}")
                    return

        if waiting:
            print("⏱ Timeout waiting for file list")
            return

        # Read file list
        print("\nFiles on SD card:")
        print("-" * 60)

        file_count = 0
        while True:
            line = self.read_line(timeout=2.0)
            if not line:
                print("⏱ Timeout reading file list")
                break

            print(f"📥 {line}")

            if line == "LIST_END":
                break
            elif line.startswith("ERROR"):
                print(f"❌ {line}")
                break
            else:
                # Parse filename,size
                try:
                    filename, size = line.split(',')
                    print(f"  📄 {filename:<30} {int(size):>10} bytes")
                    file_count += 1
                except ValueError:
                    pass

        print("-" * 60)
        print(f"Total files: {file_count}")
        print("="*60 + "\n")

    def download_file(self, filename: str, output_path: Optional[str] = None):
        """Download a file from the SD card.

        Args:
            filename: Name of file on SD card to download
            output_path: Local path to save file (default: same as filename)
        """
        if output_path is None:
            output_path = filename

        print("\n" + "="*60)
        print(f"Downloading file: {filename}")
        print("="*60)

        self.send_command(f"GET_FILE {filename}")

        # Wait for FILE_START marker
        waiting = True
        start_time = time.time()

        while waiting and (time.time() - start_time) < TIMEOUT:
            line = self.read_line(timeout=1.0)
            if line:
                print(f"📥 {line}")
                if line == "FILE_START":
                    waiting = False
                elif line.startswith("ERROR"):
                    print(f"❌ {line}")
                    return

        if waiting:
            print("⏱ Timeout waiting for file transfer")
            return

        # Read filename confirmation
        remote_filename = self.read_line(timeout=2.0)
        if not remote_filename:
            print("❌ Failed to receive filename")
            return
        print(f"📥 Filename: {remote_filename}")

        # Read file size
        file_size_str = self.read_line(timeout=2.0)
        if not file_size_str:
            print("❌ Failed to receive file size")
            return

        try:
            file_size = int(file_size_str)
            print(f"📥 File size: {file_size} bytes")
        except ValueError:
            print(f"❌ Invalid file size: {file_size_str}")
            return

        # Read hex-encoded file data
        print("\nReceiving data...")
        hex_data = ""
        bytes_received = 0
        last_progress = 0

        while True:
            line = self.read_line(timeout=10.0)
            if not line:
                print("⏱ Timeout receiving data")
                break

            if line == "FILE_END":
                print(f"📥 {line}")
                break
            elif line.startswith("ERROR"):
                print(f"❌ {line}")
                return
            else:
                # Accumulate hex data
                hex_data += line.replace(" ", "")

                # Calculate progress
                bytes_received = len(hex_data) // 2
                progress = (bytes_received * 100) // file_size if file_size > 0 else 0

                if progress >= last_progress + 10:
                    print(f"  Progress: {progress}% ({bytes_received}/{file_size} bytes)")
                    last_progress = progress

        # Convert hex to binary
        try:
            binary_data = bytes.fromhex(hex_data)
            print(f"\n✅ Received {len(binary_data)} bytes")
        except ValueError as e:
            print(f"❌ Failed to decode hex data: {e}")
            return

        # Verify size
        if len(binary_data) != file_size:
            print(f"⚠️ Warning: Size mismatch! Expected {file_size}, got {len(binary_data)}")

        # Save to file
        try:
            output_file = Path(output_path)
            with open(output_file, 'wb') as f:
                f.write(binary_data)
            print(f"💾 Saved to: {output_file.resolve()}")
            print("="*60 + "\n")
        except IOError as e:
            print(f"❌ Failed to save file: {e}")

    def interactive_mode(self):
        """Run in interactive command mode."""
        print("\n" + "="*60)
        print("VIA File Transfer - Interactive Mode")
        print("="*60)
        print("Commands:")
        print("  list              - List files on SD card")
        print("  get <filename>    - Download a file")
        print("  quit              - Exit")
        print("="*60 + "\n")

        while True:
            try:
                cmd = input("VIA> ").strip()

                if not cmd:
                    continue

                if cmd.lower() in ['quit', 'exit', 'q']:
                    break
                elif cmd.lower() == 'list':
                    self.list_files()
                elif cmd.lower().startswith('get '):
                    filename = cmd[4:].strip()
                    if filename:
                        self.download_file(filename)
                    else:
                        print("❌ Usage: get <filename>")
                else:
                    print(f"❌ Unknown command: {cmd}")
                    print("Commands: list, get <filename>, quit")

            except KeyboardInterrupt:
                print("\nInterrupted")
                break
            except EOFError:
                break

# -----------------------------------------------------------------------------
# MAIN
# -----------------------------------------------------------------------------

def main():
    """Main entry point."""

    # Parse command line arguments
    if len(sys.argv) < 2:
        print("VIA File Transfer Client")
        print("\nUsage:")
        print(f"  {sys.argv[0]} <serial_port>                    # Interactive mode")
        print(f"  {sys.argv[0]} <serial_port> list               # List files")
        print(f"  {sys.argv[0]} <serial_port> get <filename>     # Download file")
        print("\nExamples:")
        print(f"  {sys.argv[0]} /dev/ttyACM0")
        print(f"  {sys.argv[0]} /dev/ttyACM0 list")
        print(f"  {sys.argv[0]} /dev/ttyACM0 get measurements.txt")
        print(f"  {sys.argv[0]} COM3 get spectrum.csv")
        sys.exit(1)

    port = sys.argv[1]

    # Create file transfer client
    client = VIAFileTransfer(port)

    # Connect to device
    if not client.connect():
        sys.exit(1)

    try:
        # Process command
        if len(sys.argv) == 2:
            # Interactive mode
            client.interactive_mode()
        elif sys.argv[2].lower() == 'list':
            # List files
            client.list_files()
        elif sys.argv[2].lower() == 'get' and len(sys.argv) >= 4:
            # Download file
            filename = sys.argv[3]
            output = sys.argv[4] if len(sys.argv) > 4 else None
            client.download_file(filename, output)
        else:
            print("❌ Invalid command")
            print("Use: list, or get <filename>")

    finally:
        # Disconnect
        client.disconnect()

if __name__ == "__main__":
    main()
