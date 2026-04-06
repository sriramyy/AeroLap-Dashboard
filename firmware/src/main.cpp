#include <Arduino.h>
#include "BaseHardware.h"
#include "Pin.h"

BaseHardware hw;

void setup() {
    hw.begin();

    hw.updateOneLED(Pin::TOP_BAR, 0, {true, GREEN});
    hw.updateOneLED(Pin::TOP_BAR, 1, {true, YELLOW});
    hw.updateOneLED(Pin::TOP_BAR, 2, {true, GREEN});

    // TODO: only showing the decimal
    hw.updateSegmentDisplay(Pin::LEFT_DIO, 1000, 2);

    hw.displayLEDs();
}

void loop() {
    // hw.displayLEDs();
    hw.displaySegmentDisplays();
}
