#include "../include/FlightTelemetry.h"

void FlightTelemetry::syncAircraftData(const AircraftData &aircraft_data) {
    aircraftData = aircraft_data;
}

void FlightTelemetry::syncFromSim(const FlightData &incoming) {
    flightData = incoming;
}

void FlightTelemetry::updateAutopilotHeading(float heading) {
    flightData.autopilot.heading = heading;
    pushCommand("AP_HDG_SET", heading);
}

void FlightTelemetry::updateAutopilotStatus(bool active) {
    flightData.autopilot.active = active;
}

void FlightTelemetry::pushCommand(string commandID, float value) {

}


