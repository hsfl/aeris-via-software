# VIA Spectrometer Control System

**VIA** - Variability In Atmosphere

Command console interface for the Avantes AvaSpec-Mini2048CL spectrometer controlled via Teensy 4.1.

## Quick Start

```bash
# 1. Build and upload firmware
cd AvaSpecDriver
pio run --target upload

# 2. Connect to serial console
screen /dev/ttyACM0 115200

# 3. Take a measurement
VIA> measure
```

## Hardware Setup

The AvaSpec USB cable is spliced and connected to Teensy USB Host pads:

```
Computer ←─ micro-USB ─→ Teensy 4.1 ←─ USB Host Pads ─→ AvaSpec
    (Serial Console)      🟥🟩⬜⬛ spliced     (USB-A spliced)
                          5V D+ D– GND
```

**USB Connection:**
- 🟥 Red (5V) → Teensy 5V Host Pad
- 🟩 Green (D+) → Teensy D+ Host Pad
- ⬜ White (D–) → Teensy D– Host Pad
- ⬛ Black (GND) → Teensy GND Host Pad

D+/D– twisted pair maintained for signal integrity.

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show command reference |
| `measure` | Take single measurement |
| `identify` | Query device info |
| `auto [sec]` | Start auto mode |
| `stop` | Stop auto mode |
| `status` | Show system status |
| `sd-on` | Enable SD logging |
| `sd-off` | Disable SD logging |

## Telemetry

- **SD Card Status** - Logging enabled/disabled
- **Auto Mode** - Running/stopped
- **Measurement Count** - Total measurements taken
- **System Uptime** - Time since boot
- **Spectrum Data** - 2048 pixels (CSV/hex format)

## Documentation

- **[VIA_SIMPLE_REF.txt](VIA_SIMPLE_REF.txt)** - One-page quick reference
- **[VIA_COMMANDS.txt](VIA_COMMANDS.txt)** - Complete command list
- **[VIA_TELEMETRY.txt](VIA_TELEMETRY.txt)** - Telemetry details
- **[CONSOLE_GUIDE.md](CONSOLE_GUIDE.md)** - Full user guide
- **[QUICK_REF.txt](QUICK_REF.txt)** - Quick reference card

## Data Capture

### Method 1: Capture Live from Serial (Recommended)

Capture measurement data directly to your computer:

```bash
# Capture measurement to CSV file
python3 capture_measurement.py /dev/ttyACM0 spectrum.csv
```

The data is automatically saved to both:
- **Computer** (via USB Serial) → `spectrum.csv`
- **SD Card** (Teensy) → `/spectrum_XXXX.csv`

### Method 2: Download from SD Card

Download previously saved data from SD card:

```bash
# List files on SD card
python3 file_transfer.py /dev/ttyACM0 list

# Download specific spectrum file
python3 file_transfer.py /dev/ttyACM0 get spectrum_0001.csv
```

## Example Session

```
════════════════════════════════════════════════════════
  AERIS VIA Spectrometer Control System
  Version 3.0 - Command Console Mode
════════════════════════════════════════════════════════

VIA> status
────────────────────────────────────────────
System Status:
────────────────────────────────────────────
  SD Card Logging:     ENABLED
  Auto Mode:           STOPPED
  Measurements Taken:  0
  Uptime:              5 seconds
────────────────────────────────────────────

VIA> measure

════════════════════════════════════════════════════════
Starting Measurement #1
════════════════════════════════════════════════════════
📁 Logging to: /meas_0001.txt
📡 Querying device identification...
⚙️  Preparing measurement parameters...
🔬 Starting measurement...
⏳ Acquiring data...
✅ Data acquisition complete
📝 Acknowledging measurement...
💾 Data logged to SD card
════════════════════════════════════════════════════════
Measurement Complete!
════════════════════════════════════════════════════════

VIA> auto 30
🔄 Auto-measurement mode STARTED
   Interval: 30 seconds

VIA> stop
⏹  Auto-measurement mode STOPPED
```

## Project Structure

```
AvaSpecDriver/
├── src/
│   ├── main.cpp              # Command console implementation
│   ├── AvaSpec.h/cpp         # Spectrometer driver
│   ├── OBCBridge.h/cpp       # UART bridge module
│   └── Logger.h/cpp          # SD card logging
├── capture_measurement.py    # Capture live CSV data (NEW)
├── file_transfer.py          # Download from SD card
├── VIA_SIMPLE_REF.txt        # Quick reference (START HERE)
├── VIA_COMMANDS.txt          # Command list
├── VIA_TELEMETRY.txt         # Telemetry list
├── CONSOLE_GUIDE.md          # Complete guide
└── README.md                 # This file
```

## Version

**v3.0** - Command Console Mode
- Interactive command interface via USB Serial
- Manual and automatic measurement modes
- Optional SD card logging
- Real-time status monitoring

## Support

For issues:
1. Check [VIA_SIMPLE_REF.txt](VIA_SIMPLE_REF.txt) for quick help
2. Review [CONSOLE_GUIDE.md](CONSOLE_GUIDE.md) for detailed instructions
3. Monitor USB Serial output for debug messages
4. Verify hardware connections

---

**AERIS VIA Payload Team** - 2025
