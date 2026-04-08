#ifndef AEROLAP_PINS_H
#define AEROLAP_PINS_H

#include <cstdint>

// Specifies the pin values for components
namespace Pin {
    // constexpr tells the compiler to swap these at compile-time.
    // saves memory

    // onboard led
    constexpr uint8_t ONBOARD_LED = 2;

    // LED Bars
    constexpr uint8_t TOP_BAR    = 13;
    constexpr uint8_t BOTTOM_BAR = 12;

    // 7-Segment Displays (Shift Register)
    constexpr uint8_t SCLK       = 14; 
    constexpr uint8_t RCLK       = 27;
    constexpr uint8_t LEFT_DIO   = 26;
    constexpr uint8_t CENTER_DIO = 25;
    constexpr uint8_t RIGHT_DIO  = 33;
}

#endif