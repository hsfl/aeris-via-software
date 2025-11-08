/**
 * @file main.cpp
 * @brief VIA spectrometer control via USB Host with command console.
 *
 * This firmware runs on the Teensy 4.1 microcontroller and acts as the
 * controller for the Avantes AvaSpec-Mini2048CL spectrometer. It provides
 * interactive command-line control via USB Serial.
 *
 * Hardware Setup:
 *  1. Connect AvaSpec USB to Teensy 4.1 USB Host (5-pin header)
 *  2. Connect Teensy micro-USB to computer
 *  3. Insert SD card for data logging
 *
 * Functional overview:
 *  1. Initialize Serial for command console
 *  2. Initialize SD card for data logging
 *  3. Initialize USB host interface and claim AvaSpec spectrometer
 *  4. Wait for user commands via Serial console
 *  5. Execute commands to control spectrometer
 *
 * Available Commands:
 *  - help         : Show available commands
 *  - measure      : Take a single measurement
 *  - identify     : Get device identification
 *  - auto [sec]   : Start auto-measurement mode (default: 100 sec interval)
 *  - stop         : Stop auto-measurement mode
 *  - status       : Show system status
 */

#include <Arduino.h>
#include "USBHost_t36.h"
#include "AvaSpec.h"
#include <SD.h>
#include <Logger.h>

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

// ============================================================================
// COMMAND FUNCTIONS
// ============================================================================

/**
 * @brief Display help information
 */
void showHelp() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("  AVANTES SPECTROMETER COMMAND CONSOLE");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("Available Commands:");
    Serial.println("  help              - Show this help message");
    Serial.println("  measure           - Take a single measurement");
    Serial.println("  identify          - Query device identification");
    Serial.println("  auto [seconds]    - Start auto mode (default: 100s)");
    Serial.println("  stop              - Stop auto mode");
    Serial.println("  status            - Show system status");
    Serial.println("  sd-on             - Enable SD card logging");
    Serial.println("  sd-off            - Disable SD card logging");
    Serial.println();
    Serial.println("Examples:");
    Serial.println("  measure           - Take one measurement now");
    Serial.println("  auto 60           - Measure every 60 seconds");
    Serial.println("  auto              - Measure every 100 seconds");
    Serial.println("  stop              - Stop automatic measurements");
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
    Serial.print("  Uptime:              ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("────────────────────────────────────────────");
    Serial.println();
}

/**
 * @brief Perform a complete measurement sequence
 */
void performMeasurement() {
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
            Serial.print("📁 Logging to: ");
            Serial.println(filename);
            logString("=== Measurement started ===");
            logFileOpen = true;
        } else {
            Serial.println("⚠️ Failed to open log file");
        }
    }

    // Step 1: Query device identification
    Serial.println("📡 Querying device identification...");
    myavaspec.getIdentification();
    delay(1000);

    // Step 2: Prepare measurement parameters
    Serial.println("⚙️  Preparing measurement parameters...");
    myavaspec.prepareMeasurement();
    delay(1000);

    // Step 3: Start measurement
    Serial.println("🔬 Starting measurement...");
    myavaspec.startMeasurement();

    // Step 4: Wait for data acquisition
    Serial.println("⏳ Acquiring data...");
    unsigned long startTime = millis();
    while (true) {
        myavaspec.handleUnsolicitatedData();
        delay(100);

        if (millis() - startTime > 10000) {
            Serial.println("✅ Data acquisition complete");
            break;
        }
    }

    // Step 5: Acknowledge and stop
    Serial.println("📝 Acknowledging measurement...");
    myavaspec.measurementAcknowledgement();
    myavaspec.stopMeasurement();

    // Step 6: Close log file if open
    if (logFileOpen) {
        logString("=== Measurement complete ===");
        closeLogFile();
        Serial.println("💾 Data logged to SD card");
    }

    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("Measurement Complete!");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
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
        performMeasurement();
    }
    else if (cmd == "identify" || cmd == "id") {
        Serial.println("📡 Querying device identification...");
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
        Serial.println("🔄 Auto-measurement mode STARTED");
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
            Serial.println("⏹  Auto-measurement mode STOPPED");
            Serial.println();
        } else {
            Serial.println("⚠️ Auto mode is not running");
        }
    }
    else if (cmd == "status") {
        showStatus();
    }
    else if (cmd == "sd-on") {
        if (!useSD) {
            Serial.println("💾 Enabling SD card logging...");
            useSD = true;
        } else {
            Serial.println("⚠️ SD card logging already enabled");
        }
    }
    else if (cmd == "sd-off") {
        if (useSD) {
            Serial.println("💾 Disabling SD card logging...");
            useSD = false;
        } else {
            Serial.println("⚠️ SD card logging already disabled");
        }
    }
    else if (cmd.length() > 0) {
        Serial.print("❌ Unknown command: '");
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
 * Initializes the serial interface, SD card, and USB host controller.
 */
void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for Serial to initialize

    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("  AERIS VIA Spectrometer Control System");
    Serial.println("  Version 3.0 - Command Console Mode");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();

    // Initialize SD card
    Serial.println("Initializing SD Card...");
    if (SD.begin(BUILTIN_SDCARD)) {
        Serial.println("✅ SD Card initialized");
        useSD = true;
    } else {
        Serial.println("⚠️ SD Card not found - logging disabled");
        useSD = false;
    }

    // Initialize USB Host stack
    Serial.println();
    Serial.println("Initializing USB Host...");
    myusb.begin();
    Serial.println("✅ USB Host controller initialized");
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
            performMeasurement();
            lastMeasurementTime = currentTime;

            // Show next measurement countdown
            Serial.print("⏰ Next measurement in ");
            Serial.print(autoInterval / 1000);
            Serial.println(" seconds (type 'stop' to end)");
            Serial.println();
        }
    }

    // Small delay to prevent USB bus saturation
    delay(10);
}
