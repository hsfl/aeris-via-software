#include <Arduino.h>
#include "USBHost_t36.h"
#include "AvaSpec.h"
#include <iostream>
#include <vector>
#include <SD.h>
#include <Logger.h>

File measurementFile;

USBHost myusb;
AvaSpec myavaspec(myusb);
// USBDriver driver0(&myavaspec);
bool useSD = false;
bool useFile = false;

void setup() {
    Serial.begin(115200);
    
    if (initializeFile("/measurements.txt")) {
        Serial.println("Writing to SD Card");
    }

    // while (!Serial);
    myusb.begin();
}

void loop() {
    myusb.Task();
    delay(1000);
    myavaspec.getIdentification();
    delay(1000);
    myavaspec.prepareMeasurement();
    delay(1000);
    myavaspec.startMeasurement();
    int startTime = millis();
    while (true) {
        // Serial.println(millis());
        myavaspec.handleUnsolicitatedData(); 
        delay(100);
        if (millis() - startTime > 10000) {
            Serial.println("30 seconds elapsed, exiting loop...");
            break;
        }
    }
    myavaspec.measurementAcknowledgement();
    myavaspec.stopMeasurement();
    logString("=== Session End ===");
    closeLogFile();
    delay(100000);
}
