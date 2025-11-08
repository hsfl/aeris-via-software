/**
 * @file OBCBridge.cpp
 * @brief Implementation of the OBC bridge module.
 *
 * This module handles the internal bridge between the VIA payload Teensy
 * and the Artemis OBC (RPi). Measurement data is formatted as CSV and
 * transmitted over UART for radio relay. Also supports bidirectional
 * communication for file transfer and command processing.
 */

#include "OBCBridge.h"
#include <SD.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

/**
 * @brief Initializes OBC bridge parameters.
 */
OBCBridge::OBCBridge() {
    // Constructor intentionally empty - initialization happens in begin()
}

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * @brief Initialize UART serial communication.
 *
 * Configures Serial1 (UART1 on pins TX=1, RX=0) at 115200 baud for
 * communication with the Artemis OBC.
 *
 * @return true if initialization successful, false otherwise.
 */
bool OBCBridge::begin() {
    Serial.println("Initializing UART communication to Artemis OBC...");

    // Initialize Serial1 (UART1) at 115200 baud
    Serial1.begin(BAUD_RATE);

    // Wait for serial port to stabilize
    delay(100);

    if (!Serial1) {
        Serial.println("❌ UART initialization failed!");
        return false;
    }

    Serial.println("✅ UART initialized at 115200 baud (Serial1, TX=1, RX=0).");
    Serial.println("   Ready to transmit to Artemis OBC.");

    return true;
}

// ============================================================================
// MEASUREMENT TRANSMISSION
// ============================================================================

/**
 * @brief Transmit measurement data as CSV over UART.
 *
 * The 4096 bytes of pixel data (2048 16-bit values) are converted to CSV
 * format and transmitted to the Artemis OBC via UART. The OBC will parse
 * this data and forward it via the RFM23 radio system.
 *
 * Transmission format:
 *   VIA_START
 *   0,12345
 *   1,23456
 *   ...
 *   2047,34567
 *   VIA_END
 *
 * @param measurement Pointer to the 4106-byte measurement buffer
 *                    (10-byte header + 4096 bytes of pixel data).
 * @return true if transmission successful, false otherwise.
 */
bool OBCBridge::transmitMeasurement(uint8_t* measurement) {
    Serial.println("──────────────────────────────────────────────");
    Serial.println("Starting UART transmission to Artemis OBC...");

    if (!Serial1) {
        Serial.println("❌ UART not initialized!");
        return false;
    }

    // Skip the 10-byte AvaSpec header, start at pixel data
    const uint8_t* pixelData = measurement + HEADER_BYTES;

    // Send start marker
    Serial1.println("VIA_START");
    Serial.println("TX: VIA_START");

    // Transmit CSV data (pixel index, intensity value)
    for (int pixelIndex = 0; pixelIndex < PIXEL_COUNT; pixelIndex++) {
        // Read 16-bit pixel value (little-endian byte order)
        uint16_t pixelValue = pixelData[pixelIndex * 2] |
                              (pixelData[pixelIndex * 2 + 1] << 8);

        // Format as "pixel,intensity" and send
        Serial1.print(pixelIndex);
        Serial1.print(",");
        Serial1.println(pixelValue);

        // Progress indicator every 256 pixels
        if (pixelIndex % 256 == 0) {
            Serial.print("TX: Pixel ");
            Serial.print(pixelIndex);
            Serial.print("/");
            Serial.println(PIXEL_COUNT);
        }
    }

    // Send end marker
    Serial1.println("VIA_END");
    Serial.println("TX: VIA_END");

    // Flush to ensure all data is transmitted
    Serial1.flush();

    Serial.println("Measurement transmission complete.");
    Serial.print("Total pixels transmitted: ");
    Serial.println(PIXEL_COUNT);
    Serial.println("──────────────────────────────────────────────");

    return true;
}

// ============================================================================
// SIMPLE MESSAGE TRANSMISSION
// ============================================================================

/**
 * @brief Send a simple text message via UART.
 *
 * Useful for transmitting status messages or telemetry to the Artemis OBC.
 *
 * @param message String to transmit.
 * @return true if transmission successful, false otherwise.
 */
bool OBCBridge::sendMessage(const char* message) {
    if (!Serial1) {
        Serial.println("❌ UART not initialized!");
        return false;
    }

    Serial.print("TX message: ");
    Serial.println(message);

    Serial1.println(message);
    Serial1.flush();

    return true;
}

// ============================================================================
// COMMAND RECEPTION AND PROCESSING
// ============================================================================

/**
 * @brief Check for incoming commands from PC/OBC via UART.
 *
 * Polls Serial1 for incoming data and processes complete command lines.
 * Commands are expected to be newline-terminated strings.
 *
 * @return true if a command was received and processed.
 */
bool OBCBridge::checkForCommands() {
    if (!Serial1) {
        return false;
    }

    // Check if data is available
    if (Serial1.available() > 0) {
        // Read command string until newline
        String cmdString = Serial1.readStringUntil('\n');
        cmdString.trim();  // Remove whitespace

        if (cmdString.length() > 0) {
            Serial.print("RX command: ");
            Serial.println(cmdString);

            // Process the command
            processCommand(cmdString.c_str());
            return true;
        }
    }

    return false;
}

/**
 * @brief Process a received command string.
 *
 * Parses the command and executes the appropriate action.
 *
 * @param cmd Command string to process.
 */
void OBCBridge::processCommand(const char* cmd) {
    String cmdStr(cmd);
    cmdStr.toUpperCase();  // Case-insensitive commands

    if (cmdStr.startsWith("GET_FILE ")) {
        // Extract filename after "GET_FILE "
        String filename = String(cmd).substring(9);
        filename.trim();

        Serial.print("File transfer request: ");
        Serial.println(filename);

        if (!transferFile(filename.c_str())) {
            Serial1.println("ERROR: File transfer failed");
        }
    }
    else if (cmdStr.startsWith("LIST_FILES")) {
        Serial.println("Listing SD card files...");
        if (!listFiles()) {
            Serial1.println("ERROR: Failed to list files");
        }
    }
    else {
        Serial.print("Unknown command: ");
        Serial.println(cmd);
        Serial1.println("ERROR: Unknown command");
    }
}

// ============================================================================
// FILE TRANSFER FUNCTIONS
// ============================================================================

/**
 * @brief Transfer a file from SD card to PC/OBC via UART.
 *
 * Reads the specified file from the SD card and transmits it in chunks
 * over UART with protocol markers for reliable reception.
 *
 * @param filename Name of the file on SD card to transfer.
 * @return true if transfer successful, false otherwise.
 */
bool OBCBridge::transferFile(const char* filename) {
    if (!Serial1) {
        Serial.println("❌ UART not initialized!");
        return false;
    }

    // Check if SD card is available
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("❌ SD Card not available!");
        Serial1.println("ERROR: SD card not available");
        return false;
    }

    // Open the file
    File dataFile = SD.open(filename, FILE_READ);
    if (!dataFile) {
        Serial.print("❌ Failed to open file: ");
        Serial.println(filename);
        Serial1.println("ERROR: File not found");
        return false;
    }

    // Get file size
    size_t fileSize = dataFile.size();

    Serial.println("──────────────────────────────────────────────");
    Serial.print("📁 Transferring file: ");
    Serial.println(filename);
    Serial.print("   Size: ");
    Serial.print(fileSize);
    Serial.println(" bytes");

    // Send file transfer start marker
    Serial1.println("FILE_START");
    Serial1.println(filename);
    Serial1.println(fileSize);

    // Transfer file in chunks
    uint8_t buffer[FILE_CHUNK_SIZE];
    size_t bytesTransferred = 0;
    size_t lastProgress = 0;

    while (dataFile.available()) {
        // Read chunk
        size_t bytesRead = dataFile.read(buffer, FILE_CHUNK_SIZE);

        // Send chunk as hex-encoded data for reliability
        for (size_t i = 0; i < bytesRead; i++) {
            if (buffer[i] < 0x10) Serial1.print("0");
            Serial1.print(buffer[i], HEX);
        }

        bytesTransferred += bytesRead;

        // Progress indicator every 10%
        size_t progress = (bytesTransferred * 100) / fileSize;
        if (progress >= lastProgress + 10) {
            Serial.print("   Progress: ");
            Serial.print(progress);
            Serial.println("%");
            lastProgress = progress;
        }
    }

    // Send end marker
    Serial1.println();
    Serial1.println("FILE_END");
    Serial1.flush();

    // Close file
    dataFile.close();

    Serial.println("✅ File transfer complete.");
    Serial.print("   Total bytes sent: ");
    Serial.println(bytesTransferred);
    Serial.println("──────────────────────────────────────────────");

    return true;
}

/**
 * @brief List all files on the SD card root directory.
 *
 * Sends a list of files with their sizes over UART.
 *
 * @return true if successful, false otherwise.
 */
bool OBCBridge::listFiles() {
    if (!Serial1) {
        Serial.println("❌ UART not initialized!");
        return false;
    }

    // Check if SD card is available
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("❌ SD Card not available!");
        Serial1.println("ERROR: SD card not available");
        return false;
    }

    // Open root directory
    File root = SD.open("/");
    if (!root) {
        Serial.println("❌ Failed to open root directory!");
        Serial1.println("ERROR: Failed to open directory");
        return false;
    }

    Serial.println("──────────────────────────────────────────────");
    Serial.println("📁 SD Card File Listing:");

    // Send list start marker
    Serial1.println("LIST_START");

    int fileCount = 0;

    // Iterate through files
    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String fileName = entry.name();
            size_t fileSize = entry.size();

            // Send to UART
            Serial1.print(fileName);
            Serial1.print(",");
            Serial1.println(fileSize);

            // Print to debug console
            Serial.print("   ");
            Serial.print(fileName);
            Serial.print(" (");
            Serial.print(fileSize);
            Serial.println(" bytes)");

            fileCount++;
        }
        entry.close();
        entry = root.openNextFile();
    }

    // Send list end marker
    Serial1.println("LIST_END");
    Serial1.flush();

    root.close();

    Serial.print("Total files: ");
    Serial.println(fileCount);
    Serial.println("──────────────────────────────────────────────");

    return true;
}
