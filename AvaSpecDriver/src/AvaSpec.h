#ifndef __AvaSpec_h_
#define __AvaSpec_h_

#include <Arduino.h>
#include <USBHost_t36.h>
#include <SD.h>
#include <Logger.h>

/**
 * @class AvaSpec
 * @brief USB Host driver for the Avantes AvaSpec-Mini2048CL spectrometer.
 *
 * This class manages communication between a Teensy 4.1 acting as a USB host
 * and the AvaSpec-Mini spectrometer. It handles initialization, command transmission,
 * and reception of the 4106-byte measurement data frames produced by the spectrometer.
 *
 * The AvaSpec operates over a proprietary USB bulk protocol. Each transaction follows
 * the command-response model defined in Avantes’ protocol documentation:
 *  - 6-byte command headers
 *  - Variable-length payloads
 *  - USB IN/OUT transfers (512-byte packets)
 *
 * Commands implemented here:
 *   - getIdentification()          → Request device metadata
 *   - prepareMeasurement()         → Configure integration parameters
 *   - startMeasurement()           → Begin integration, read data, and log
 *   - stopMeasurement()            → Abort ongoing measurement
 *   - measurementAcknowledgement() → Send ACK to clear device buffer
 *
 * Core features:
 *   - Fully USBHost_t36 compatible
 *   - 4106-byte full-frame readout (10-byte header + 4096-byte pixel data)
 *   - Logging via Logger module (SD interface)
 *   - Explicit debug output via Serial for testing and flat-sat integration
 */
class AvaSpec : public USBDriver {

public:
    /**
     * Constructor
     * @param host Reference to the global USBHost object
     */
    AvaSpec(USBHost &host) { init(); }

    // ──────────────────────────────────────────────
    // High-level command interface
    // ──────────────────────────────────────────────

    /** Retrieve device identification block (Command 0x13). */
    void getIdentification();

    /** Configure acquisition parameters (Command 0x05). */
    void prepareMeasurement();

    /**
     * Start a measurement cycle (Command 0x06).
     * Automatically reads the resulting 4106-byte dataset
     * and sends an acknowledgement to the spectrometer.
     */
    void startMeasurement();

    /** Stop any ongoing measurement (Command 0x0F). */
    void stopMeasurement();

    /**
     * Acknowledge receipt of a measurement (Command 0xC0).
     * Resets internal buffers and logs measurement to SD.
     */
    void measurementAcknowledgement();

    /** Handle unexpected incoming data packets (asynchronous). */
    void handleUnsolicitatedData();

    /** Utility to print a buffer in hexadecimal (debugging). */
    void printBuffer(uint8_t* buf, size_t n);

protected:
    // ──────────────────────────────────────────────
    // USBHost_t36 driver overrides
    // ──────────────────────────────────────────────

    /**
     * Attempt to claim an attached USB device.
     * Called automatically by the USBHost stack when a new device is enumerated.
     */
    virtual bool claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len);

    /** Perform driver initialization: register pipes, transfers, and reset state. */
    void init();

    /** Required override — no explicit disconnect logic used. */
    virtual void disconnect() {};

    /** RX handler called when USB IN transfer completes. */
    void process_rx_data(const Transfer_t *transfer);

    /** TX handler called when USB OUT transfer completes. */
    void process_tx_data(const Transfer_t *transfer);

private:
    // ──────────────────────────────────────────────
    // Internal state and USB structures
    // ──────────────────────────────────────────────

    /** Bulk IN (RX) data pipe pointer. */
    Pipe_t *rxpipe;

    /** Bulk OUT (TX) data pipe pointer. */
    Pipe_t *txpipe;

    /** Transfer sizes for RX/TX (512 bytes per USB bulk packet). */
    uint16_t rx_size;
    uint16_t tx_size;

    /** Endpoint addresses (from USB descriptor). */
    uint8_t rx_ep;
    uint8_t tx_ep;

    /** Statically allocated pipe and transfer structures required by USBHost_t36. */
    Pipe_t mypipes[3] __attribute__ ((aligned(32)));
    Transfer_t mytransfers[7] __attribute__ ((aligned(32)));

    /** Static callback hooks registered with the USB library. */
    static void tx_callback(const Transfer_t *transfer);
    static void rx_callback(const Transfer_t *transfer);

    // ──────────────────────────────────────────────
    // Buffers and data state
    // ──────────────────────────────────────────────

    /** Default USB packet buffer size. */
    static const int BUF_SIZE = 512;

    /** Total measurement size (10-byte header + 4096-byte data). */
    static const int MEAS_SIZE = 4106;

    /** Flag indicating that a USB RX transfer has completed. */
    bool rx_data_ready;

    /** RX transfer buffer (512 bytes). */
    uint8_t rx_buffer[BUF_SIZE];

    /** TX transfer buffer (512 bytes). */
    uint8_t tx_buffer[BUF_SIZE];

    /** Indicates that a valid measurement header (0x21 or 0xB1) was found. */
    bool messageFound;

    /** Legacy counter for partial-packet tracking (retained for compatibility). */
    uint8_t measAmount;

    /** Full measurement storage buffer. */
    uint8_t measurement[MEAS_SIZE];

    /** Write index within measurement buffer. */
    int appendIndex;

    // ──────────────────────────────────────────────
    // Internal helper functions
    // ──────────────────────────────────────────────

    /**
     * Reads the entire 4106-byte measurement dataset.
     * This is automatically invoked by startMeasurement().
     */
    void readFullMeasurement();

    /**
     * Wait helper — blocks until rx_data_ready or timeout.
     * @param timeout_ms Maximum time to wait in milliseconds
     * @return True if data became ready, false if timeout expired
     */
    bool waitForData(uint32_t timeout_ms);
};

#endif // __AvaSpec_h_