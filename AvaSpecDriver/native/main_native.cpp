/**
 * @file main_native.cpp
 * @brief Native Linux entry point for VIA firmware simulation
 *
 * Simulates the VIA spectrometer firmware for testing without hardware.
 * Generates fake spectrum data that looks like real AvaSpec output.
 *
 * Usage:
 *   ./via_native
 *
 * Commands are read from stdin, output goes to stdout.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <random>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <cstdarg>

#include "Arduino.h"
#include "SD.h"

// Global instances
SerialClass Serial;
SDClass SD;

// Simulation state
static std::atomic<bool> g_running(true);
static int g_measurementCount = 0;
static bool g_useSD = false;
static bool g_autoMode = false;
static unsigned long g_autoInterval = 100000;  // 100 seconds default
static unsigned long g_lastMeasurementTime = 0;

// Random number generator for simulated spectra
static std::mt19937 g_rng(42);

// Signal handler
void signalHandler(int) {
    g_running = false;
}

/**
 * @brief Check if input is available on stdin
 */
bool SerialClass::available() {
    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    return poll(&pfd, 1, 0) > 0;
}

/**
 * @brief Read a line from stdin
 */
String SerialClass::readStringUntil(char terminator) {
    std::string result;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == terminator) break;
        result += c;
    }
    return String(result.c_str());
}

/**
 * @brief Generate simulated spectrum data
 *
 * Creates realistic-looking spectrum with:
 * - Baseline noise
 * - A few emission peaks at random wavelengths
 * - Gaussian peak shapes
 */
void generateSimulatedSpectrum(uint16_t* spectrum, size_t numPixels) {
    std::uniform_real_distribution<float> noise(0, 100);
    std::uniform_real_distribution<float> peakPos(200, 1800);
    std::uniform_real_distribution<float> peakHeight(5000, 40000);
    std::uniform_real_distribution<float> peakWidth(10, 50);

    // Baseline with noise
    for (size_t i = 0; i < numPixels; i++) {
        spectrum[i] = (uint16_t)(500 + noise(g_rng));
    }

    // Add 3-5 emission peaks
    int numPeaks = 3 + (g_rng() % 3);
    for (int p = 0; p < numPeaks; p++) {
        float center = peakPos(g_rng);
        float height = peakHeight(g_rng);
        float width = peakWidth(g_rng);

        for (size_t i = 0; i < numPixels; i++) {
            float x = (float)i;
            float gaussian = height * exp(-0.5 * pow((x - center) / width, 2));
            spectrum[i] = min<uint16_t>(65535, spectrum[i] + (uint16_t)gaussian);
        }
    }
}

/**
 * @brief Display help information
 */
void showHelp() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("  AVANTES SPECTROMETER COMMAND CONSOLE (SIMULATION)");
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
    Serial.println(g_useSD ? "ENABLED" : "DISABLED");
    Serial.print("  Auto Mode:           ");
    Serial.println(g_autoMode ? "RUNNING" : "STOPPED");
    if (g_autoMode) {
        Serial.print("  Auto Interval:       ");
        Serial.print(g_autoInterval / 1000);
        Serial.println(" seconds");
    }
    Serial.print("  Measurements Taken:  ");
    Serial.println(g_measurementCount);
    Serial.print("  Uptime:              ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("────────────────────────────────────────────");
    Serial.println();
}

/**
 * @brief Perform a simulated measurement
 */
void performMeasurement() {
    g_measurementCount++;

    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.print("Starting Measurement #");
    Serial.println(g_measurementCount);
    Serial.println("════════════════════════════════════════════════════════");

    // Simulate measurement steps
    Serial.println("🛑 Ensuring device is stopped...");
    delay(100);

    Serial.println("📡 Querying device identification...");
    delay(200);

    Serial.println("⚙️  Preparing measurement parameters...");
    delay(200);

    Serial.println("🔬 Starting measurement...");
    delay(500);

    Serial.println("⏳ Acquiring data...");
    delay(1000);

    Serial.println("✅ Data acquisition complete");

    // Generate simulated spectrum
    const size_t numPixels = 2048;
    uint16_t spectrum[numPixels];
    generateSimulatedSpectrum(spectrum, numPixels);

    // Output raw hex dump (4106 bytes = 10 header + 4096 data)
    Serial.println();
    Serial.println("Reading full 4106-byte measurement...");

    // Create measurement buffer (10 header bytes + 4096 data bytes)
    uint8_t measurement[4106];
    // Header (first 10 bytes)
    measurement[0] = 0x20;  // Protocol ID
    measurement[1] = 0x00;  // Sequence
    measurement[2] = 0x0A;  // Payload length LSB
    measurement[3] = 0x10;  // Payload length MSB
    measurement[4] = 0x00;  // Command response
    measurement[5] = 0x00;  // Status
    measurement[6] = 0x00;  // Reserved
    measurement[7] = 0x00;
    measurement[8] = 0x00;
    measurement[9] = 0x00;

    // Copy spectrum data (little-endian 16-bit values)
    for (size_t i = 0; i < numPixels; i++) {
        measurement[10 + i*2] = spectrum[i] & 0xFF;
        measurement[10 + i*2 + 1] = (spectrum[i] >> 8) & 0xFF;
    }

    // Output hex dump (16 bytes per line, matching real firmware)
    for (size_t i = 0; i < 4106; i++) {
        if (i % 16 == 0 && i > 0) Serial.println();
        if (measurement[i] < 0x10) Serial.print("0");
        Serial.print(measurement[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    Serial.println("Full 4106 bytes received.");

    // Output CSV data
    Serial.println();
    Serial.println("──────────────────────────────────────────────");
    Serial.println("CSV DATA OUTPUT:");
    Serial.println("──────────────────────────────────────────────");
    Serial.println("Pixel,Intensity");

    for (size_t i = 0; i < numPixels; i++) {
        Serial.print((int)i);
        Serial.print(",");
        Serial.println(spectrum[i]);
    }

    Serial.println("──────────────────────────────────────────────");
    Serial.println("END CSV DATA");
    Serial.println("──────────────────────────────────────────────");

    // Write to SD if enabled
    if (g_useSD) {
        char filename[64];
        snprintf(filename, sizeof(filename), "data/spectrum_%04d.csv", g_measurementCount);
        FILE* f = fopen(filename, "w");
        if (f) {
            fprintf(f, "Pixel,Intensity\n");
            for (size_t i = 0; i < numPixels; i++) {
                fprintf(f, "%zu,%u\n", i, spectrum[i]);
            }
            fclose(f);
            Serial.print("💾 Data logged to: ");
            Serial.println(filename);
        }
    }

    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("Measurement Complete!");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
}

/**
 * @brief Simulate device identification query
 */
void getIdentification() {
    Serial.println();
    Serial.println("📡 Device Identification:");
    Serial.println("  Model:    AvaSpec-Mini2048CL (SIMULATED)");
    Serial.println("  Serial:   SIM-2024-001");
    Serial.println("  Firmware: 1.0.0-native");
    Serial.println("  Pixels:   2048");
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
        getIdentification();
    }
    else if (cmd.startsWith("auto")) {
        int spaceIdx = cmd.indexOf(' ');
        if (spaceIdx > 0) {
            String intervalStr = cmd.substring(spaceIdx + 1);
            int seconds = intervalStr.toInt();
            if (seconds > 0) {
                g_autoInterval = seconds * 1000UL;
            }
        }

        g_autoMode = true;
        g_lastMeasurementTime = millis();
        Serial.println();
        Serial.println("🔄 Auto-measurement mode STARTED");
        Serial.print("   Interval: ");
        Serial.print(g_autoInterval / 1000);
        Serial.println(" seconds");
        Serial.println("   Type 'stop' to end auto mode");
        Serial.println();
    }
    else if (cmd == "stop") {
        if (g_autoMode) {
            g_autoMode = false;
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
        g_useSD = true;
        Serial.println("💾 SD card logging enabled");
    }
    else if (cmd == "sd-off") {
        g_useSD = false;
        Serial.println("💾 SD card logging disabled");
    }
    else if (cmd.length() > 0) {
        Serial.print("❌ Unknown command: '");
        Serial.print(cmd.c_str());
        Serial.println("'");
        Serial.println("   Type 'help' for available commands");
    }
}

/**
 * @brief Main entry point
 */
int main() {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // Initialize SD simulation
    SD.begin(0);

    // Print startup banner
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("  AERIS VIA Spectrometer Control System");
    Serial.println("  Version 3.0 - NATIVE SIMULATION MODE");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("Initializing SD Card...");
    Serial.println("✅ SD Card initialized (simulation)");
    Serial.println();
    Serial.println("Initializing USB Host...");
    Serial.println("✅ USB Host controller initialized (simulation)");
    Serial.println("   AvaSpec spectrometer SIMULATED");
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println("System Ready!");
    Serial.println("════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("Type 'help' for available commands");
    Serial.println();
    Serial.print("VIA> ");
    fflush(stdout);

    // Main loop
    while (g_running) {
        // Check for serial commands
        if (Serial.available()) {
            String command = Serial.readStringUntil('\n');
            Serial.println(command.c_str());
            processCommand(command);

            if (!g_autoMode) {
                Serial.print("VIA> ");
                fflush(stdout);
            }
        }

        // Handle auto-measurement mode
        if (g_autoMode) {
            unsigned long currentTime = millis();
            if (currentTime - g_lastMeasurementTime >= g_autoInterval) {
                performMeasurement();
                g_lastMeasurementTime = currentTime;

                Serial.print("⏰ Next measurement in ");
                Serial.print(g_autoInterval / 1000);
                Serial.println(" seconds (type 'stop' to end)");
                Serial.println();
            }
        }

        // Small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    Serial.println("\n[Native] Shutting down...");
    return 0;
}
