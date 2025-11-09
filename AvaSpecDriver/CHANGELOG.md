# VIA Spectrometer Changelog

## [Unreleased] - 2025-11-08

### Added
- **Real-time pixel value display** during measurement data collection
  - Shows `Pixel N: value` for each pixel as chunks are received
  - Displays all 2048 pixels with their intensity values during acquisition

- **Dual-output CSV data** - Measurements now save to BOTH:
  - **USB Serial** - Full CSV output for live capture on computer
  - **SD Card** - Automatic backup to `/spectrum_XXXX.csv`
  - Works even if SD card is not available (Serial output continues)

- **Live data capture script** - `capture_measurement.py`
  - Sends `VIA_MEASURE` command automatically
  - Captures CSV data from Serial to computer file
  - Example: `python3 capture_measurement.py /dev/ttyACM0 spectrum.csv`

### Removed
- **Timeout warning messages** from AvaSpec commands
  - Removed "❌ No response received (timeout)" from `getIdentification()`
  - Removed "❌ No response received (timeout)" from `prepareMeasurement()`
  - Removed "⚠️ No ACK received, continuing to data read..." from `startMeasurement()`
  - Removed "❌ No response (timeout)" from `stopMeasurement()`
  - Device works correctly even without explicit responses

### Technical Details

**CSV Output Format:**
```
──────────────────────────────────────────────
CSV DATA OUTPUT:
──────────────────────────────────────────────
Pixel,Intensity
0,12543
1,12589
2,12601
...
2047,13845
──────────────────────────────────────────────
END CSV DATA
──────────────────────────────────────────────
```

**Pixel Display During Collection:**
```
Chunk received: 512 / 4106 bytes
  Pixel 0: 12543
  Pixel 1: 12589
  Pixel 2: 12601
  ...
  Pixel 251: 13042
Chunk received: 1024 / 4106 bytes
  Pixel 252: 13098
  ...
```

**Why Remove Timeouts?**
The AvaSpec-Mini2048CL doesn't always send explicit ACK responses, but commands execute successfully. The timeout messages were misleading and have been removed. The device still works correctly.

---

## [3.0] - 2025-11-06

### Added
- Command console interface via USB Serial
- Interactive VIA commands (VIA_MEASURE, VIA_STATUS, VIA_IDENTIFY, etc.)
- Automatic measurement mode with configurable intervals
- SD card data logging (CSV + raw hex)
- File transfer protocol (GET_FILE, LIST_FILES)

### Features
- USB Host communication with AvaSpec-Mini2048CL
- 4106-byte full-frame spectral data readout
- Real-time debug output
- SD card CSV export of spectrum data

---

**AERIS VIA Payload Team** - 2025
