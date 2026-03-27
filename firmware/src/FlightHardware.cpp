//
// Created by srira on 3/27/2026.
//

#include "../include/FlightHardware.h"

void FlightHardware::updateAllDisplays() {
    // left display is AIRSPEED
    int airspeed = static_cast<int>(fd.airspeed);
    hw.updateSegmentDisplay(hw.PIN_LEFT_DIO, airspeed);

    // right display is ALTITUDE
    // altitude format - 3,500 ft -> 3.50 k ft
    int formattedAltitude = static_cast<int>(fd.altitude/10.0f);
    hw.updateSegmentDisplay(hw.PIN_RIGHT_DIO, formattedAltitude, 2);

    // center display is FLAP POSITION
    // TODO: find a way to get
    int flapPosition = fd.flapPosition;
    bool isFull = fd.flapPosition == ad.flapFull;
    hw.updateSegmentDisplay(hw.PIN_CENTER_DIO, 0, isFull); // show decimal if flaps are full
}
