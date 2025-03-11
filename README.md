# aeris-via-software
## Brief Overview
This is the code repository for the software to utilize the VIA Payload, the Avantes 2048CL/4096CL Spectrometer. 
The Spectrometer has a USB interface which, in the current AERIS design, is utilized using a Teensy 4.1 and the [USBHost_t36](https://github.com/PaulStoffregen/USBHost_t36) library by Paul Stroffregen.
A driver specific to the Spectrometer is needed, and is what enables the command and data communication between the Teensy and the Spectrometer.

(image of set up here)

## AvaSpec Software Build Instructions
1. Download and install VSCode & GitHub Desktop.
* VSCode: https://code.visualstudio.com/download
* GitHub Desktop: https://desktop.github.com/download/
2. Install the PlatformIO extension in VSCode.
* PlatformIO: https://platformio.org/install/ide?install=vscode
3. Clone the repository from GitHub Desktop by clicking "Add" at the top left, "Clone Repository...", and "URL," copying the link below into the prompt, and then clicking Clone.
* Repository URL: https://github.com/hsfl/aeris-via-software.git
4. Go to VSCode and initialize the PlatformIO extension.
5. In the "QUICK ACCESS" column, click on "Open" and then "Open Project" in the tab that opens. Locate and choose the "AvaSpecDriver" folder within the "aeris-via-software" folder.
* This should have opened the AvaSpecDriver folder as a PlatformIO project with all the dependencies and configurations it needs.
6. From the explorer column on the left, navigate the AvaSpecDriver folder to "src", then to "main.cpp".
7. On the bottom left are multiple buttons; click the checkmark to build the code, confirming a successful build when [SUCCESS] appears on the terminal that pops up.
* That finishes building the software.

To get relevant data, continue to "Getting Data" section.

## Getting Data
WIP...

### Critical Path: 
- [x] Create Initial Driver
- [x] Validate Teensy and Spectrometer USB Ports can connect.
- [x] Attempt Identification Command & Response to validate communication.
- [x] Attempt Measurement Commands and read the "Unsolicitated" data messages, validating measurement usage.
- [ ] Be able to read all 10 + 4096 bytes of data from a full measurement.
- [ ] Graph data by sending to computer Serial Port to be copied to .csv/.txt and graphed through Matlab to see the spectrum. 
- [ ] Write data into relevant storage, i.e. SD Card.
- [ ] ...
- [ ] ...
- [ ] Integrate software to Spacecraft Bus Software.
