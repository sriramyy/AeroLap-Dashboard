#ifndef AEROLAP_FLIGHTTELEMETRY_H
#define AEROLAP_FLIGHTTELEMETRY_H

#include "TelemetryData.h"

// ---------------- SENDING COMMANDS TO THE SIM ----------------
// Unique IDs for every action you want to perform in the sim
enum CommandID : uint8_t {
    CMD_NONE = 0,
    AP_MASTER_SET = 1,
    AP_HDG_SET = 2,
    AP_ALT_SET = 3,
    GEAR_TOGGLE = 4,
    FLAPS_UP = 5,
    FLAPS_DOWN = 6
};

struct __attribute__((packed)) CommandPacket {
    uint8_t magic1 = 0xCC; // Distinct magic bytes for commands
    uint8_t magic2 = 0xDD;
    uint8_t commandID;     // Use the enum here
    float value;           // The parameter (e.g., the actual heading)
};

// ---------------- RECEIVING DATA FROM THE SIM ----------------
// format that data is being sent from the Python bridge to the ESP32
struct __attribute__((packed)) TelemetryPacket {
    uint8_t magic1 = 0xAA;
    uint8_t magic2 = 0xBB;

    // Floats (4 bytes each)
    float alt_ft, speed, v_speed, radio_alt;
    float pitch, roll, heading;

    // Integers (2 bytes)
    int16_t flaps_raw; // (0-16383)

    // individual gear position
    uint16_t gear_nose, gear_left, gear_right;

    // Booleans (1 byte each in this context)
    bool speedbrakes_ext, parking_brake;
    bool masterWarning, masterCaution, overspeed, gpws; // gpws ignored, done manually
    bool ap_active;
    bool flaps_moving;

    // Autopilot Floats
    float ap_heading, ap_alt;
};

// ---------------- FLIGHT TELEM ----------------

class FlightTelemetry {
public:
    AircraftData aircraftData;
    FlightData flightData; // local copy of the flight data

public:
    // Receiving changes FROM the simulator
    // void syncAircraftData(const AircraftData& aircraft_data);
    void updateFromSim(TelemetryPacket& incoming);

    // Sending changes TO the simulator
    void updateAutopilotHeading(float heading);
    void updateAutopilotStatus(bool active);


private:
    // sends the change to the simulator
    void pushCommand(CommandID commandID, float value);

    // helper for gear conversion
    static GearPositon convertToGearPosition(uint16_t raw_pos);
    // helper for flap position (0-8)
    static int convertToFlapPosition(uint16_t raw_pos);
    // helper for gpws
    bool determineGPWS(float ralt, float vs, uint16_t flaps_raw);

};


#endif //AEROLAP_FLIGHTTELEMETRY_H