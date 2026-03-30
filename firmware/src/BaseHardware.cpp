//
// Created by srira on 3/27/2026.
//

#include "../include/BaseHardware.h"

void BaseHardware::begin() {
}

void BaseHardware::updateSegmentDisplay(uint8_t pin, int value) {
}

void BaseHardware::updateSegmentDisplay(uint8_t pin, int value, int decimalIndex) {
}

void BaseHardware::updateLEDBar(uint8_t pin, const LEDBar &ledBar) {
}

void BaseHardware::updateOneLED(uint8_t pin, uint8_t ledIndex, const LED &led) {
}

void BaseHardware::updateOneLED(SpecificLED specificLED) {
}

void BaseHardware::updateLEDZone(LEDZone zone, LED led) {
    for (int i = 0; i < zone.ledCount; i++) {
        updateOneLED(zone.barPin, zone.startIndex+i, led);
    }
}
