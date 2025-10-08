#ifndef __Logger_h_
#define __Logger_h_

#include <Arduino.h>
#include <SD.h>

/**
 * @file Logger.h
 * @brief Lightweight SD card logging utility for the AERIS payload firmware.
 *
 * This module provides minimal but reliable functions for recording
 * measurement data and diagnostic messages to the Teensy 4.1’s built-in SD card.
 *
 * The intent is to ensure that all mission-critical information (telemetry,
 * sensor readouts, and event traces) can be recovered even if telemetry
 * transmission is interrupted. All writes are immediately flushed to disk to
 * minimize the risk of data loss during resets or power cycling.
 *
 * Files are stored on the Teensy’s internal SD interface (`BUILTIN_SDCARD`).
 * Each log file is opened in append/write mode when initialized, and all
 * functions in this module operate on the same global `logFile` handle.
 *
 * Usage example:
 * @code
 *   initializeFile("VIA_LOG.TXT");
 *   logString("VIA startup complete.");
 *   logMeasurement(dataBuffer, 4106);
 *   closeLogFile();
 * @endcode
 */

/**
 * @brief Global SD file handle shared across all logger functions.
 *
 * Declared here for access from other modules (e.g., AvaSpec.cpp).
 * Initialized in `initializeFile()`.
 */
extern File logFile;

/**
 * @brief Initialize the SD card and open a log file for writing.
 *
 * Must be called once before any logging operation. Attempts to mount the SD
 * card using the built-in SD interface and open or create the given filename.
 *
 * @param filename  Name of the log file (e.g., "LOG.TXT").
 * @return true  if initialization and file opening succeed.
 * @return false if the SD card or file cannot be accessed.
 */
bool initializeFile(const char *filename);

/**
 * @brief Write a binary measurement buffer to the log file as formatted hex.
 *
 * Each byte is written as two-digit hexadecimal text, grouped into lines of 32
 * bytes for readability. Useful for offline parsing and data validation.
 *
 * @param buf Pointer to the buffer containing measurement bytes.
 * @param n   Number of bytes in the buffer.
 */
void logMeasurement(uint8_t* buf, size_t n);

/**
 * @brief Append a plain text string to the log file.
 *
 * Adds a newline automatically and flushes immediately to disk.
 * Typically used for event messages or debug traces.
 *
 * @param text  String object containing the message to write.
 */
void logString(const String &text);

/**
 * @brief Close the currently open log file.
 *
 * Should be called before power-down, reboot, or SD card removal.
 * Ensures that all buffered data is written to the SD card.
 */
void closeLogFile();

#endif // __Logger_h_