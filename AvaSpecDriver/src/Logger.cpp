/**
 * @file Logger.cpp
 * @brief Implementation of the SD card logging module.
 *
 * This module provides functions for writing measurement data and status
 * messages to the Teensy 4.1's built-in SD card. All logging operations
 * are immediately flushed to ensure data integrity during power cycles.
 */

#include <Logger.h>
#include <Arduino.h>

File logFile;  ///< Global file handle used for all logging operations

// ============================================================================
// LOGGER INITIALIZATION
// ============================================================================

/**
 * @brief Initializes the SD card and opens a log file for writing.
 *
 * This function must be called once during system startup before any
 * measurement data or text logging occurs. It uses the Teensy 4.1’s
 * built-in microSD interface (BUILTIN_SDCARD) and attempts to open the
 * specified file in append/write mode.
 *
 * @param filename The name of the log file to open (e.g. "VIA_LOG.TXT").
 * @return true  if initialization and file open succeed.
 * @return false if the SD card or file cannot be accessed.
 */
bool initializeFile(const char *filename) {
    // Attempt to mount the SD card via the built-in SPI interface.
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("❌ SD Card failed to initialize!");
        return false;
    }

    // Attempt to open or create the log file.
    logFile = SD.open(filename, FILE_WRITE);

    if (!logFile) {
        Serial.println("❌ Failed to open log file for writing!");
        return false;
    }

    Serial.print("✅ Log file opened: ");
    Serial.println(filename);
    return true;
}

// ============================================================================
// MEASUREMENT LOGGING
// ============================================================================

/**
 * @brief Writes a raw measurement buffer to the SD log file in hex format.
 *
 * Each line contains 32 bytes for readability. Two-digit zero-padded
 * hexadecimal values are separated by spaces. This representation makes
 * it possible to visually inspect the data or re-parse it later using
 * standard hex-to-binary conversion tools.
 *
 * @param buf Pointer to the byte buffer to write.
 * @param n   Number of bytes in the buffer.
 */
void logMeasurement(uint8_t* buf, size_t n) {
    if (!logFile) {
        Serial.println("⚠️ logMeasurement() called before file initialization.");
        return;
    }

    for (size_t i = 0; i < n; i++) {
        // Start a new line every 32 bytes for visual alignment
        if (i % 32 == 0) logFile.println();

        // Always print two hexadecimal digits per byte
        if (buf[i] < 0x10) logFile.print("0");
        logFile.print(buf[i], HEX);
        logFile.print(" ");
    }

    logFile.println();   // Final newline after the dump
    logFile.flush();     // Ensure data is committed to SD card immediately

    Serial.print("💾 Logged ");
    Serial.print(n);
    Serial.println(" bytes to SD.");
}

// ============================================================================
// STRING LOGGING
// ============================================================================

/**
 * @brief Writes a plain text string to the log file.
 *
 * Useful for timestamped messages, event notices, or debug traces.
 * Each call appends a newline automatically.
 *
 * @param text The string to be written to the file.
 */
void logString(const String &text) {
    if (!logFile) {
        Serial.println("⚠️ logString() called before file initialization.");
        return;
    }

    logFile.println(text);
    logFile.flush();  // Flush immediately to minimize data loss on reset
}

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

/**
 * @brief Closes the active log file.
 *
 * Always call this before power-down or SD card removal to ensure that
 * file buffers are safely written.
 */
void closeLogFile() {
    if (!logFile) return;

    logFile.close();
    Serial.println("📁 Log file closed.");
}