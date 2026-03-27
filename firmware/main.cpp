#include <iostream>
#include "include/TelemetryData.h"
#include "include/BaseHardware.h"


int main() {

    FlightData x{};
    x.gear.front = DOWN;

    BaseHardware base;

    LEDBar example;

    LEDBar anotherExample = { {true, RED}, {false, RED}, };
    example.allOn(WHITE);

    base.updateLEDBar(base.PIN_TOP_BAR, example);

    base.updateOneLED(base.PIN_TOP_BAR, 2, {true, RED});

    return 0;
}