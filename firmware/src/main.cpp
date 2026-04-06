#include <Arduino.h>

#include "FlightHardware.h"
#include "../include/FlightTelemetry.h"
#include "../include/BaseHardware.h"
#include "../include/Pin.h"

// ------------------- MODE MANAGEMENT -------------------
enum DeviceMode { FLIGHT, RACING };
DeviceMode currentMode = FLIGHT;

// ------------------- TELEMETRY OBJECTS -------------------
FlightTelemetry flightTelem;
BaseHardware hw;
FlightHardware flightHW(hw, flightTelem.flightData, flightTelem.aircraftData);

// ------------------- FUNCTION PROTOTYPES -------------------
void handleFlightLoop();
void handleRacingLoop();
void updateFlightHardware();

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        delay(10);
    }

    hw.begin(10);


    Serial.println("AEROLAP SYSTEM BOOTED: FLIGHT MODE ACTIVE");
}

void loop() {
    switch (currentMode) {
        case FLIGHT:
            handleFlightLoop();
            break;

        case RACING:
            handleRacingLoop();
            break;
    }

    hw.displayLEDs();
    hw.displaySegmentDisplays();
}

// ------------------- FLIGHT MODE LOGIC -------------------
void handleFlightLoop() {
    if (Serial.available() >= sizeof(TelemetryPacket)) {

        if (Serial.peek() == 0xAA) {

            TelemetryPacket incoming;

            Serial.readBytes((uint8_t*)&incoming, sizeof(TelemetryPacket));

            if (incoming.magic2 == 0xBB) {
                flightTelem.updateFromSim(incoming);

                updateFlightHardware();
            }
        } else {
            Serial.read();
        }
    }
}

// ------------------- RACING MODE LOGIC -------------------
void handleRacingLoop() {

}

// ------------------- HARDWARE DRIVERS -------------------
void updateFlightHardware() {
    flightHW.updateAllLights();
    flightHW.updateAllDisplays();
}