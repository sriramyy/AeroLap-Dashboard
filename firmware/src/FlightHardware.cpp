//
// Created by srira on 3/27/2026.
//

#include "../include/FlightHardware.h"


void FlightHardware::updateAllDisplays() {
    // left display is AIRSPEED
    int airspeed = static_cast<int>(fd.airspeed);
    hw.updateSegmentDisplay(Pin::LEFT_DIO, airspeed);

    // right display is ALTITUDE
    // altitude format - 3,500 ft -> 3.50 k ft
    int formattedAltitude = static_cast<int>(fd.altitude/10.0f);
    hw.updateSegmentDisplay(Pin::RIGHT_DIO, formattedAltitude, 2);

    // center display is FLAP POSITION
    // TODO: find a way to get
    int flapPosition = fd.flapPosition;
    bool isFull = fd.flapPosition == ad.flapFull;
    hw.updateSegmentDisplay(Pin::CENTER_DIO, 0, isFull); // show decimal if flaps are full
}

void FlightHardware::updateAllLights() {
    updateAlertLights();
    updateGearLights();
}

void FlightHardware::updateGearLights() {
    // down means green
    // transitioning means yellow
    // up means off

    LED frontGear = {fd.gear.front == DOWN || fd.gear.front == TRANSITIONING, fd.gear.front == DOWN ? GREEN : YELLOW};
    LED rearRightGear = {fd.gear.rearRight == DOWN || fd.gear.rearRight == TRANSITIONING, fd.gear.rearRight == DOWN ? GREEN : YELLOW};
    LED rearLeftGear = {fd.gear.rearLeft == DOWN || fd.gear.rearLeft == TRANSITIONING, fd.gear.rearLeft == DOWN ? GREEN : YELLOW};

    hw.updateOneLED(gearZone.barPin, gearZone.startIndex+1, frontGear);
    hw.updateOneLED(gearZone.barPin, gearZone.startIndex+0, rearLeftGear);
    hw.updateOneLED(gearZone.barPin, gearZone.startIndex+2, rearRightGear);
}

void FlightHardware::updateAlertLights() {
    // Determine the "Current Alert State" based on precedence
    bool isBlinking = (millis() / 250) % 2 == 0; // 4Hz blink rate

    if (fd.masterWarning) {
        // Highest Precedence: Blinking RED
        hw.updateLEDZone(alertZone, {isBlinking, RED});
    }
    else if (fd.masterCaution) {
        // Second Precedence: Solid YELLOW
        hw.updateLEDZone(alertZone, {true, YELLOW});
    }
    else if (fd.overspeed) {
        // Third Precedence: Solid BLUE
        hw.updateLEDZone(alertZone, {true, BLUE});
    }
    else {
        // No alerts: Off
        hw.updateLEDZone(alertZone, {false, NONE});
    }
}

void FlightHardware::updateOtherLights() {
    // speedbrake lights
    if (fd.speedbrakes) hw.updateLEDZone(speedbrakeZone, {true, BLUE});

}
