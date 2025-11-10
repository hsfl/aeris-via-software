# VIA Spectrometer Control System

**VIA** - Variability In Atmosphere

Command console interface for the Avantes AvaSpec-Mini2048CL spectrometer controlled via Teensy 4.1.

---

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

**For detailed instructions, see [QUICKSTART.md](QUICKSTART.md)**

---

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

---

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show command reference |
| `measure` | Take single measurement (~15 sec) |
| `identify` | Query device info |
| `auto [sec]` | Start auto mode (default: 100s) |
| `stop` | Stop auto mode |
| `status` | Show system status |
| `sd-on` | Enable SD logging |
| `sd-off` | Disable SD logging |

**Example usage:**
```
VIA> measure              # Single measurement
VIA> auto 30              # Auto mode every 30 seconds
VIA> status               # Check system status
VIA> sd-on                # Enable logging
VIA> stop                 # Stop auto mode
```

---

## Telemetry

### System Status
- **SD Card Status** - ENABLED / DISABLED
- **Auto Mode** - RUNNING / STOPPED
- **Auto Interval** - Seconds between measurements
- **Measurement Count** - Total measurements taken
- **System Uptime** - Time since boot (seconds)

### Spectrum Data
- **Format**: 2048 pixels × 16-bit intensity values
- **Binary size**: 4,106 bytes (10-byte header + 4,096 bytes pixel data)
- **ASCII hex size**: ~12,355 bytes (for SD card logging)
- **CSV format**: Pixel,Intensity

### Data Files (SD Card)
- `meas_0001.txt` - Raw measurement (hex format)
- `spectrum_0001.csv` - Spectrum data (CSV format)
- Files increment automatically with each measurement

---

## Data Capture

### Method 1: Live Capture from Serial (Recommended)

Capture measurement data directly to your computer:

```bash
python3 capture_measurement.py /dev/ttyACM0 spectrum.csv
```

Data is automatically saved to both:
- **Computer** (via USB Serial) → `spectrum.csv`
- **SD Card** (Teensy) → `/spectrum_XXXX.csv`

### Method 2: Download from SD Card

Download previously saved data from SD card:

```bash
# List files on SD card
python3 file_transfer.py /dev/ttyACM0 list

# Download specific file
python3 file_transfer.py /dev/ttyACM0 get spectrum_0001.csv
```

---

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

---

## Project Structure

```
AvaSpecDriver/
├── src/
│   ├── main.cpp              # Command console implementation
│   ├── AvaSpec.h/cpp         # Spectrometer driver
│   ├── Logger.h/cpp          # SD card logging
│   └── OBCBridge.h/cpp       # Optional UART bridge module
├── README.md                 # This file
├── QUICKSTART.md             # Detailed usage guide
├── capture_measurement.py    # Live CSV data capture
├── file_transfer.py          # SD card file download
├── via_monitor.py            # Real-time monitoring
├── via_data_monitor.py       # Data capture with monitoring
└── platformio.ini            # Build configuration
```

---

## Architecture

### Firmware Modules
- **main.cpp** - Command console and measurement sequencing
- **AvaSpec.h/.cpp** - USB Host driver for AvaSpec spectrometer
- **Logger.h/.cpp** - SD card logging utility
- **OBCBridge.h/.cpp** - Optional UART bridge (future use)

### Hardware Configuration
- **USB Host**: AvaSpec spectrometer connection (spliced cable)
- **Serial (USB)**: Command console and debug output (115200 baud)
- **Serial1 (UART)**: Optional OBC bridge (TX=1, RX=0, 115200 baud)
- **SD Card**: Built-in Teensy 4.1 SD interface (BUILTIN_SDCARD)

---

## Version

**v3.0** - Command Console Mode

### Features
- ✅ Interactive command interface via USB Serial
- ✅ Manual and automatic measurement modes
- ✅ Optional SD card logging (CSV + hex dump)
- ✅ Real-time status monitoring
- ✅ Dual CSV output (Serial + SD)
- ✅ Real-time pixel value display during acquisition
- ✅ Python utilities for data capture and monitoring

### Data Budget
- **Current (ASCII hex)**: ~12,355 bytes per measurement
- **Future (raw binary)**: ~4,110 bytes per measurement (67% reduction)

---

## Troubleshooting

### No Serial Port
```bash
# Linux/Mac
ls /dev/ttyACM*
sudo chmod 666 /dev/ttyACM0

# Windows: Check Device Manager
```

### Spectrometer Not Detected
1. Check USB Host cable connections (5V, D+, D–, GND)
2. Verify D+/D– twisted pair integrity
3. Ensure 5V power to spectrometer
4. Re-plug spectrometer
5. Check Serial output for enumeration messages

### SD Card Issues
1. Format as FAT32
2. Check proper insertion
3. Try different SD card
4. Check write-protect tab

### Commands Not Working
1. Verify 115200 baud rate
2. Enable newline terminator in terminal
3. Commands are case-insensitive
4. Type `help` to verify connection

---

## Documentation

- **[QUICKSTART.md](QUICKSTART.md)** - Complete usage guide (start here!)
- **[README.md](README.md)** - This file (overview)
- **Git History** - See commit log for version history

---

**AERIS VIA Payload Team** - Hawaii Space Flight Laboratory - 2025
