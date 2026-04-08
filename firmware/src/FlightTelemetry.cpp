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
    flightData.flapPosition = convertToFlapPosition(incoming.flap_actual);
    flightData.flapsMoving = abs(incoming.flap_actual - incoming.flap_handle) > 50;

    // convert gear pos
    flightData.gear.front = convertToGearPosition(incoming.gear_nose);
    flightData.gear.rearLeft = convertToGearPosition(incoming.gear_left);
    flightData.gear.rearRight = convertToGearPosition(incoming.gear_right);

    // alerts
    flightData.masterWarning = incoming.masterWarning;
    flightData.masterCaution = incoming.masterCaution;
    flightData.overspeed = incoming.overspeed;
    flightData.gpws = incoming.gpws;

    flightData.minimums = incoming.radio_alt <= 200; // trigger below 200 ft

    // autopilot
    flightData.autopilot = {incoming.ap_active, incoming.ap_nav_mode, incoming.ap_heading, incoming.ap_alt};
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
