/**
 * @file OBCBridge.cpp
 * @brief Implementation of the OBC bridge module.
 *
 * This module handles the internal bridge between the VIA payload Teensy
 * and the Artemis OBC (RPi). Measurement data is formatted as CSV and
 * transmitted over UART for radio relay.
 */

#include "OBCBridge.h"

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
