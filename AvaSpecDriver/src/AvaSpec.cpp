#include <Arduino.h>
#include "USBHost_t36.h"
#include "AvaSpec.h"
#include <iostream>
#include <vector>
#include <SD.h>
#include <Logger.h>

// Vendor and Product IDs for Avantes AvaSpec-Mini2048CL spectrometer
#define AV_VID 0x1992
#define AV_PID 0x0668

// ============================================================================
// INITIALIZATION AND DEVICE CLAIM
// ============================================================================

/**
 * Initializes the AvaSpec driver within the USBHost_t36 framework.
 * Sets up pipes, transfer slots, and runtime state variables.
 */
void AvaSpec::init() {
    // Register available pipe and transfer structures with the Teensy USB host controller.
    contribute_Pipes(mypipes, sizeof(mypipes) / sizeof(Pipe_t));
    contribute_Transfers(mytransfers, sizeof(mytransfers) / sizeof(Transfer_t));

    // Notify the USB host library that this driver is ready to claim matching devices.
    driver_ready_for_device(this);

    // Reset internal flags and counters.
    rx_data_ready = false;   // True when RX transfer completes
    messageFound  = false;   // True when valid measurement found
    measAmount    = 0;       // Legacy count of RX chunks
    appendIndex   = 0;       // Index offset for measurement buffer
    measurementCounter = 0;  // Counter for unique CSV filenames

    // Clear the measurement buffer (safety against stale data)
    memset(measurement, 0, MEAS_SIZE);
}

/**
 * Attempt to claim a newly detected USB device.
 * This function is called automatically by the USBHost_t36 framework
 * when new devices are attached to the USB bus.
 */
bool AvaSpec::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len) {
    Serial.println("\nAttempting to claim AvaSpec device...");

    if (type == 0) {
        Serial.printf("VID: %04X  PID: %04X\n", dev->idVendor, dev->idProduct);
    }

    // Reject any device that doesn’t match Avantes’ descriptor pattern.
    if (len != 23) return false;
    if (dev->idVendor != AV_VID || dev->idProduct != AV_PID) return false;

    Serial.println("✅ AvaSpec Mini device found and claimed.");

    Serial.printf("Descriptor length: %lu\n", len);
    Serial.printf("Parsed endpoints — OUT:0x%02X  IN:0x%02X\n", descriptors[11], descriptors[18]);
    Serial.println("✅ Pipes about to be created...");


    // Extract endpoint information from descriptor.
    tx_ep = descriptors[11];   // OUT endpoint address
    rx_ep = descriptors[18];   // IN endpoint address

    rx_size = BUF_SIZE;
    tx_size = BUF_SIZE;

    // Create USB pipes: one for incoming (bulk IN), one for outgoing (bulk OUT)
    rxpipe = new_Pipe(dev, 2, 6, 1, rx_size);
    txpipe = new_Pipe(dev, 2, 2, 0, tx_size);

    // Assign static callback functions
    rxpipe->callback_function = rx_callback;
    txpipe->callback_function = tx_callback;

    Serial.println("✅ USB pipes configured and callbacks registered.");

    return true;
}

// ============================================================================
// DEBUG UTILITIES
// ============================================================================

/**
 * Prints the contents of a byte buffer as hexadecimal values.
 * Newline every 32 bytes for easy visual parsing.
 */
void AvaSpec::printBuffer(uint8_t* buf, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (i % 32 == 0) Serial.println();
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

/**
 * Wait helper — blocks until rx_data_ready or timeout.
 */
bool AvaSpec::waitForData(uint32_t timeout_ms) {
    uint32_t start = millis();
    while (!rx_data_ready && (millis() - start < timeout_ms)) delay(1);
    return rx_data_ready;
}


// ============================================================================
// USB TRANSFER CALLBACKS
// ============================================================================

/**
 * Static callback entry point called by the USBHost_t36 library.
 * Redirects to the object-specific handler.
 */
void AvaSpec::rx_callback(const Transfer_t *transfer) {
    if (transfer->driver) {
        ((AvaSpec *)(transfer->driver))->process_rx_data(transfer);
    }
}

/**
 * Handles incoming USB transfer data.
 * Copies the transfer payload into rx_buffer and sets ready flag.
 */
void AvaSpec::process_rx_data(const Transfer_t *transfer) {
    memcpy(rx_buffer, transfer->buffer, transfer->length);
    rx_data_ready = true;
}

/**
 * Static TX callback entry point.
 */
void AvaSpec::tx_callback(const Transfer_t *transfer) {
    if (transfer->driver) {
        ((AvaSpec *)(transfer->driver))->process_tx_data(transfer);
    }
}

/**
 * Handles completed TX transfers.
 * Currently unused but retained for debugging or LED signaling.
 */
void AvaSpec::process_tx_data(const Transfer_t *transfer) {
    // Transmission complete
}

// ============================================================================
// LOW-LEVEL HELPERS
// ============================================================================

/**
 * Queues a background read to capture any unsolicited messages
 * from the device (e.g., async notifications or residual data).
 */
void AvaSpec::handleUnsolicitatedData() {
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
    memset(rx_buffer, 0, BUF_SIZE);
    rx_data_ready = false;
}

// ============================================================================
// COMMAND: GET IDENTIFICATION (0x13)
// ============================================================================

/**
 * Request device identification and configuration details.
 * Used to confirm successful communication.
 */
void AvaSpec::getIdentification() {
    uint32_t len = 0;

    // Command frame structure (6 bytes total)
    tx_buffer[len++] = 0x20; // [0] Protocol identifier (constant)
    tx_buffer[len++] = 0x00; // [1] Sequence number (unused)
    tx_buffer[len++] = 0x02; // [2] Payload length LSB
    tx_buffer[len++] = 0x00; // [3] Payload length MSB
    tx_buffer[len++] = 0x13; // [4] Command ID (Get Identification)
    tx_buffer[len++] = 0x00; // [5] Options (none)

    Serial.println("\nSending command: get_ident");
    printBuffer(tx_buffer, len);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

    delay(25); // allow device to process before reading

    rx_data_ready = false;
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

    uint32_t start = millis();
    
    if (waitForData(3000)) {
        Serial.println("\nResponse: get_ident");
        printBuffer(rx_buffer, 92);
    } else {
        Serial.println("❌ No response received (timeout).");
    }


    memset(tx_buffer, 0, BUF_SIZE);
    memset(rx_buffer, 0, BUF_SIZE);
}

// ============================================================================
// COMMAND: PREPARE MEASUREMENT (0x05)
// ============================================================================

/**
 * Configures spectrometer parameters before measurement.
 * Each field corresponds to bytes in the Avantes USB protocol specification.
 */
void AvaSpec::prepareMeasurement() {
    memset(tx_buffer, 0, BUF_SIZE);

    // ──────────────────────────────────────────────
    // Header section (6 bytes)
    // ──────────────────────────────────────────────
    tx_buffer[0] = 0x20;  // [0] Protocol start marker
    tx_buffer[1] = 0x00;  // [1] Sequence number
    tx_buffer[2] = 0x2B;  // [2] Payload length LSB (43 bytes)
    tx_buffer[3] = 0x00;  // [3] Payload length MSB
    tx_buffer[4] = 0x05;  // [4] Command ID: Prepare Measurement
    tx_buffer[5] = 0x00;  // [5] Reserved / flags (none)

    // ──────────────────────────────────────────────
    // Pixel range (start and stop pixels)
    // ──────────────────────────────────────────────
    tx_buffer[6] = 0x00;  // [6] Start pixel MSB
    tx_buffer[7] = 0x00;  // [7] Start pixel LSB
    tx_buffer[8] = 0x07;  // [8] Stop pixel MSB (0x07FF = 2047)
    tx_buffer[9] = 0xFF;  // [9] Stop pixel LSB

    // ──────────────────────────────────────────────
    // Integration time (4 bytes, little endian)
    // Example: 200,000 µs = 0x00030D40
    // ──────────────────────────────────────────────
    tx_buffer[10] = 0x40; // [10] LSB
    tx_buffer[11] = 0x0D; // [11]
    tx_buffer[12] = 0x03; // [12]
    tx_buffer[13] = 0x00; // [13] MSB

    // ──────────────────────────────────────────────
    // Integration delay (4 bytes)
    // Default = 0 (no delay)
    // ──────────────────────────────────────────────
    tx_buffer[14] = 0x00; // [14] LSB
    tx_buffer[15] = 0x00; // [15]
    tx_buffer[16] = 0x00; // [16]
    tx_buffer[17] = 0x00; // [17] MSB

    // ──────────────────────────────────────────────
    // Number of averages (4 bytes)
    // Usually 1 for single-shot measurements
    // ──────────────────────────────────────────────
    tx_buffer[18] = 0x00; // [18] LSB
    tx_buffer[19] = 0x00; // [19]
    tx_buffer[20] = 0x00; // [20]
    tx_buffer[21] = 0x01; // [21] MSB → 1 average

    // ──────────────────────────────────────────────
    // Remaining 21 bytes (trigger mode, control, reserved)
    // All set to 0 for software-triggered mode.
    // ──────────────────────────────────────────────
    memset(&tx_buffer[22], 0, 21);
    tx_buffer[22] = 0x01;  // enable software trigger


    Serial.println("\nSending command: prepare_measurement");
    printBuffer(tx_buffer, 47);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

    delay(25);

    rx_data_ready = false;
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

    uint32_t start = millis();
    if (waitForData(3000)) {
        Serial.println("\nResponse: prepare_measurement");
        printBuffer(rx_buffer, 8);
    } else {
        Serial.println("❌ No response received (timeout).");
    }


    memset(tx_buffer, 0, BUF_SIZE);
    memset(rx_buffer, 0, BUF_SIZE);
}

// ============================================================================
// COMMAND: START MEASUREMENT (0x06)
// ============================================================================

/**
 * Starts a single measurement.
 * The device will integrate light and prepare 4106 bytes of output.
 */
void AvaSpec::startMeasurement() {
    memset(tx_buffer, 0, BUF_SIZE);
    uint32_t len = 0;

    // ──────────────────────────────────────────────
    // Header section (8 bytes)
    // ──────────────────────────────────────────────
    tx_buffer[len++] = 0x20; // [0] Protocol start
    tx_buffer[len++] = 0x00; // [1] Sequence number
    tx_buffer[len++] = 0x04; // [2] Payload length LSB (4 bytes)
    tx_buffer[len++] = 0x00; // [3] Payload length MSB
    tx_buffer[len++] = 0x06; // [4] Command ID = Start Measurement
    tx_buffer[len++] = 0x00; // [5] Flags (none)
    tx_buffer[len++] = 0x00; // [6] Unused
    tx_buffer[len++] = 0x04; // [7] Measurement count (1 measurement)

    Serial.println("\nSending command: start_measurement");
    printBuffer(tx_buffer, len);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

    delay(50);

    rx_data_ready = false;
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

    uint32_t start = millis();
    while (!rx_data_ready && (millis() - start < 2000)) delay(1);

    if (waitForData(2000)) {
        Serial.println("\nResponse: start_measurement");
        printBuffer(rx_buffer, 6);
    } else {
        Serial.println("⚠️ No ACK received, continuing to data read...");
    }


    rx_data_ready = false;
    memset(tx_buffer, 0, BUF_SIZE);
    memset(rx_buffer, 0, BUF_SIZE);

    // Immediately begin reading the full dataset.
    readFullMeasurement();

    // After complete capture, acknowledge and log.
    measurementAcknowledgement();
}

// ============================================================================
// COMMAND: STOP MEASUREMENT (0x0F)
// ============================================================================

/**
 * Sends a command to halt ongoing measurements.
 */
void AvaSpec::stopMeasurement() {
    memset(tx_buffer, 0, BUF_SIZE);

    // ──────────────────────────────────────────────
    // Header (6 bytes)
    // ──────────────────────────────────────────────
    tx_buffer[0] = 0x20; // [0] Protocol ID
    tx_buffer[1] = 0x00; // [1] Sequence number
    tx_buffer[2] = 0x02; // [2] Payload length LSB (2)
    tx_buffer[3] = 0x00; // [3] Payload length MSB
    tx_buffer[4] = 0x0F; // [4] Command ID = Stop Measurement
    tx_buffer[5] = 0x00; // [5] Flags (none)

    Serial.println("\nSending command: stop_measurement");
    printBuffer(tx_buffer, 6);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

    delay(25);

    rx_data_ready = false;
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

    uint32_t start = millis();
    if (waitForData(2000)) {
        Serial.println("Response: stop_measurement");
        printBuffer(rx_buffer, 12);
    } else {
        Serial.println("❌ No response (timeout).");
    }


    memset(tx_buffer, 0, BUF_SIZE);
    memset(rx_buffer, 0, BUF_SIZE);
}

// ============================================================================
// COMMAND: READ FULL MEASUREMENT DATA
// ============================================================================

/**
 * Reads the complete 4106-byte dataset from the spectrometer
 * (10-byte header + 4096-byte pixel intensity data).
 * 
 * The AvaSpec sends data in 512-byte USB bulk packets. This function
 * assembles each packet into the global measurement buffer until all
 * bytes are received. Serial progress messages indicate read status.
 */
void AvaSpec::readFullMeasurement() {
    const uint16_t totalBytes = 4106;   // Total bytes per measurement
    uint16_t bytesReceived = 0;         // Running count

    // Clear any previous data before starting a new read
    memset(measurement, 0, totalBytes);
    Serial.println("\n📡 Reading full 4106-byte measurement...");

    // Continue requesting data until all bytes are received
    while (bytesReceived < totalBytes) {
        memset(rx_buffer, 0, BUF_SIZE);

        // Queue USB transfer for next 512-byte packet
        rx_data_ready = false;
        __disable_irq();
        queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
        __enable_irq();

        // Wait until transfer completes or times out (3 s)
        uint32_t start = millis();
        while (!rx_data_ready && (millis() - start < 3000)) {
            delay(1);
        }

        // Copy received bytes into the global buffer
        uint16_t n = min<uint16_t>(512, totalBytes - bytesReceived);
        memcpy(&measurement[bytesReceived], rx_buffer, n);
        bytesReceived += n;

        Serial.printf("Chunk received: %u / %u bytes\n", bytesReceived, totalBytes);
    }

    Serial.println("✅ Full 4106 bytes received.\n");
}

// ============================================================================
// COMMAND: MEASUREMENT ACKNOWLEDGEMENT (0xC0)
// ============================================================================

/**
 * Acknowledges receipt of the measurement to the device,
 * logs the data, writes a CSV file of the spectrum, and
 * resets internal state variables.
 */
void AvaSpec::measurementAcknowledgement() {
    // ──────────────────────────────────────────────
    // Step 1: Write spectrum to CSV (Pixel,Intensity)
    // ──────────────────────────────────────────────
    // Increment measurement counter
    measurementCounter++;

    // Create unique filename (e.g., spectrum_0001.csv, spectrum_0002.csv, etc.)
    char csvFilename[32];
    snprintf(csvFilename, sizeof(csvFilename), "/spectrum_%04lu.csv", measurementCounter);

    Serial.print("\nWriting ");
    Serial.print(csvFilename);
    Serial.println(" to SD card...");

    File csvFile = SD.open(csvFilename, FILE_WRITE);
    if (csvFile) {
        // Write CSV header
        csvFile.println("Pixel,Intensity");

        // Skip the first 10 header bytes (data begins at byte 10)
        for (int i = 10; i < MEAS_SIZE; i += 2) {
            int pixelIndex = (i - 10) / 2;                     // Pixel index (0–2047)
            uint16_t intensity = measurement[i] | (measurement[i + 1] << 8);  // 16-bit LE value
            csvFile.print(pixelIndex);
            csvFile.print(",");
            csvFile.println(intensity);
        }

        csvFile.flush();
        csvFile.close();
        Serial.print("✅ ");
        Serial.print(csvFilename);
        Serial.println(" successfully written to SD card.");
    } else {
        Serial.print("❌ Failed to open ");
        Serial.print(csvFilename);
        Serial.println(" for writing!");
    }

    // ──────────────────────────────────────────────
    // Step 2: Log the raw measurement to SD (hex dump)
    // ──────────────────────────────────────────────
    Serial.println("Logging raw measurement data (hex)...");
    logMeasurement(measurement, sizeof(measurement));

    // ──────────────────────────────────────────────
    // Step 3: Build and send measurement acknowledgement (0xC0)
    // ──────────────────────────────────────────────
    memset(tx_buffer, 0, BUF_SIZE);
    tx_buffer[0] = 0x20; // [0] Protocol ID
    tx_buffer[1] = 0x00; // [1] Sequence number
    tx_buffer[2] = 0x02; // [2] Payload length LSB (2)
    tx_buffer[3] = 0x00; // [3] Payload length MSB
    tx_buffer[4] = 0xC0; // [4] Command ID = Acknowledge Measurement
    tx_buffer[5] = 0x00; // [5] Flags (none)

    Serial.println("\nSending measurement acknowledgement:");
    printBuffer(tx_buffer, 6);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

    delay(10);
    memset(tx_buffer, 0, BUF_SIZE);

    // ──────────────────────────────────────────────
    // Step 4: Reset internal state for next cycle
    // ──────────────────────────────────────────────
    messageFound = false;
    measAmount = 0;
    appendIndex = 0;

    Serial.println("Measurement acknowledgement complete.\n");