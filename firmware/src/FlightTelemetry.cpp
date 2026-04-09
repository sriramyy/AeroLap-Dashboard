#include "../include/FlightTelemetry.h"
#include <Arduino.h>


void FlightTelemetry::updateFromSim(TelemetryPacket &incoming) {
    flightData.altitude = incoming.alt_ft;
    flightData.airspeed = incoming.speed;
    flightData.verticalSpeed = incoming.v_speed;

    flightData.pitch = incoming.pitch;
    flightData.roll = incoming.roll;
    flightData.heading = incoming.heading;

    // convert flap position to int from 0-8
    flightData.flapPosition = convertToFlapPosition(incoming.flaps_raw);
    flightData.flapsMoving = incoming.flaps_moving;

    // convert gear pos
    flightData.gear.front = convertToGearPosition(incoming.gear_nose);
    flightData.gear.rearLeft = convertToGearPosition(incoming.gear_left);
    flightData.gear.rearRight = convertToGearPosition(incoming.gear_right);

    // parkign brake
    flightData.parkingBrake = incoming.parking_brake;
    // speed brake
    flightData.speedbrakes = incoming.speedbrakes_ext;

    // alerts
    flightData.masterWarning = incoming.masterWarning;
    flightData.masterCaution = incoming.masterCaution;
    flightData.overspeed = incoming.overspeed;

    // minimums, trigger below the ft
    flightData.minimums = incoming.radio_alt <= 400 && incoming.radio_alt != 0;

    // autopilot
    flightData.autopilot = {incoming.ap_active, incoming.ap_heading, incoming.ap_alt};

    // calc gpws manually
    flightData.gpws = determineGPWS(incoming.radio_alt, incoming.v_speed, incoming.flaps_raw);
}

void FlightTelemetry::updateAutopilotHeading(float heading) {
    flightData.autopilot.heading = heading;
    pushCommand(AP_HDG_SET, heading);
}

void FlightTelemetry::updateAutopilotStatus(bool active) {
    flightData.autopilot.active = active;
    pushCommand(AP_MASTER_SET, active);
}

void FlightTelemetry::pushCommand(CommandID commandID, float value) {
    CommandPacket packet;
    packet.commandID = commandID;
    packet.value = value;

    Serial.write((uint8_t*)&packet, sizeof(CommandPacket));
    Serial.flush(); // send
}

GearPositon FlightTelemetry::convertToGearPosition(uint16_t raw_pos) {
    if (raw_pos == 16383) return GearPositon::DOWN;
    if (raw_pos == 0) return GearPositon::UP;
    return GearPositon::TRANSITIONING;
}

int FlightTelemetry::convertToFlapPosition(uint16_t raw_pos) {
    if (raw_pos == 0) return 0;
    if (raw_pos >= 16383) return 8; // full

    return (int)((raw_pos * 8 + 8191) / 16383);
}

bool FlightTelemetry::determineGPWS(float ralt, float vs, uint16_t flaps_raw) {

    // sink rate
    // If you're dropping faster than 2,500 fpm while below 2,500 ft AGL
    if (ralt > 20 && ralt < 2500 && vs < -2500) {
        return true;
    }

    // too low, gear
    // If you're below 500 ft AGL and gears aren't fully down (16383 is full down)
    if (ralt > 20 && ralt < 500) {
        if (!flightData.gear.allDown()) {
            return true;
        }
    }

    // too low, flaps
    // If you're below 250 ft AGL and flaps aren't at a landing setting
    if (ralt > 20 && ralt < 250 && flaps_raw < 5000) {
        return true;
    }

    // pull up
    // If your sink rate is extremely aggressive (e.g., > 4,000 fpm) near the ground
    if (ralt > 20 && ralt < 1000 && vs < -4000) {
        return true;
    }

    return false;
}

