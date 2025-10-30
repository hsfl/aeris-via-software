/**
 * @file OBCBridge.h
 * @brief Internal bridge for transmitting VIA payload data to Artemis OBC.
 *
 * This module provides an internal data bridge between the VIA payload Teensy
 * and the Artemis OBC (RPi). Measurement data is transmitted in CSV format
 * over UART for relay by the Artemis radio system.
 */

#ifndef __OBCBridge_h_
#define __OBCBridge_h_

#include <Arduino.h>

/**
 * @class OBCBridge
 * @brief Internal bridge for VIA payload to Artemis OBC data transfer.
 *
 * This class manages serial communication over UART pins, transmitting
 * spectrometer measurement data to the Artemis OBC for radio downlink.
 *
 * Hardware Configuration (Teensy 4.1):
 *  - Serial1 (UART1): TX=1, RX=0
 *  - Baud Rate: 115200
 *  - Data Format: 8N1 (8 data bits, no parity, 1 stop bit)
 *
 * Data Protocol:
 *  - Start marker: "VIA_START\n"
 *  - CSV data: "pixel,intensity\n" for each of 2048 pixels
 *  - End marker: "VIA_END\n"
 *
 * Usage Example:
 * @code
 *   OBCBridge bridge;
 *   bridge.begin();
 *   uint8_t* data = spectrometer.getMeasurementBuffer();
 *   bridge.transmitMeasurement(data);
 * @endcode
 */
class OBCBridge {
public:
    // ──────────────────────────────────────────────
    // Public Interface
    // ──────────────────────────────────────────────

    /**
     * @brief Constructor - initializes bridge parameters.
     */
    OBCBridge();

    /**
     * @brief Initialize UART serial communication.
     *
     * Configures Serial1 (UART1) at 115200 baud for communication with
     * the Artemis OBC. Must be called once during setup.
     *
     * @return true if initialization successful, false otherwise.
     */
    bool begin();

    /**
     * @brief Transmit measurement data as CSV over UART.
     *
     * Converts the 4096 bytes of pixel data (2048 16-bit values) to CSV
     * format and transmits to the Artemis OBC via UART. The OBC will
     * forward this data via the RFM23 radio.
     *
     * Format: "VIA_START\npixel,intensity\n...VIA_END\n"
     *
     * @param measurement Pointer to the 4106-byte measurement buffer
     *                    (10-byte header + 4096 bytes of pixel data).
     * @return true if transmission successful, false otherwise.
     */
    bool transmitMeasurement(uint8_t* measurement);

    /**
     * @brief Send a simple status message via UART.
     *
     * Useful for telemetry or debug messages.
     *
     * @param message String to transmit.
     * @return true if transmission successful, false otherwise.
     */
    bool sendMessage(const char* message);

private:
    // ──────────────────────────────────────────────
    // Configuration Parameters
    // ──────────────────────────────────────────────

    static const unsigned long BAUD_RATE = 115200;  ///< UART baud rate
    static const int HEADER_BYTES        = 10;      ///< AvaSpec header size
    static const int PIXEL_COUNT         = 2048;    ///< Number of 16-bit pixels
};

#endif // __OBCBridge_h_
