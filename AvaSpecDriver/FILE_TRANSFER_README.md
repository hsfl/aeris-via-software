# VIA File Transfer Feature

> **Note:** This document describes the optional file transfer functionality via Serial1 UART.
> The main VIA system uses USB Host for the spectrometer and USB Serial for the command console.
> File transfer via Serial1 is a legacy/optional feature not required for basic operation.

## Overview

This optional implementation adds bidirectional serial communication via UART (Serial1), enabling file transfer from the Teensy 4.1's SD card to a PC.

**Current System (v3.0):**
- Command console via USB Serial (micro-USB to PC)
- Spectrometer control via USB Host (spliced cable)
- File transfer via Python script (using same USB Serial connection)

**This Optional Feature:**
- Additional UART-based file transfer (requires OBCBridge code)
- Not currently enabled in main.cpp

## Features

- **Command Reception**: Teensy listens for commands on UART RX
- **File Listing**: List all files on SD card with sizes
- **File Download**: Transfer files from SD card to PC
- **Hex Encoding**: Reliable binary transfer using hex encoding
- **Progress Tracking**: Real-time progress indicators
- **Error Handling**: Comprehensive error reporting

## Hardware Configuration

- **Serial Port**: Serial1 (UART1 on Teensy 4.1)
- **TX Pin**: Pin 1
- **RX Pin**: Pin 0
- **Baud Rate**: 115200
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)

## Protocol Specification

### Commands (PC → Teensy)

Commands are newline-terminated ASCII strings sent to Serial1:

#### 1. LIST_FILES
Lists all files in the SD card root directory.

**Command Format:**
```
LIST_FILES\n
```

**Response Format:**
```
LIST_START\n
<filename1>,<size1>\n
<filename2>,<size2>\n
...
LIST_END\n
```

**Example:**
```
LIST_START
measurements.txt,12458
spectrum.csv,32768
LOG.TXT,5120
LIST_END
```

#### 2. GET_FILE
Downloads a specific file from the SD card.

**Command Format:**
```
GET_FILE <filename>\n
```

**Response Format:**
```
FILE_START\n
<filename>\n
<size_in_bytes>\n
<hex_encoded_data>
FILE_END\n
```

**Example:**
```
FILE_START
measurements.txt
12458
48656C6C6F20576F726C64...
FILE_END
```

**Notes:**
- File data is hex-encoded (2 hex chars per byte)
- Hex encoding ensures reliable binary transfer over serial
- Progress updates printed to debug Serial (USB)

### Error Responses

If an error occurs, the Teensy sends:
```
ERROR: <error_message>\n
```

Examples:
- `ERROR: SD card not available`
- `ERROR: File not found`
- `ERROR: Unknown command`

## Python Client Usage

The `file_transfer.py` script provides a command-line interface for file operations.

### Requirements

```bash
pip install pyserial
```

### Interactive Mode

Start an interactive session:

```bash
python3 file_transfer.py /dev/ttyACM0
```

Interactive commands:
- `list` - List files on SD card
- `get <filename>` - Download a file
- `quit` - Exit

### Command-Line Mode

**List files:**
```bash
python3 file_transfer.py /dev/ttyACM0 list
```

**Download a file:**
```bash
python3 file_transfer.py /dev/ttyACM0 get measurements.txt
```

**Download with custom output name:**
```bash
python3 file_transfer.py /dev/ttyACM0 get measurements.txt my_data.txt
```

### Windows Usage

On Windows, use COM port instead:
```bash
python file_transfer.py COM3 list
python file_transfer.py COM3 get measurements.txt
```

## Code Structure

### Teensy Firmware Changes

**Modified Files:**

1. **[OBCBridge.h](src/OBCBridge.h)** - Added method declarations:
   - `checkForCommands()` - Poll for incoming commands
   - `transferFile()` - Send file over UART
   - `listFiles()` - List SD card contents
   - `processCommand()` - Parse and execute commands (private)

2. **[OBCBridge.cpp](src/OBCBridge.cpp)** - Implemented file transfer logic:
   - Command parsing and routing
   - SD card file operations
   - Hex encoding for binary data
   - Progress tracking and error handling

3. **[main.cpp](src/main.cpp)** - Added command checking:
   ```cpp
   // Check for incoming commands from PC/OBC
   if (useOBC) {
       obcBridge.checkForCommands();
   }
   ```

**New Files:**

- **[file_transfer.py](file_transfer.py)** - Python client script

## Usage Examples

### Example 1: List Files on SD Card

**Terminal:**
```bash
$ python3 file_transfer.py /dev/ttyACM0 list

✅ Connected to /dev/ttyACM0 at 115200 baud
============================================================
Requesting file list from VIA payload...
============================================================
📤 Sent: LIST_FILES
📥 LIST_START

Files on SD card:
------------------------------------------------------------
  📄 measurements.txt                     12458 bytes
  📄 spectrum.csv                         32768 bytes
  📄 LOG.TXT                               5120 bytes
------------------------------------------------------------
Total files: 3
============================================================
```

**Teensy Debug Output (Serial USB):**
```
RX command: LIST_FILES
Listing SD card files...
──────────────────────────────────────────────
📁 SD Card File Listing:
   measurements.txt (12458 bytes)
   spectrum.csv (32768 bytes)
   LOG.TXT (5120 bytes)
Total files: 3
──────────────────────────────────────────────
```

### Example 2: Download a File

**Terminal:**
```bash
$ python3 file_transfer.py /dev/ttyACM0 get spectrum.csv

✅ Connected to /dev/ttyACM0 at 115200 baud
============================================================
Downloading file: spectrum.csv
============================================================
📤 Sent: GET_FILE spectrum.csv
📥 FILE_START
📥 Filename: spectrum.csv
📥 File size: 32768 bytes

Receiving data...
  Progress: 10% (3276/32768 bytes)
  Progress: 20% (6553/32768 bytes)
  Progress: 30% (9830/32768 bytes)
  ...
  Progress: 100% (32768/32768 bytes)
📥 FILE_END

✅ Received 32768 bytes
💾 Saved to: spectrum.csv
============================================================
```

**Teensy Debug Output (Serial USB):**
```
RX command: GET_FILE spectrum.csv
File transfer request: spectrum.csv
──────────────────────────────────────────────
📁 Transferring file: spectrum.csv
   Size: 32768 bytes
TX: FILE_START
   Progress: 10%
   Progress: 20%
   ...
   Progress: 100%
TX: FILE_END
✅ File transfer complete.
   Total bytes sent: 32768
──────────────────────────────────────────────
```

### Example 3: Interactive Session

```bash
$ python3 file_transfer.py /dev/ttyACM0

✅ Connected to /dev/ttyACM0 at 115200 baud
============================================================
VIA File Transfer - Interactive Mode
============================================================
Commands:
  list              - List files on SD card
  get <filename>    - Download a file
  quit              - Exit
============================================================

VIA> list
[... file listing ...]

VIA> get measurements.txt
[... file download ...]

VIA> quit
Disconnected from serial port
```

## Integration with Existing System

The file transfer feature integrates seamlessly with the existing VIA payload firmware:

1. **Non-blocking**: Command checking happens in main loop without blocking measurements
2. **Independent**: File transfer doesn't interfere with spectrometer operations
3. **Safe**: SD card is properly initialized/checked before each operation
4. **Debug-friendly**: All operations logged to USB Serial for monitoring

## Timing Considerations

- Command checking: ~1ms per loop iteration
- File listing: ~100-500ms depending on file count
- File transfer: ~2-5 seconds for typical VIA data files (depends on size)
- Main measurement loop: Unchanged (100-second cycle)

## Troubleshooting

### Connection Issues

**Problem:** Cannot connect to serial port

**Solutions:**
- Check port name (Linux: `/dev/ttyACM0`, Windows: `COM3`, etc.)
- Verify USB cable is connected
- Check permissions: `sudo chmod 666 /dev/ttyACM0`
- Close other programs using the serial port

### File Transfer Failures

**Problem:** File not found error

**Solutions:**
- Use `list` command to verify filename
- Check filename spelling and case (case-sensitive)
- Ensure file exists on SD card
- Verify SD card is properly inserted

**Problem:** Timeout during transfer

**Solutions:**
- Increase timeout in `file_transfer.py` (line 21)
- Check serial cable quality
- Reduce baud rate if experiencing errors
- Ensure Teensy is powered properly

### Data Corruption

**Problem:** Size mismatch warning

**Solutions:**
- Check serial connection quality
- Verify no electromagnetic interference
- Try downloading again
- Check SD card integrity

## Testing Checklist

- [ ] Connect Teensy to PC via Serial1 (TX/RX pins)
- [ ] Upload firmware with file transfer support
- [ ] Insert SD card with test files
- [ ] Run `list` command - verify files appear
- [ ] Download small file (< 1KB) - verify contents
- [ ] Download large file (> 10KB) - verify contents
- [ ] Test error handling (request non-existent file)
- [ ] Verify main measurement loop still functions
- [ ] Check progress indicators work correctly
- [ ] Test interactive mode

## Future Enhancements

Potential improvements for future versions:

1. **CRC Checksums**: Add data integrity verification
2. **Compression**: Compress files before transfer
3. **Resume Support**: Resume interrupted transfers
4. **Multiple Files**: Download multiple files in one command
5. **File Deletion**: Add command to delete files from SD
6. **Directory Support**: Navigate subdirectories
7. **Upload**: Transfer files from PC to SD card

## Version History

- **v3.0** (2025-11-06): Initial file transfer implementation
  - Added LIST_FILES command
  - Added GET_FILE command
  - Created Python client script
  - Integrated with main loop

## References

- **Main Firmware**: [main.cpp](src/main.cpp)
- **OBC Bridge**: [OBCBridge.h](src/OBCBridge.h), [OBCBridge.cpp](src/OBCBridge.cpp)
- **Logger**: [Logger.h](src/Logger.h), [Logger.cpp](src/Logger.cpp)
- **Platform Config**: [platformio.ini](platformio.ini)
