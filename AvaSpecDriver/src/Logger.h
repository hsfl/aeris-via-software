#ifndef __Logger_h_
#define __Logger_h_
#include <Arduino.h>
#include <SD.h>

extern File logFile;

bool initializeFile(const char *filename);
void logMeasurement(uint8_t* buf, size_t n);
void logString(const String &text);
void closeLogFile();

#endif