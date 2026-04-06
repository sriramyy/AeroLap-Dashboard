#ifndef AEROLAP_BASEHARDWARE_H
#define AEROLAP_BASEHARDWARE_H

#include <array>
#include <cstdint>
#include <Adafruit_NeoPixel.h>
#include <DIYables_4Digit7Segment_74HC595.h>
#include "Pin.h"

enum Color {RED, GREEN, BLUE, YELLOW, WHITE, NONE};

struct LED {
    bool on;
    Color color;

    // helper to get the hex color
    uint32_t getHexColor() const {
        if (!on) return 0;

        switch (color) {
            case RED:    return Adafruit_NeoPixel::Color(255, 0, 0);
            case GREEN:  return Adafruit_NeoPixel::Color(0, 255, 0);
            case BLUE:   return Adafruit_NeoPixel::Color(0, 0, 255);
            case YELLOW: return Adafruit_NeoPixel::Color(255, 255, 0);
            case WHITE:  return Adafruit_NeoPixel::Color(255, 255, 255);
            default:     return 0;
        }
    }
};

struct LEDBar {
    std::array<LED, 8> leds{};

    LEDBar() { leds.fill({false,NONE}); }

    LEDBar(std::initializer_list<LED> list) {
        size_t i = 0;
        for (const auto& item : list) {
            if (i >= 8) break;
            leds[i++] = item;
        }
    }

    void allOn(Color color) {
        for (auto& led : leds) {
            led.on = true;
            led.color = color;
        }
    }
};

// barPin, startIndex, ledCount
struct LEDZone {
    uint8_t barPin;
    uint8_t startIndex;
    uint8_t ledCount;
};

struct SpecificLED {
    uint8_t barPin;
    uint8_t ledIndex; // index within the bar for the specific led
    LED led; // changes for that specific led;
};

// for 7-segment display buffer
struct DisplayBuffer {
    uint8_t value;
    int decimalIndex; // -1 if no decimal

    DisplayBuffer() : value(0000), decimalIndex(-1) {}
};


class BaseHardware {
    // lights use Hardware Abstraction Layer interact with the internal adafruit strips logic

    // PIN MOVED TO Pin.h

private:
    // internal Adafruit strips
    Adafruit_NeoPixel topStrip = Adafruit_NeoPixel(8, Pin::TOP_BAR, NEO_GRB + NEO_KHZ800);
    Adafruit_NeoPixel bottomStrip = Adafruit_NeoPixel(8, Pin::BOTTOM_BAR, NEO_GRB + NEO_KHZ800);

    // internal LED buffers, these are edited by the update functions
    LEDBar topBarBuffer;
    LEDBar bottomBarBuffer;

    // internal diyables display
    DIYables_4Digit7Segment_74HC595 leftDisplay = DIYables_4Digit7Segment_74HC595(Pin::SCLK, Pin::RCLK, Pin::LEFT_DIO);
    DIYables_4Digit7Segment_74HC595 centerDisplay = DIYables_4Digit7Segment_74HC595(Pin::SCLK, Pin::RCLK, Pin::CENTER_DIO);
    DIYables_4Digit7Segment_74HC595 rightDisplay = DIYables_4Digit7Segment_74HC595(Pin::SCLK, Pin::RCLK, Pin::RIGHT_DIO);

    // internal 7-segment display buffers
    DisplayBuffer leftDisplayBuffer;
    DisplayBuffer centerDisplayBuffer;
    DisplayBuffer rightDisplayBuffer;


public:

    // functions modify hardware / update/render
    void begin(uint8_t ledBrightness = 5);
    void displayLEDs();
    void displaySegmentDisplays();

    // update internal values
    void updateSegmentDisplay(uint8_t pin, int value);
    void updateSegmentDisplay(uint8_t pin, int value, int decimalIndex);
    void updateLEDBar(uint8_t pin, const LEDBar& ledBar);
    void updateOneLED(uint8_t pin, uint8_t ledIndex, const LED& led);
    void updateOneLED(const SpecificLED &specificLED);
    void updateLEDZone(LEDZone zone, LED led);

    // helper functions
    LEDBar& getTargetBarBuffer(uint8_t pin);
    DisplayBuffer& getTargetDisplayBuffer(uint8_t pin);
};


#endif //AEROLAP_BASEHARDWARE_H