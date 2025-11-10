# VIA Spectrometer Quick Start Guide

**VIA** - Variability In Atmosphere Payload for AERIS Mission

Interactive command console for the Avantes AvaSpec-Mini2048CL spectrometer.

---

## Hardware Setup

### Required Components
- Teensy 4.1 microcontroller
- Avantes AvaSpec-Mini2048CL spectrometer
- USB cable (spliced for USB Host connection)
- Micro-USB cable (for serial console)
- SD card (optional, for data logging)

### Connections

```
Computer ←─ micro-USB ─→ Teensy 4.1 ←─ USB Host Pads ─→ AvaSpec
    (Serial Console)      🟥🟩⬜⬛ spliced     (USB-A spliced)
                          5V D+ D– GND
```

**USB Host Wiring:**
- 🟥 Red (5V) → Teensy 5V Host Pad
- 🟩 Green (D+) → Teensy D+ Host Pad
- ⬜ White (D–) → Teensy D– Host Pad
- ⬛ Black (GND) → Teensy GND Host Pad

**Note:** D+/D– twisted pair must be maintained for signal integrity.

---

## Quick Start

### 1. Build and Upload Firmware

```bash
cd AvaSpecDriver
pio run --target upload
```

### 2. Open Serial Console

**Linux/Mac:**
```bash
screen /dev/ttyACM0 115200
```

**Windows:**
- Use PuTTY or Tera Term
- Port: COM3 (or your Teensy port)
- Baud: 115200

### 3. Take a Measurement

```
VIA> measure
```

### 4. Start Automatic Mode

```
VIA> auto 60
```
(Takes measurements every 60 seconds)

### 5. Stop Automatic Mode

```
VIA> stop
```

---

## Available Commands

| Command | Parameters | Description |
|---------|------------|-------------|
| `help` | none | Show command help |
| `measure` | none | Take single measurement (~15 sec) |
| `identify` | none | Query device identification |
| `auto` | [seconds] | Start auto mode (default: 100s) |
| `stop` | none | Stop auto mode |
| `status` | none | Show system status |
| `sd-on` | none | Enable SD card logging |
| `sd-off` | none | Disable SD card logging |

**Examples:**
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
- **Size**: 4,106 bytes raw binary (10-byte header + 4,096 bytes pixel data)
- **Output**: CSV (Serial + SD) and hex dump (SD only)

### Data Files (SD Card)
- `meas_0001.txt` - Raw measurement (hex format, ~12,350 bytes)
- `spectrum_0001.csv` - Spectrum data (Pixel,Intensity format)
- Files increment automatically

---

## Data Capture

### Method 1: Live Capture from Serial

Capture measurement data directly to your computer:

```bash
python3 capture_measurement.py /dev/ttyACM0 spectrum.csv
```

Data is saved to both:
- **Computer** (via USB Serial) → `spectrum.csv`
- **SD Card** (Teensy) → `/spectrum_XXXX.csv`

### Method 2: Download from SD Card

Download previously saved data:

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

## Troubleshooting

### No Serial Port Detected

**Linux/Mac:**
```bash
# Check for Teensy
ls /dev/ttyACM*

# Fix permissions
sudo chmod 666 /dev/ttyACM0

# Or add user to dialout group
sudo usermod -a -G dialout $USER
# (logout/login required)
```

**Windows:**
- Open Device Manager
- Check "Ports (COM & LPT)" section
- Note the COM port number

### Spectrometer Not Detected

1. Check USB Host cable connections
2. Verify D+/D– wires are correctly connected
3. Ensure 5V power to spectrometer
4. Try re-plugging the spectrometer
5. Check Serial output for enumeration messages

### SD Card Errors

1. Verify SD card is FAT32 formatted
2. Check card is properly inserted
3. Try a different SD card
4. Check for write-protect tab

### Commands Not Working

1. Ensure baud rate is 115200
2. Check for newline terminator in terminal
3. Commands are case-insensitive
4. Type `help` to verify connection

---

## Data Budget

**Current system (ASCII hex format):**
- File size: ~12,355 bytes per measurement

**Future optimization (raw binary):**
- File size: ~4,110 bytes per measurement (67% smaller)
- Same data, just not human-readable
- Requires conversion script for viewing

---

**AERIS VIA Payload Team** - 2025
