# Makefile for VIA Native Linux Build
#
# Builds the VIA firmware for Linux simulation testing.
# Simulates spectrometer responses without actual hardware.
#
# Cross-compilation for ARM64 (Pi 400):
#   make CXX=aarch64-linux-gnu-g++ TARGET=via_native_arm64

CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -static
INCLUDES = -I.

TARGET ?= via_native
SOURCES = main_native.cpp

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SOURCES) Arduino.h SD.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCES)

clean:
	rm -f via_native via_native_x64 via_native_arm64

install: $(TARGET)
	mkdir -p $(HOME)/Aeris/bin
	cp $(TARGET) $(HOME)/Aeris/bin/
	@echo "Installed to $(HOME)/Aeris/bin/$(TARGET)"
