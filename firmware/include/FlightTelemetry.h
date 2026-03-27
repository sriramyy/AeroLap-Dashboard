#ifndef AEROLAP_FLIGHTTELEMETRY_H
#define AEROLAP_FLIGHTTELEMETRY_H

#include "TelemetryData.h"

class FlightTelemetry {
    AircraftData aircraftData;
    FlightData flightData; // local copy of the flight data

public:
    // Receiving changes FROM the simulator
    void syncAircraftData(const AircraftData& aircraft_data);
    void syncFromSim(const FlightData& incoming);

    // Sending changes TO the simulator
    // update ap heading
    void updateAutopilotHeading(float heading);
    void updateAutopilotStatus(bool active);


private:
    // sends the change to the simulator
    void pushCommand(string commandID, float value);
};


#endif //AEROLAP_FLIGHTTELEMETRY_H