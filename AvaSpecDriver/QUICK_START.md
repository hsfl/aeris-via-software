# VIA File Transfer - Quick Start Guide

## Installation

### 1. Install Python Dependencies

```bash
pip install pyserial
```

### 2. Upload Firmware to Teensy

Use PlatformIO to build and upload:

```bash
cd AvaSpecDriver
pio run --target upload
```

Or use the PlatformIO extension in your IDE.

### 3. Connect Hardware

**Serial Connection (for file transfer):**
- Connect Teensy TX (Pin 1) to USB-Serial adapter RX
- Connect Teensy RX (Pin 0) to USB-Serial adapter TX
- Connect GND to GND

**Or use Teensy's USB serial directly:**
- The implementation uses Serial1 (hardware UART)
- For testing, you can connect pins 0↔1 on the same Teensy
- Then use the USB serial port for both debug and commands

**SD Card:**
- Insert SD card with FAT32 formatting
- VIA will automatically create measurement files

## Basic Usage

### List Files

```bash
python3 file_transfer.py /dev/ttyACM0 list
```

**Expected Output:**
```
✅ Connected to /dev/ttyACM0 at 115200 baud
============================================================
Requesting file list from VIA payload...
============================================================

Files on SD card:
------------------------------------------------------------
  📄 measurements.txt                     12458 bytes
  📄 spectrum.csv                         32768 bytes
------------------------------------------------------------
Total files: 2
```

### Download a File

```bash
python3 file_transfer.py /dev/ttyACM0 get measurements.txt
```

**Expected Output:**
```
✅ Connected to /dev/ttyACM0 at 115200 baud
============================================================
Downloading file: measurements.txt
============================================================

Receiving data...
  Progress: 10% (1245/12458 bytes)
  Progress: 20% (2491/12458 bytes)
  ...
  Progress: 100% (12458/12458 bytes)

✅ Received 12458 bytes
💾 Saved to: measurements.txt
```

### Interactive Mode

```bash
python3 file_transfer.py /dev/ttyACM0
```

**Session Example:**
```
VIA> list
[... file listing ...]

VIA> get spectrum.csv
[... downloads file ...]

VIA> quit
```

## Quick Test

Run the automated test script:

```bash
./test_file_transfer.sh /dev/ttyACM0
```

This will:
1. Check dependencies
2. List files on the SD card
3. Prompt to download a file
4. Display results

## Troubleshooting

### "Permission denied" on /dev/ttyACM0

```bash
sudo chmod 666 /dev/ttyACM0
# or add yourself to dialout group:
sudo usermod -a -G dialout $USER
# then logout and login again
```

### "Port not found"

Check available ports:
```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

On Windows, use Device Manager to find COM port number, then:
```bash
python file_transfer.py COM3 list
```

### "No response from device"

1. Verify firmware is uploaded (check USB Serial debug output)
2. Ensure SD card is inserted
3. Check UART connections (TX↔RX, GND)
4. Verify correct port and baud rate (115200)

### File not found

1. List files first: `python3 file_transfer.py /dev/ttyACM0 list`
2. Check filename spelling (case-sensitive!)
3. Verify SD card has files
4. Check SD card formatting (FAT32)

## Protocol Reference

### Supported Commands

| Command | Description | Response |
|---------|-------------|----------|
| `LIST_FILES` | List all files | `LIST_START`, files, `LIST_END` |
| `GET_FILE <name>` | Download file | `FILE_START`, data, `FILE_END` |

### Response Format

**LIST_FILES:**
```
LIST_START
filename1,size1
filename2,size2
LIST_END
```

**GET_FILE:**
```
FILE_START
<filename>
<size_in_bytes>
<hex_encoded_data>
FILE_END
```

**Errors:**
```
ERROR: <error_message>
```

## Code Structure

```
AvaSpecDriver/
├── src/
│   ├── main.cpp              # Main loop (command checking added)
│   ├── OBCBridge.h           # File transfer API declarations
│   ├── OBCBridge.cpp         # File transfer implementation
│   ├── AvaSpec.h/cpp         # Spectrometer driver (unchanged)
│   └── Logger.h/cpp          # SD logging (unchanged)
├── file_transfer.py          # Python client
├── test_file_transfer.sh     # Test script
├── FILE_TRANSFER_README.md   # Full documentation
└── QUICK_START.md            # This file
```

## Next Steps

1. **Test with real data**: Run a measurement cycle and download the results
2. **Integrate with workflow**: Add file transfer to your data pipeline
3. **Automate**: Create scripts to automatically download files after measurements
4. **Monitor**: Use USB Serial to watch debug output during transfers

## Examples

### Download All Files

```bash
# List files and extract filenames
FILES=$(python3 file_transfer.py /dev/ttyACM0 list | grep "📄" | awk '{print $2}')

# Download each file
for file in $FILES; do
    python3 file_transfer.py /dev/ttyACM0 get "$file"
done
```

### Verify Downloaded File

```bash
# Download spectrum.csv
python3 file_transfer.py /dev/ttyACM0 get spectrum.csv

# Check file size
ls -lh spectrum.csv

# View first few lines
head -20 spectrum.csv

# Plot the data (if you have the plotter)
python3 SerialRead.py spectrum.csv
```

### Continuous Monitoring

```bash
# Keep downloading new measurements
while true; do
    sleep 100  # Wait for measurement cycle
    python3 file_transfer.py /dev/ttyACM0 get measurements.txt \
        "measurements_$(date +%Y%m%d_%H%M%S).txt"
done
```

## Support

For issues or questions:
- Check [FILE_TRANSFER_README.md](FILE_TRANSFER_README.md) for detailed documentation
- Review code comments in [OBCBridge.cpp](src/OBCBridge.cpp)
- Enable debug output on USB Serial to monitor Teensy operations

---

**Version 3.0** - File Transfer Implementation
AERIS VIA Payload Team - 2025
