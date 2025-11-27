#!/usr/bin/env python3
"""
Virtual Serial Port Simulator

Creates a pair of virtual serial ports for testing VIA scripts
without physical hardware.

Usage:
    # Terminal 1: Run the simulator (acts as Teensy)
    python3 virtual_serial_port.py

    # Terminal 2: Run your test script
    python3 ../scripts/via_interactive.py /tmp/ttyVIA0
"""

import os
import pty
import time
import select
import sys
import termios
import tty
from pathlib import Path
from test_data_generator import SpectrometerSimulator


class VirtualTeensy:
    """Simulates Teensy 4.1 running VIA firmware"""

    def __init__(self):
        self.sim = SpectrometerSimulator()
        self.measurement_count = 0
        self.auto_mode = False
        self.auto_interval = 100

    def handle_command(self, command):
        """Process incoming command and return response"""
        cmd = command.strip().lower()

        if cmd in ['help', '?']:
            return self.get_help_text()

        elif cmd == 'measure' or cmd == 'm':
            return self.perform_measurement()

        elif cmd == 'identify' or cmd == 'id':
            return self.get_identification()

        elif cmd == 'status':
            return self.get_status()

        elif cmd.startswith('auto'):
            parts = cmd.split()
            if len(parts) > 1:
                try:
                    self.auto_interval = int(parts[1])
                except ValueError:
                    pass
            self.auto_mode = True
            return (f"\n🔄 Auto-measurement mode STARTED\n"
                   f"   Interval: {self.auto_interval} seconds\n"
                   f"   Type 'stop' to end auto mode\n")

        elif cmd == 'stop':
            if self.auto_mode:
                self.auto_mode = False
                return "\n⏹  Auto-measurement mode STOPPED\n"
            else:
                return "⚠️ Auto mode is not running\n"

        elif cmd:
            return (f"❌ Unknown command: '{cmd}'\n"
                   f"   Type 'help' for available commands\n")

        return ""

    def get_help_text(self):
        """Return help text"""
        return """
════════════════════════════════════════════════════════
  AVANTES SPECTROMETER COMMAND CONSOLE (SIMULATED)
════════════════════════════════════════════════════════

Available Commands:
  help              - Show this help message
  measure           - Take a single measurement
  identify          - Query device identification
  auto [seconds]    - Start auto mode (default: 100s)
  stop              - Stop auto mode
  status            - Show system status

Examples:
  measure           - Take one measurement now
  auto 60           - Measure every 60 seconds
  stop              - Stop automatic measurements

════════════════════════════════════════════════════════
"""

    def get_identification(self):
        """Return simulated device identification"""
        return """
📡 Querying device identification...

Response: get_ident
20 00 5A 00 13 00 41 76 61 53 70 65 63 2D 4D 69
6E 69 32 30 34 38 43 4C 00 00 00 00 00 00 00 00
...
✅ Device: AvaSpec-Mini2048CL (Simulated)
"""

    def get_status(self):
        """Return system status"""
        return f"""
────────────────────────────────────────────
System Status:
────────────────────────────────────────────
  SD Card Logging:     DISABLED (Simulated)
  Auto Mode:           {'RUNNING' if self.auto_mode else 'STOPPED'}
  Auto Interval:       {self.auto_interval} seconds
  Measurements Taken:  {self.measurement_count}
────────────────────────────────────────────
"""

    def perform_measurement(self):
        """Simulate a full measurement cycle"""
        self.measurement_count += 1

        output = []
        output.append("\n════════════════════════════════════════════════════════")
        output.append(f"Starting Measurement #{self.measurement_count}")
        output.append("════════════════════════════════════════════════════════")
        output.append("🛑 Ensuring device is stopped...")
        output.append("📡 Querying device identification...")
        output.append("⚙️  Preparing measurement parameters...")
        output.append("🔬 Starting measurement...")
        output.append("⏳ Acquiring data...")
        output.append("")
        output.append("📡 Reading full 4106-byte measurement...")

        # Generate realistic spectrum data
        intensities = self.sim.generate_gaussian_peak(
            center=1024 + (self.measurement_count * 50) % 500,
            width=100,
            amplitude=45000 + (self.measurement_count * 1000) % 10000
        )

        # Output hex data (header + intensity data)
        # Header (10 bytes)
        header = [0x20, 0x00, 0x0A, 0x10, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00]

        # Convert to bytes
        data_bytes = header.copy()
        for intensity in intensities:
            data_bytes.append(intensity & 0xFF)
            data_bytes.append((intensity >> 8) & 0xFF)

        # Output hex dump (16 bytes per line)
        for i in range(0, len(data_bytes), 16):
            chunk = data_bytes[i:i+16]
            hex_line = ' '.join(f"{b:02X}" for b in chunk)
            output.append(hex_line)

        output.append("✅ Full 4106 bytes received.")
        output.append("")
        output.append("──────────────────────────────────────────────")
        output.append("CSV DATA OUTPUT:")
        output.append("──────────────────────────────────────────────")
        output.append("Pixel,Intensity")

        # Output CSV data
        for pixel, intensity in enumerate(intensities):
            output.append(f"{pixel},{intensity}")

        output.append("──────────────────────────────────────────────")
        output.append("END CSV DATA")
        output.append("──────────────────────────────────────────────")
        output.append("════════════════════════════════════════════════════════")
        output.append("Measurement Complete!")
        output.append("════════════════════════════════════════════════════════")
        output.append("")

        return '\n'.join(output)


def run_virtual_serial():
    """Create virtual serial port pair and simulate Teensy"""

    print("=" * 70)
    print("VIA VIRTUAL SERIAL PORT SIMULATOR")
    print("=" * 70)
    print()
    print("Creating virtual serial port pair...")

    # Create pseudo-terminal pair
    master, slave = pty.openpty()
    slave_name = os.ttyname(slave)

    # Disable echo on the slave side to prevent feedback loop
    attrs = termios.tcgetattr(slave)
    attrs[3] = attrs[3] & ~termios.ECHO  # Disable ECHO flag
    termios.tcsetattr(slave, termios.TCSANOW, attrs)

    # Create symbolic link for easier access
    link_path = "/tmp/ttyVIA0"
    if os.path.exists(link_path):
        os.remove(link_path)
    os.symlink(slave_name, link_path)

    print(f"✅ Virtual port created: {link_path}")
    print(f"   (actual device: {slave_name})")
    print()
    print("To test, run in another terminal:")
    print(f"    python3 scripts/via_interactive.py {link_path}")
    print()
    print("Press Ctrl+C to stop the simulator")
    print("=" * 70)
    print()

    teensy = VirtualTeensy()

    # Send boot message
    boot_msg = """
════════════════════════════════════════════════════════
  AERIS VIA Spectrometer Control System (SIMULATED)
  Version 3.0 - Command Console Mode
════════════════════════════════════════════════════════

Initializing SD Card...
⚠️ SD Card not found - logging disabled

Initializing USB Host...
✅ USB Host controller initialized
   Waiting for AvaSpec spectrometer...

✅ AvaSpec Mini device found and claimed. (Simulated)

════════════════════════════════════════════════════════
System Ready!
════════════════════════════════════════════════════════

Type 'help' for available commands

VIA> """

    os.write(master, boot_msg.encode())

    try:
        buffer = ""
        while True:
            # Check for incoming data
            ready, _, _ = select.select([master], [], [], 0.1)

            if ready:
                try:
                    data = os.read(master, 1024).decode('utf-8', errors='ignore')
                    buffer += data

                    # Process complete commands (lines ending with newline)
                    while '\n' in buffer or '\r' in buffer:
                        # Split on newline
                        if '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                        else:
                            line, buffer = buffer.split('\r', 1)

                        line = line.strip()
                        if line:
                            print(f"[RECV] {line}")

                            # Process command
                            response = teensy.handle_command(line)

                            if response:
                                os.write(master, response.encode())
                                print(f"[SENT] {len(response)} bytes")

                            # Send prompt
                            if not teensy.auto_mode:
                                os.write(master, b"VIA> ")

                except OSError:
                    time.sleep(0.1)

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n\nShutting down virtual serial port...")

    finally:
        os.close(master)
        os.close(slave)
        if os.path.exists(link_path):
            os.remove(link_path)
        print("✅ Cleanup complete")


if __name__ == "__main__":
    run_virtual_serial()
