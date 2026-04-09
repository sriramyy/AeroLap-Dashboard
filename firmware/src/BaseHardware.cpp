//
// Created by srira on 3/27/2026.
//

#include "../include/BaseHardware.h"


void BaseHardware::begin(uint8_t ledBrightness) {
    // init neopiexels
    topStrip.begin();
    bottomStrip.begin();
    topStrip.setBrightness(ledBrightness);
    bottomStrip.setBrightness(ledBrightness);

    // set Shift Register pins to OUTPUT (for displays)
    pinMode(Pin::SCLK, OUTPUT);
    pinMode(Pin::RCLK, OUTPUT);
    pinMode(Pin::LEFT_DIO, OUTPUT);
    pinMode(Pin::CENTER_DIO, OUTPUT);
    pinMode(Pin::RIGHT_DIO, OUTPUT);

    // clear
    digitalWrite(Pin::SCLK, LOW);
    digitalWrite(Pin::RCLK, LOW);

    topStrip.show();
    bottomStrip.show();
}

void BaseHardware::displayLEDs() {

    for (int i = 0; i < 8; i++) {
        // sync each LED
        topStrip.setPixelColor(i, topBarBuffer.leds[i].getHexColor());
        bottomStrip.setPixelColor(i, bottomBarBuffer.leds[i].getHexColor());
    }

    // push logic
    topStrip.show();
    bottomStrip.show();
}

void BaseHardware::displaySegmentDisplays() {
    leftDisplay.loop();
    centerDisplay.loop();
    rightDisplay.loop();
}

void BaseHardware::startupSequence() {
    updateOneLED(Pin::TOP_BAR, 0, {true, Color::WHITE});
    for (int i = 1; i < 9; i++) {
        updateOneLED(Pin::TOP_BAR, 1, {i > 1, Color::YELLOW});
        updateOneLED(Pin::TOP_BAR, 2, {i > 2, Color::ORANGE});
        updateOneLED(Pin::TOP_BAR, 3, {i > 3, Color::RED});
        updateOneLED(Pin::TOP_BAR, 4, {i > 4, Color::PINK});
        updateOneLED(Pin::TOP_BAR, 5, {i > 5, Color::CYAN});
        updateOneLED(Pin::TOP_BAR, 6, {i > 6, Color::BLUE});
        updateOneLED(Pin::TOP_BAR, 7, {i > 7, Color::GREEN});

        updateSegmentDisplay(Pin::LEFT_DIO, i*111, i % 2 == 0);
        updateSegmentDisplay(Pin::RIGHT_DIO, i*111, i % 2 == 0);

        displayLEDs();
        displaySegmentDisplays();

        delay(100);
    }
    delay(500);

    // turn off all
    updateLEDZone({Pin::TOP_BAR, 0, 8} , {false, NONE});
    updateSegmentDisplay(Pin::LEFT_DIO, 0, false);
    updateSegmentDisplay(Pin::RIGHT_DIO, 0, false);

    displayLEDs();
    displaySegmentDisplays();
}

void BaseHardware::flashLightPin(uint16_t pin, uint8_t flashNum, uint32_t delayTime) {
    for (int i = 0; i < flashNum; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        if (i < flashNum - 1) delay(delayTime);
    }
}


void BaseHardware::updateSegmentDisplay(uint8_t pin, int value, int decimalIndex) {
    DIYables_4Digit7Segment_74HC595* targetDisplay;
    DisplayBuffer* targetBuffer;

    if (pin == Pin::LEFT_DIO) {
        targetDisplay = &leftDisplay;
        targetBuffer = &leftDisplayBuffer;
    } else if (pin == Pin::CENTER_DIO) {
        targetDisplay = &centerDisplay;
        targetBuffer = &centerDisplayBuffer;
    } else if (pin == Pin::RIGHT_DIO) {
        targetDisplay = &rightDisplay;
        targetBuffer = &rightDisplayBuffer;
    } else return;

    // update if the value or decimal has actually changed
    if (targetBuffer->value != value || targetBuffer->decimalIndex != decimalIndex) {
        targetBuffer->value = value;
        targetBuffer->decimalIndex = decimalIndex;

        // printInt clears previous dots automatically
        targetDisplay->printInt(value, false);

        if (decimalIndex >= 0 && decimalIndex < 4) {
            targetDisplay->setDot(decimalIndex);
        }
    }
}

void BaseHardware::updateSegmentDisplay(uint8_t pin, int value) {
    updateSegmentDisplay(pin, value, -1);
}

void BaseHardware::updateLEDBar(uint8_t pin, const LEDBar &ledBar) {
    LEDBar& targetBar = getTargetBarBuffer(pin);
    targetBar = ledBar;
}

void BaseHardware::updateOneLED(uint8_t pin, uint8_t ledIndex, const LED &led) {
    // find the relevant led bar
    LEDBar& targetBar = getTargetBarBuffer(pin);
    // modify the target bar single LED
    targetBar.leds[ledIndex] = led;
}

void BaseHardware::updateOneLED(const SpecificLED &specificLED) {
    LEDBar& targetBar = getTargetBarBuffer(specificLED.barPin);
    targetBar.leds[specificLED.ledIndex] = specificLED.led;
}

void BaseHardware::updateLEDZone(LEDZone zone, LED led) {
    for (int i = 0; i < zone.ledCount; i++) {
        updateOneLED(zone.barPin, zone.startIndex+i, led);
    }
}

LEDBar & BaseHardware::getTargetBarBuffer(uint8_t pin) {
    return (pin == Pin::TOP_BAR) ? topBarBuffer : bottomBarBuffer;
}

DisplayBuffer& BaseHardware::getTargetDisplayBuffer(uint8_t pin) {
    if (pin == Pin::LEFT_DIO) return leftDisplayBuffer;
    if (pin == Pin::CENTER_DIO) return centerDisplayBuffer;
    if (pin == Pin::RIGHT_DIO) return rightDisplayBuffer;

    return centerDisplayBuffer;
}
