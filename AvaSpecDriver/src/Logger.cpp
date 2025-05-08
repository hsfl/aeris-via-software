#include <Logger.h>
#include <Arduino.h>

File logFile; 

bool initializeFile(const char *filename) {
    if (SD.begin(BUILTIN_SDCARD)) {
        logFile = SD.open(filename, FILE_WRITE);
        if (!logFile) {
            Serial.println("Failed to open file for writing!");
            return false;
        } else if (logFile) {
            return true;
        }
    } else {
        Serial.println("SD Card failed to initialize!");
        return false; 
    }
}

void logMeasurement(uint8_t* buf, size_t n) {
    if (logFile) {
        for (size_t i = 0; i < n; i++)
        {
            if (i % 32 == 0)
            { // Print a newline every 16 bytes for readability. will print a newline immediately
                logFile.println();
            }
            if (buf[i] < 0x10)
            { // Add leading zero for single hex digits
                logFile.print("0");
            }
            logFile.print(buf[i], HEX);
            logFile.print(" ");
        }
        logFile.println();
        logFile.flush(); // Data is saved
    }
}

void logString(const String &text) {
    if (logFile) {
        logFile.println(text);
        logFile.flush();
    }
}

void closeLogFile() {
    if (logFile) {
        logFile.close();
    }
}