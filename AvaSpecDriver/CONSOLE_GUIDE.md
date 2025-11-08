# VIA Spectrometer Command Console Guide

## Overview

The VIA spectrometer system provides an interactive command console for controlling the Avantes AvaSpec-Mini2048CL spectrometer through your computer's serial terminal.

## Hardware Setup

### Required Components
- Teensy 4.1 microcontroller
- Avantes AvaSpec-Mini2048CL spectrometer
- USB Host cable (5-pin header to USB-A female)
- Micro-USB cable
- SD card (optional, for data logging)

### Connections

```
┌─────────────────────┐
│  Your Computer      │
│                     │
└──────┬──────────────┘
       │ Micro-USB
       │ (Serial Console)
       │
┌──────▼──────────────┐         ┌──────────────────────┐
│  Teensy 4.1         │         │  AvaSpec             │
│                     │         │  Spectrometer        │
│  [USB Host Header]──┼─────────┤  USB Port            │
│  (5-pin)            │ USB-A   │                      │
│                     │         │                      │
│  [SD Card Slot]     │         └──────────────────────┘
│  (Optional)         │
└─────────────────────┘
```

### Step-by-Step Setup

1. **Insert SD Card** (optional)
   - Format as FAT32
   - Insert into Teensy 4.1 SD card slot

2. **Connect Spectrometer to Teensy**
   - Connect USB Host cable to Teensy 4.1 5-pin header (pins labeled)
   - Connect USB-A end to AvaSpec spectrometer USB port

3. **Connect Teensy to Computer**
   - Use micro-USB cable
   - Connect to Teensy micro-USB port
   - Connect other end to your computer

4. **Upload Firmware**
   ```bash
   cd AvaSpecDriver
   pio run --target upload
   ```

5. **Open Serial Console**
   - Linux/Mac: `screen /dev/ttyACM0 115200`
   - Windows: Use PuTTY or Tera Term on appropriate COM port
   - Or use Arduino Serial Monitor at 115200 baud

## Command Reference

### Available Commands

| Command | Alias | Description | Example |
|---------|-------|-------------|---------|
| `help` | `?` | Show command help | `help` |
| `measure` | `m` | Take single measurement | `measure` |
| `identify` | `id` | Query device info | `identify` |
| `auto [sec]` | - | Start auto mode | `auto 60` |
| `stop` | - | Stop auto mode | `stop` |
| `status` | - | Show system status | `status` |
| `sd-on` | - | Enable SD logging | `sd-on` |
| `sd-off` | - | Disable SD logging | `sd-off` |

### Command Details

#### `help`
Shows the command reference guide.

**Example:**
```
VIA> help

════════════════════════════════════════════════════════
  AVANTES SPECTROMETER COMMAND CONSOLE
════════════════════════════════════════════════════════

Available Commands:
  help              - Show this help message
  measure           - Take a single measurement
  ...
```

#### `measure`
Executes a complete measurement sequence:
1. Query device identification
2. Configure measurement parameters
3. Acquire spectral data
4. Save to SD card (if enabled)

**Example:**
```
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
```

#### `identify`
Queries the spectrometer for device identification information.

**Example:**
```
VIA> identify
📡 Querying device identification...
```

#### `auto [seconds]`
Starts automatic measurement mode with optional interval in seconds (default: 100).

**Examples:**
```
VIA> auto
🔄 Auto-measurement mode STARTED
   Interval: 100 seconds
   Type 'stop' to end auto mode

VIA> auto 30
🔄 Auto-measurement mode STARTED
   Interval: 30 seconds
   Type 'stop' to end auto mode
```

#### `stop`
Stops automatic measurement mode.

**Example:**
```
VIA> stop
⏹  Auto-measurement mode STOPPED
```

#### `status`
Shows current system status including SD card state, auto mode, and measurement count.

**Example:**
```
VIA> status

────────────────────────────────────────────
System Status:
────────────────────────────────────────────
  SD Card Logging:     ENABLED
  Auto Mode:           RUNNING
  Auto Interval:       60 seconds
  Measurements Taken:  5
  Uptime:              342 seconds
────────────────────────────────────────────
```

#### `sd-on` / `sd-off`
Enable or disable SD card logging.

**Examples:**
```
VIA> sd-on
💾 Enabling SD card logging...

VIA> sd-off
💾 Disabling SD card logging...
```

## Usage Scenarios

### Scenario 1: Single Measurement

```
VIA> measure
[... measurement runs ...]
VIA>
```

### Scenario 2: Continuous Monitoring

```
VIA> auto 60
🔄 Auto-measurement mode STARTED
   Interval: 60 seconds

[Measurements run automatically every 60 seconds]

VIA> stop
⏹  Auto-measurement mode STOPPED
```

### Scenario 3: Test Without Logging

```
VIA> sd-off
💾 Disabling SD card logging...

VIA> measure
[... measurement runs without saving to SD ...]

VIA> sd-on
💾 Enabling SD card logging...
```

## Data Files

### File Naming

When SD logging is enabled, measurements are saved with sequential numbering:
- `meas_0001.txt` - First measurement
- `meas_0002.txt` - Second measurement
- `meas_0003.txt` - Third measurement
- etc.

### File Format

Each file contains:
- Measurement header (10 bytes)
- Pixel data (4096 bytes, 2048 pixels × 2 bytes each)
- Logged in both hex and CSV formats

### Accessing Files

Use the file transfer tool to download files:
```bash
python3 file_transfer.py /dev/ttyACM0 list
python3 file_transfer.py /dev/ttyACM0 get meas_0001.txt
```

## Troubleshooting

### No Serial Port Visible

**Linux:**
```bash
# Check if Teensy is detected
ls /dev/ttyACM*

# Add user to dialout group
sudo usermod -a -G dialout $USER
# (logout/login required)

# Or temporarily grant access
sudo chmod 666 /dev/ttyACM0
```

**Windows:**
- Open Device Manager
- Look under "Ports (COM & LPT)"
- Note the COM port number
- Use that in your terminal program

### Spectrometer Not Detected

1. Check USB Host cable connections
2. Verify spectrometer is powered
3. Check Teensy 4.1 USB Host 5V power
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

## Terminal Programs

### Linux/Mac - screen
```bash
screen /dev/ttyACM0 115200
# Exit: Ctrl-A, then K, then Y
```

### Linux/Mac - minicom
```bash
minicom -D /dev/ttyACM0 -b 115200
# Exit: Ctrl-A, then X
```

### Windows - PuTTY
1. Download from putty.org
2. Connection type: Serial
3. Serial line: COM3 (or your port)
4. Speed: 115200
5. Click "Open"

### Arduino IDE
1. Tools → Serial Monitor
2. Set baud rate to 115200
3. Set line ending to "Newline"

### Platform IO
```bash
pio device monitor -b 115200
```

## Tips and Best Practices

1. **Always check status before measurements**
   ```
   VIA> status
   ```

2. **Enable SD logging for data collection**
   ```
   VIA> sd-on
   VIA> measure
   ```

3. **Use auto mode for time-series data**
   ```
   VIA> auto 300    # Every 5 minutes
   ```

4. **Monitor system uptime**
   ```
   VIA> status      # Check uptime and measurement count
   ```

5. **Test identification before measurements**
   ```
   VIA> identify    # Verify spectrometer responds
   VIA> measure
   ```

## Example Session

```
════════════════════════════════════════════════════════
  AERIS VIA Spectrometer Control System
  Version 3.0 - Command Console Mode
════════════════════════════════════════════════════════

Initializing SD Card...
✅ SD Card initialized

Initializing USB Host...
✅ USB Host controller initialized
   Waiting for AvaSpec spectrometer...

════════════════════════════════════════════════════════
System Ready!
════════════════════════════════════════════════════════

Type 'help' for available commands

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
   Type 'stop' to end auto mode

[... automatic measurements continue ...]

VIA> stop
⏹  Auto-measurement mode STOPPED

VIA>
```

## Version History

- **v3.0** (2025-11-06): Command console implementation
  - Added interactive command interface
  - Removed OBC bridge dependencies
  - Simplified control via USB Serial
  - Added auto-measurement mode
  - Dynamic SD logging control

## Support

For issues or questions, check:
- Debug output on USB Serial
- SD card file system
- USB Host enumeration logs
- PlatformIO build output
