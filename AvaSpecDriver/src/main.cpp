/**
 * @file main.cpp
 * @brief VIA payload firmware entry point for the AERIS mission.
 *
 * This firmware runs on the Teensy 4.1 microcontroller and acts as the
 * controller for the Avantes AvaSpec-Mini2048CL spectrometer. It handles
 * initialization, measurement sequencing, and data logging to the onboard SD card.
 *
 * Functional overview:
 *  1. Initialize Serial for debugging output.
 *  2. Initialize SD card and open a measurement log file.
 *  3. Initialize the USB host interface and claim the AvaSpec spectrometer.
 *  4. In each measurement cycle:
 *      - Request device identification (connectivity check).
 *      - Configure acquisition parameters.
 *      - Trigger a single measurement.
 *      - Continuously handle any unsolicited data.
 *      - Log all received data to the SD card.
 *  5. Gracefully stop acquisition, close files, and wait for the next run.
 */

#include <Arduino.h>
#include "USBHost_t36.h"
#include "AvaSpec.h"
#include <iostream>
#include <vector>
#include <SD.h>
#include <Logger.h>

// ============================================================================
// GLOBALS
// ============================================================================

// Global USB host controller instance
USBHost myusb;

// AvaSpec spectrometer driver instance (attached to the host)
AvaSpec myavaspec(myusb);

// Session flags (future use)
bool useSD   = false;
bool useFile = false;

// ============================================================================
// SETUP
// ============================================================================

/**
 * @brief Arduino setup routine executed once at boot.
 *
 * Initializes the serial interface, SD logger, and USB host controller.
 */
void setup() {
    Serial.begin(115200);
    delay(500);  // Small delay to stabilize USB and Serial

    Serial.println();
    Serial.println("────────────────────────────────────────────");
    Serial.println("AERIS VIA Payload Firmware Starting...");
    Serial.println("────────────────────────────────────────────");

    // Initialize SD card logging
    if (initializeFile("/measurements.txt")) {
        Serial.println("✅ SD Card initialized and log file opened.");
        logString("=== VIA session started ===");
    } else {
        Serial.println("⚠️ SD Card initialization failed — proceeding without logging.");
    }

    // Initialize USB Host stack
    myusb.begin();
    Serial.println("✅ USB Host controller initialized.");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

/**
 * @brief Main control loop.
 *
 * Performs a single measurement sequence every 100 seconds.
 * The measurement sequence includes device identification,
 * measurement preparation, data collection, and shutdown.
 */
void loop() {
    // Service the USB host stack (handles enumeration, events, etc.)
    myusb.Task();

    // Short delay for bus stability before issuing commands
    delay(1000);

    // Step 1: Query the AvaSpec device identification block
    myavaspec.getIdentification();
    delay(1000);

    // Step 2: Prepare measurement parameters (integration, pixel range, etc.)
    myavaspec.prepareMeasurement();
    delay(1000);

    // Step 3: Start a single measurement and read back the full data buffer
    myavaspec.startMeasurement();

    // Step 4: Listen for any unsolicited data while measurement completes
    int startTime = millis();
    while (true) {
        myavaspec.handleUnsolicitatedData();  // Poll for incoming data
        delay(100);                           // Prevents USB bus saturation

        if (millis() - startTime > 10000) {   // ~10 seconds window
            Serial.println("⏱ Measurement window elapsed — exiting loop.");
            break;
        }
    }

    // Step 5: Send acknowledgement and stop measurement gracefully
    myavaspec.measurementAcknowledgement();
    myavaspec.stopMeasurement();

    // Step 6: Mark end of session and close the SD file
    logString("=== VIA session complete ===");
    closeLogFile();
    Serial.println("📁 Log file closed and measurement session complete.");

    // Delay before restarting loop or power cycling (100 seconds)
    delay(100000);
}