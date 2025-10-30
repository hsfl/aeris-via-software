# aeris-via-software

## Brief Overview
This is the code repository for the VIA (Visual Imaging and Analysis) Payload firmware for the AERIS mission. The VIA payload utilizes an Avantes AvaSpec-Mini2048CL spectrometer connected to a Teensy 4.1 microcontroller acting as a USB host.

The firmware handles:
- USB communication with the AvaSpec spectrometer using the [USBHost_t36](https://github.com/PaulStoffregen/USBHost_t36) library by Paul Stoffregen
- Measurement data collection (4096 bytes / 2048 pixels)
- SD card logging for data backup
- Internal bridge to Artemis OBC (Raspberry Pi) via UART for radio downlink

## System Architecture

```
AvaSpec Spectrometer (USB)
         ↓
Teensy 4.1 (VIA Payload)
    ├─ SD Card (local logging)
    └─ UART (Serial1) → Artemis OBC (RPi) → RFM23 Radio
```

**Current Version:** V2.0 - UART communication to Artemis OBC established

## AvaSpec Software Build Instructions
1. Download and install VSCode & GitHub Desktop.
* VSCode: https://code.visualstudio.com/download
* GitHub Desktop: https://desktop.github.com/download/
2. Install the both the C/C++ and PlatformIO extension in VSCode.
* PlatformIO: https://platformio.org/install/ide?install=vscode
* C/C++: https://code.visualstudio.com/docs/languages/cpp 
3. Clone the repository from GitHub Desktop by clicking "Add" at the top left, "Clone Repository...", "URL," copying the link below into the prompt, and then clicking Clone.
* Repository URL: https://github.com/hsfl/aeris-via-software.git
4. Go to VSCode and initialize the PlatformIO extension.
5. In the "QUICK ACCESS" column, click on "Open" and then "Open Project" in the tab that opens. Locate and choose the "AvaSpecDriver" folder within the "aeris-via-software" folder.
* This should have opened the AvaSpecDriver folder as a PlatformIO project with all the dependencies and configurations it needs.
6. From the explorer column on the left, navigate the AvaSpecDriver folder to "src", then to "main.cpp".
7. On the bottom left are multiple buttons; click the checkmark to build the code, confirming a successful build when [SUCCESS] appears on the terminal that pops up.
* That finishes building the software.

To get relevant data, continue to "Getting Data" section.

## Getting Data

### Hardware Connections
1. **AvaSpec Spectrometer**: Connect to Teensy 4.1 via USB host port
2. **SD Card**: Insert into Teensy 4.1 built-in SD card slot
3. **OBC Bridge**: Connect Teensy Serial1 (TX=pin 1, RX=pin 0) to Artemis OBC UART at 115200 baud
4. **Power**: Provide 5V power to Teensy 4.1

### Data Flow
Every 100 seconds, the firmware:
1. Collects a full spectrum measurement (2048 pixels)
2. Logs raw hex dump to SD card (`/measurements.txt`)
3. Logs CSV format to SD card (`/spectrum.csv`)
4. Transmits CSV data to Artemis OBC via UART in format:
   ```
   VIA_START
   0,12345
   1,23456
   ...
   2047,45678
   VIA_END
   ```

### Data Output Formats
- **Raw hex**: Logged to `/measurements.txt` on SD card
- **CSV**: Logged to `/spectrum.csv` on SD card (format: `pixel,intensity`)
- **UART**: Transmitted to Artemis OBC for radio downlink

![Example Pixel Spectrum](Example_Pixel_Spectrum.png "Example Spectrum")

## Firmware Modules

### Core Modules
- **main.cpp**: Main firmware entry point with measurement sequencing
- **AvaSpec.h/.cpp**: USB Host driver for AvaSpec spectrometer (commands 0x05, 0x06, 0x0F, 0x13, 0xC0)
- **Logger.h/.cpp**: SD card logging utility for measurements and telemetry
- **OBCBridge.h/.cpp**: Internal bridge for UART communication to Artemis OBC

### Hardware Configuration
- **USB Host**: AvaSpec spectrometer connection
- **Serial (USB)**: Debug output at 115200 baud
- **Serial1 (UART)**: OBC bridge at 115200 baud (TX=1, RX=0)
- **SD Card**: Built-in Teensy 4.1 SD interface (BUILTIN_SDCARD)

## Development Roadmap

### Version History
- **V1.0**: ✅ Standalone testing - USB to spectrometer, SD card logging
- **V2.0**: ✅ Radio link working - UART to Artemis OBC established
- **V3.0**: 🔄 Add RPI node to OBC - RPI connection, sync SD storage, FSM/packet handling
- **V4.0**: 🔄 Full system (AERIS iOBC) - Merge into AERIS FSW, EPS/ADCS integration

### Critical Path
- [x] Create Initial Driver
- [x] Validate Teensy and Spectrometer USB Ports can connect
- [x] Attempt Identification Command & Response to validate communication
- [x] Attempt Measurement Commands and read the "Unsolicitated" data messages
- [x] Read all 10 + 4096 bytes of data from a full measurement
- [x] Graph data by sending to computer Serial Port and graphing through MATLAB
- [x] Write data into SD Card storage
- [x] Implement UART bridge to Artemis OBC for radio relay (V2.0)
- [ ] Define Payload-to-Bus Software ICD fields for packetized CSV transmission
- [ ] Add command handlers for remote "Start/Stop/Config" through spacecraft command bus
- [ ] Integrate RPI connection code on Artemis OBC side
- [ ] Interface into flat-sat testing environment
- [ ] Full integration with Artemis spacecraft bus software