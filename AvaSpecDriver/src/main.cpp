/**
 * @file main.cpp
 * @brief VIA spectrometer control with RFM23 radio transmission.
 *
 * This firmware runs on the Teensy 4.1 microcontroller and acts as the
 * controller for the Avantes AvaSpec-Mini2048CL spectrometer, with integrated
 * RFM23 radio transmission capability.
 *
 * Hardware Setup:
 *  1. Connect AvaSpec USB to Teensy 4.1 USB Host (5-pin header)
 *  2. Connect Teensy micro-USB to computer/Pi Zero
 *  3. Insert SD card for data logging
 *  4. Connect RFM23 radio to SPI1:
 *       MISO=39, MOSI=26, SCK=27, CS=38, INT=40
 *       RX_ON=30, TX_ON=31
 *
 * Available Commands:
 *  - help         : Show available commands
 *  - measure      : Take a single measurement
 *  - identify     : Get device identification
 *  - auto [sec]   : Start auto-measurement mode (default: 100 sec interval)
 *  - stop         : Stop auto-measurement mode
 *  - status       : Show system status
 *  - radio        : Send test message (Hello World)
 *  - send         : Measure AND transmit via radio
 *  - radiosend    : Retransmit last measurement
 */

#include <Arduino.h>
#include "USBHost_t36.h"
#include "AvaSpec.h"
#include <SD.h>
#include <Logger.h>
#include <SPI.h>
#include <RH_RF22.h>

// ============================================================================
// RADIO CONFIGURATION (Artemis Kit pinout)
// ============================================================================

#define RFM23_CS    38
#define RFM23_INT   40
#define RFM23_RX_ON 30
#define RFM23_TX_ON 31

// Use SPI1 for radio (separate from USB Host)
// SPI1 pins: MISO=39, MOSI=26, SCK=27
RH_RF22 rf22(RFM23_CS, RFM23_INT, SPI1);

// Radio chunk size (RFM23 FIFO limit is ~64 bytes, use 60 for safety)
#define RADIO_CHUNK_SIZE 60

// ============================================================================
// GLOBALS
// ============================================================================

// Global USB host controller instance
USBHost myusb;

// AvaSpec spectrometer driver instance (attached to the host)
AvaSpec myavaspec(myusb);

// Session flags
bool useSD = false;
bool autoMode = false;
unsigned long autoInterval = 100000;  // Default 100 seconds
unsigned long lastMeasurementTime = 0;
int measurementCount = 0;

// Radio state
bool radioAvailable = false;

// Last measurement data (for radio retransmit)
uint8_t lastMeasurement[4106];
bool hasMeasurement = false;

// ============================================================================
// RADIO FUNCTIONS
// ============================================================================

/**
 * @brief Initialize the RFM23 radio on SPI1
 */
bool initRadio() {
    pinMode(RFM23_RX_ON, OUTPUT);
    pinMode(RFM23_TX_ON, OUTPUT);
    digitalWrite(RFM23_RX_ON, LOW);
    digitalWrite(RFM23_TX_ON, LOW);

    // Configure SPI1 pins
    SPI1.setMISO(39);
    SPI1.setMOSI(26);
    SPI1.setSCK(27);

    Serial.println("Initializing RFM23 radio...");

    if (!rf22.init()) {
        Serial.println("ERROR: RFM23 init failed!");
        return false;
    }

    // Configure for 433 MHz, low data rate (better range)
    rf22.setFrequency(433.0);
    rf22.setModemConfig(RH_RF22::GFSK_Rb2Fd5);  // 2kbps, robust

    Serial.println("RFM23 radio initialized @ 433 MHz");
    return true;
}

/**
 * @brief Send a test message via radio
 */
void radioTest() {
    if (!radioAvailable) {
        Serial.println("ERROR: Radio not available");
        return;
    }

    Serial.println("Sending radio test message...");

    digitalWrite(RFM23_TX_ON, HIGH);
    digitalWrite(RFM23_RX_ON, LOW);

    uint8_t msg[] = "Hello World from VIA!";
    rf22.send(msg, sizeof(msg));
    rf22.waitPacketSent();

    digitalWrite(RFM23_TX_ON, LOW);

    Serial.println("Radio test message sent.");
}

/**
 * @brief Transmit measurement data via radio
 * @param data Pointer to measurement buffer
 * @param len Length of data to transmit
 */
void transmitViaRadio(uint8_t* data, uint16_t len) {
    if (!radioAvailable) {
        Serial.println("ERROR: Radio not available");
        return;
    }

    Serial.println();
    Serial.println("========================================");
    Serial.println("  RADIO TRANSMISSION STARTING");
    Serial.println("========================================");
    Serial.print("Transmitting ");
    Serial.print(len);
    Serial.println(" bytes...");

    digitalWrite(RFM23_TX_ON, HIGH);
    digitalWrite(RFM23_RX_ON, LOW);

    // Send header packet first
    uint8_t header[16];
    snprintf((char*)header, sizeof(header), "VIA:%u:", len);
    rf22.send(header, strlen((char*)header));
    rf22.waitPacketSent();
    delay(50);

    // Send data in chunks
    uint16_t sent = 0;
    uint16_t packetNum = 0;

    while (sent < len) {
        uint16_t chunkSize = min((uint16_t)RADIO_CHUNK_SIZE, (uint16_t)(len - sent));

        rf22.send(data + sent, chunkSize);
        rf22.waitPacketSent();

        sent += chunkSize;
        packetNum++;

        // Progress update every 10 packets
        if (packetNum % 10 == 0 || sent >= len) {
            Serial.print("TX Progress: ");
            Serial.print(sent);
            Serial.print(" / ");
            Serial.print(len);
            Serial.print(" bytes (");
            Serial.print((sent * 100) / len);
            Serial.println("%)");
        }

        delay(30);  // Give receiver time to process
    }

    // Send end marker
    uint8_t footer[] = ":END";
    rf22.send(footer, sizeof(footer) - 1);
    rf22.waitPacketSent();

    digitalWrite(RFM23_TX_ON, LOW);

    Serial.println("========================================");
    Serial.println("  RADIO TRANSMISSION COMPLETE");
    Serial.println("========================================");
    Serial.println();
}

// ============================================================================
// COMMAND FUNCTIONS
// ============================================================================

/**
 * @brief Display help information
 */
void showHelp() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("  AVANTES SPECTROMETER + RADIO COMMAND CONSOLE");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("Spectrometer Commands:");
    Serial.println("  help              - Show this help message");
    Serial.println("  measure           - Take a single measurement");
    Serial.println("  identify          - Query device identification");
    Serial.println("  auto [seconds]    - Start auto mode (default: 100s)");
    Serial.println("  stop              - Stop auto mode");
    Serial.println("  status            - Show system status");
    Serial.println("  sd-on             - Enable SD card logging");
    Serial.println("  sd-off            - Disable SD card logging");
    Serial.println();
    Serial.println("Radio Commands:");
    Serial.println("  radio             - Send test message (Hello World)");
    Serial.println("  send              - Measure AND transmit via radio");
    Serial.println("  radiosend         - Retransmit last measurement");
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
}

/**
 * @brief Show system status
 */
void showStatus() {
    Serial.println();
    Serial.println("────────────────────────────────────────────");
    Serial.println("System Status:");
    Serial.println("────────────────────────────────────────────");
    Serial.print("  SD Card Logging:     ");
    Serial.println(useSD ? "ENABLED" : "DISABLED");
    Serial.print("  Auto Mode:           ");
    Serial.println(autoMode ? "RUNNING" : "STOPPED");
    if (autoMode) {
        Serial.print("  Auto Interval:       ");
        Serial.print(autoInterval / 1000);
        Serial.println(" seconds");
    }
    Serial.print("  Measurements Taken:  ");
    Serial.println(measurementCount);
    Serial.print("  Radio:               ");
    Serial.println(radioAvailable ? "AVAILABLE" : "NOT AVAILABLE");
    Serial.print("  Last Measurement:    ");
    Serial.println(hasMeasurement ? "READY" : "NONE");
    Serial.print("  Uptime:              ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("────────────────────────────────────────────");
    Serial.println();
}

/**
 * @brief Perform a complete measurement sequence
 * @param transmit If true, also transmit via radio after measurement
 */
void performMeasurement(bool transmit = false) {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.print("Starting Measurement #");
    Serial.println(++measurementCount);
    Serial.println("════════════════════════════════════════════════════════");

    // Open log file if SD is enabled
    bool logFileOpen = false;
    if (useSD) {
        char filename[32];
        snprintf(filename, sizeof(filename), "/meas_%04d.txt", measurementCount);
        if (initializeFile(filename)) {
            Serial.print("Logging to: ");
            Serial.println(filename);
            logString("=== Measurement started ===");
            logFileOpen = true;
        } else {
            Serial.println("WARNING: Failed to open log file");
        }
    }

    // Step 0: Stop any ongoing measurement to ensure clean state
    Serial.println("Ensuring device is stopped...");
    myavaspec.stopMeasurement();
    delay(500);

    // Step 1: Query device identification
    Serial.println("Querying device identification...");
    myavaspec.getIdentification();
    delay(1000);

    // Step 2: Prepare measurement parameters
    Serial.println("Preparing measurement parameters...");
    myavaspec.prepareMeasurement();
    delay(1000);

    // Step 3: Start measurement
    Serial.println("Starting measurement...");
    myavaspec.startMeasurement();

    // Step 4: Wait for data acquisition
    Serial.println("Acquiring data...");
    unsigned long startTime = millis();
    while (true) {
        myavaspec.handleUnsolicitatedData();
        delay(100);

        if (millis() - startTime > 10000) {
            Serial.println("Data acquisition complete");
            break;
        }
    }

    // Step 5: Acknowledge and stop
    Serial.println("Acknowledging measurement...");
    myavaspec.measurementAcknowledgement();
    myavaspec.stopMeasurement();

    // Step 6: Copy measurement data for radio
    memcpy(lastMeasurement, myavaspec.getMeasurementBuffer(), 4106);
    hasMeasurement = true;

    // Step 7: Close log file if open
    if (logFileOpen) {
        logString("=== Measurement complete ===");
        closeLogFile();
        Serial.println("Data logged to SD card");
    }

    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("Measurement Complete!");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();

    // Step 8: Transmit if requested
    if (transmit && radioAvailable) {
        transmitViaRadio(lastMeasurement, 4106);
    }
}

/**
 * @brief Process a command string
 */
void processCommand(String cmd) {
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "help" || cmd == "?") {
        showHelp();
    }
    else if (cmd == "measure" || cmd == "m") {
        performMeasurement(false);
    }
    else if (cmd == "send") {
        // Measure and transmit
        performMeasurement(true);
    }
    else if (cmd == "radiosend" || cmd == "rs") {
        // Retransmit last measurement
        if (hasMeasurement) {
            transmitViaRadio(lastMeasurement, 4106);
        } else {
            Serial.println("ERROR: No measurement data available");
            Serial.println("  Run 'measure' first, then 'radiosend'");
        }
    }
    else if (cmd == "radio" || cmd == "r") {
        radioTest();
    }
    else if (cmd == "identify" || cmd == "id") {
        Serial.println("Querying device identification...");
        myavaspec.getIdentification();
        delay(1000);
    }
    else if (cmd.startsWith("auto")) {
        // Parse optional interval
        int spaceIdx = cmd.indexOf(' ');
        if (spaceIdx > 0) {
            String intervalStr = cmd.substring(spaceIdx + 1);
            int seconds = intervalStr.toInt();
            if (seconds > 0) {
                autoInterval = seconds * 1000UL;
            }
        }

        autoMode = true;
        lastMeasurementTime = millis();
        Serial.println();
        Serial.println("Auto-measurement mode STARTED");
        Serial.print("   Interval: ");
        Serial.print(autoInterval / 1000);
        Serial.println(" seconds");
        Serial.println("   Type 'stop' to end auto mode");
        Serial.println();
    }
    else if (cmd == "stop") {
        if (autoMode) {
            autoMode = false;
            Serial.println();
            Serial.println("Auto-measurement mode STOPPED");
            Serial.println();
        } else {
            Serial.println("WARNING: Auto mode is not running");
        }
    }
    else if (cmd == "status") {
        showStatus();
    }
    else if (cmd == "sd-on") {
        if (!useSD) {
            Serial.println("Enabling SD card logging...");
            useSD = true;
        } else {
            Serial.println("SD card logging already enabled");
        }
    }
    else if (cmd == "sd-off") {
        if (useSD) {
            Serial.println("Disabling SD card logging...");
            useSD = false;
        } else {
            Serial.println("SD card logging already disabled");
        }
    }
    else if (cmd.length() > 0) {
        Serial.print("Unknown command: '");
        Serial.print(cmd);
        Serial.println("'");
        Serial.println("   Type 'help' for available commands");
    }
}

// ============================================================================
// SETUP
// ============================================================================

/**
 * @brief Arduino setup routine executed once at boot.
 *
 * Initializes the serial interface, SD card, radio, and USB host controller.
 */
void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for Serial to initialize

    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("  AERIS VIA Spectrometer + Radio Control System");
    Serial.println("  Version 4.0 - Integrated Radio Transmission");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();

    // Initialize SD card
    Serial.println("Initializing SD Card...");
    if (SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD Card initialized");
        useSD = true;
    } else {
        Serial.println("WARNING: SD Card not found - logging disabled");
        useSD = false;
    }

    // Initialize radio
    Serial.println();
    radioAvailable = initRadio();
    if (!radioAvailable) {
        Serial.println("WARNING: Radio not available - transmit commands disabled");
    }

    // Initialize USB Host stack
    Serial.println();
    Serial.println("Initializing USB Host...");
    myusb.begin();
    Serial.println("USB Host controller initialized");
    Serial.println("   Waiting for AvaSpec spectrometer...");

    // Wait a moment for USB enumeration
    delay(2000);

    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("System Ready!");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("Type 'help' for available commands");
    Serial.println();
    Serial.print("VIA> ");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

/**
 * @brief Main control loop.
 *
 * Handles USB host tasks, checks for serial commands, and manages
 * auto-measurement mode.
 */
void loop() {
    // Service the USB host stack (handles enumeration, events, etc.)
    myusb.Task();

    // Check for serial commands
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        Serial.println(command);  // Echo the command
        processCommand(command);

        // Show prompt again if not in auto mode
        if (!autoMode) {
            Serial.print("VIA> ");
        }
    }

    // Handle auto-measurement mode
    if (autoMode) {
        unsigned long currentTime = millis();
        if (currentTime - lastMeasurementTime >= autoInterval) {
            performMeasurement(false);
            lastMeasurementTime = currentTime;

            // Show next measurement countdown
            Serial.print("Next measurement in ");
            Serial.print(autoInterval / 1000);
            Serial.println(" seconds (type 'stop' to end)");
            Serial.println();
        }
    }

    // Small delay to prevent USB bus saturation
    delay(10);
}
