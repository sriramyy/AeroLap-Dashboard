#ifndef AEROLAP_BASEHARDWARE_H
#define AEROLAP_BASEHARDWARE_H

#include <FastLED.h>
#include <array>
#include <cstdint>


enum Color {RED, GREEN, BLUE, YELLOW, WHITE};

struct LED {
    bool on;
    Color color;
};

struct LEDBar {
    std::array<LED, 8> leds;

    void allOn(Color color) {
        for (auto& led : leds) {
            led.on = true;
            led.color = color;
        }
    }

    LEDBar(std::initializer_list<LED> list) {
        size_t i = 0;
        for (const auto& item : list) {
            if (i >= 8) break;
            leds[i++] = item;
        }
    }
};

struct SpecificLED {
    uint8_t barPin;
    uint8_t ledIndex; // index within the bar for the specific led
    LED led; // changes for that specific led;
};


class BaseHardware {
public:

    // display pins
    const uint8_t PIN_SCLK = 0;        // shared
    const uint8_t PIN_RCLK = 0;        // shared
    const uint8_t PIN_LEFT_DIO = 0;    // left 4-digit
    const uint8_t PIN_CENTER_DIO = 0;  // center 1-digit
    const uint8_t PIN_RIGHT_DIO = 0;   // right 4-digit

    // led pins
    const uint8_t PIN_TOP_BAR = 0;     //
    const uint8_t PIN_BOTTOM_BAR = 0;  //

    // actual led buffers
    LEDBar topBarState;
    LEDBar bottomBarState;

    void begin();
    void updateSegmentDisplay(uint8_t pin, int value);
    void updateSegmentDisplay(uint8_t pin, int value, int decimalIndex);
    void updateLEDBar(uint8_t pin, const LEDBar& ledBar);
    void updateOneLED(uint8_t pin, uint8_t ledIndex, const LED& led);
    void updateOneLED(SpecificLED specificLED);
};


#endif //AEROLAP_BASEHARDWARE_H