//
// Created by srira on 3/27/2026.
//

#include "../include/FlightHardware.h"

#include <bits/regex_constants.h>


void FlightHardware::updateRotaryMode() {
    // need to select the next mode AND configure all settings

    switch (rotary_param.mode) {
        case ApHeading:
            // switch to altitude
            rotary_param.mode         = ApAltitude;
            rotary_param.value        = fd.autopilot.altitude;
            rotary_param.range_upper  = -1;
            rotary_param.range_lower  = 0;
            rotary_param.change_delta = 500;
            rotary_param.displayOnly  = false;
            break;
        case ApAltitude:
            // switch to speed
            rotary_param.mode         = ApSpeed;
            rotary_param.value        = fd.autopilot.speed;
            rotary_param.range_upper  = 500;
            rotary_param.range_lower  = 0;
            rotary_param.change_delta = 25;
            rotary_param.displayOnly  = false;
            break;
        case ApSpeed:
            // switch to flap
            rotary_param.mode         = Flap;
            rotary_param.value        = static_cast<float>(fd.flapPosition);
            rotary_param.range_upper  = 4;
            rotary_param.range_lower  = 0;
            rotary_param.change_delta = -1;
            rotary_param.displayOnly  = true;
        default:
            // switch to heading
            rotary_param.mode         = ApHeading;
            rotary_param.value        = fd.autopilot.heading;
            rotary_param.range_upper  = 360;
            rotary_param.range_lower  = 0;
            rotary_param.change_delta = 1;
            rotary_param.displayOnly  = false;
    }
}

void FlightHardware::updateRotaryValueUp() {
    if (rotary_param.displayOnly) return; // do nothing if its display only

    if (rotary_param.value + rotary_param.change_delta > rotary_param.range_upper) {
        rotary_param.value = rotary_param.range_upper;
    } else {
        rotary_param.value += rotary_param.change_delta;
    }
    // then push our changes to reflect
    pushUpdatedRotaryValue();
}

void FlightHardware::updateRotaryValueDown() {
    if (rotary_param.displayOnly) return; // do nothing if its display only

    if (rotary_param.value - rotary_param.change_delta < rotary_param.range_lower) {
        rotary_param.value = rotary_param.range_lower;
    } else {
        rotary_param.value -= rotary_param.change_delta;
    }
    // then push our changes to reflect
    pushUpdatedRotaryValue();
}

void FlightHardware::pushUpdatedRotaryValue() {
    switch (rotary_param.mode) {
        case ApHeading:
            fd.autopilot.heading = rotary_param.value;
            break;
        case ApAltitude:
            fd.autopilot.altitude = rotary_param.value;
            break;
        case ApSpeed:
            fd.autopilot.speed = rotary_param.value;
            break;
        default:
            break;
    }
}

void FlightHardware::updateAllDisplays() {
    // left display is AIRSPEED
    int airspeed = static_cast<int>(fd.airspeed);
    hw.updateSegmentDisplay(Pin::LEFT_DIO, airspeed);

    // right display is ALTITUDE
    // altitude format - 3,500 ft -> 3.50 k ft
    int formattedAltitude = static_cast<int>(fd.altitude/10.0f);
    hw.updateSegmentDisplay(Pin::RIGHT_DIO, formattedAltitude, 2);

    // center display is ROTARY SELECTION VALUE
    int rotaryValue = rotary_param.value;
    // TODO: need to make this format of like H256, where it has a key and then also the value
    // TODO: for alt need this to be like A035 -> 3,500ft, A100 -> 10,000ft
    bool isDisplayOnly = rotary_param.displayOnly;
    hw.updateSegmentDisplay(Pin::CENTER_DIO, rotaryValue, isDisplayOnly); // show decimal the value is display-only
}

void FlightHardware::updateAllLights() {
    updateAlertLights();
    updateGearLights();
    updateAutopilotLights();
    updateOtherLights();
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
    bool isBlinking = (millis() / 125) % 2 == 0;

    // terrain, need to blink
    hw.updateLEDZone(gpwsZone, {fd.gpws && isBlinking, fd.gpws ? YELLOW : NONE});

    // minimums
    hw.updateLEDZone(minimumsZone, {fd.minimums, fd.minimums ? BLUE : NONE});

    // main alerts
    if (fd.masterWarning) {
        hw.updateLEDZone(alertZone, {isBlinking, RED});
    }
    else if (fd.masterCaution) {
        hw.updateLEDZone(alertZone, {true, ORANGE});
    }
    else if (fd.overspeed) {
        hw.updateLEDZone(alertZone, {true, BLUE});
    }
    else {
        hw.updateLEDZone(alertZone, {false, NONE});
    }
}

void FlightHardware::updateAutopilotLights() {
    bool isBlinking = (millis() / 200) % 2 == 0;
    uint32_t now = millis();

    if (lastApState == true && fd.autopilot.active == false) {
        apDisconnectTime = now;
    }
    lastApState = fd.autopilot.active;

    if (fd.autopilot.active) {
        // ap on
        hw.updateLEDZone(apZone, {true, GREEN});
    }
    // can change this to edit disconnect flashing time
    else if (now - apDisconnectTime < 2000 && apDisconnectTime != 0) {
        // ap just turned off
        hw.updateLEDZone(apZone, {isBlinking, RED});
    }
    else {
        // ap off
        hw.updateLEDZone(apZone, {false, NONE});
    }
}


void FlightHardware::updateOtherLights() {
    // speedbrake lights
    hw.updateLEDZone(speedbrakeZone, {fd.speedbrakes, fd.speedbrakes ? BLUE : NONE});

    // flap transition zone
    hw.updateLEDZone(flapsTransitioningZone, {fd.flapsMoving, fd.flapsMoving ? YELLOW : NONE});

    // parking brake
    hw.updateLEDZone(parkingBrakeZone, {fd.parkingBrake, fd.parkingBrake ? RED : NONE});
}

void FlightHardware::printTesting() const {
    Serial.println("\n--- AEROLAP TELEMETRY DEBUG ---");

    // main
    Serial.print("SPD: "); Serial.print(fd.airspeed, 1); Serial.print(" kts | ");
    Serial.print("ALT: "); Serial.print(fd.altitude, 0); Serial.print(" ft | ");
    Serial.println();

    // control srf
    Serial.print("FLAPS: "); Serial.print(fd.flapPosition);
    Serial.print(fd.flapsMoving ? " [MOVING]" : " [STABLE]");
    Serial.print(" | GEAR: ");

    // print gear
    auto printGear = [](GearPositon p) {
        if (p == DOWN) Serial.print("DWN ");
        else if (p == UP) Serial.print("UP  ");
        else Serial.print("TRN ");
    };
    printGear(fd.gear.front); printGear(fd.gear.rearLeft); printGear(fd.gear.rearRight);
    Serial.println();

    // alets
    Serial.print("ALERTS: ");
    if (fd.masterWarning) Serial.print("[!! WARN !!] ");
    if (fd.masterCaution) Serial.print("[! CAUTION !] ");
    if (fd.overspeed)     Serial.print("[OVERSPEED] ");
    if (fd.minimums)     Serial.print("[MINIMUMS] ");
    if (fd.gpws)         Serial.print("[TERRAIN] ");
    if (!fd.masterWarning && !fd.masterCaution && !fd.overspeed) Serial.print("CLEAN");
    Serial.println();

    // ap
    Serial.print("AP: "); Serial.print(fd.autopilot.active ? "ON" : "OFF");
    Serial.print(" | HDG: "); Serial.print(fd.autopilot.heading, 0);
    Serial.print(" | ALT: "); Serial.println(fd.autopilot.altitude, 0);

    Serial.println("-------------------------------");
}
