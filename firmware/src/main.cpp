#include <Arduino.h>

#include "FlightHardware.h"
#include "../include/FlightTelemetry.h"
#include "../include/BaseHardware.h"
#include "../include/Pin.h"

// ------------------- MODE MANAGEMENT -------------------
enum DeviceMode { FLIGHT, RACING, TESTING };

// helper to convert to a string
string deviceModeToString(const DeviceMode mode) {
    if (mode == FLIGHT) return "FLIGHT";
    if (mode == RACING) return "RACING";
    if (mode == TESTING) return "[ TESTING ]";
    return "null";
}

DeviceMode currentMode = FLIGHT;

// ------------------- TELEMETRY OBJECTS -------------------
FlightTelemetry flightTelem;
BaseHardware hw;
FlightHardware flightHW(hw, flightTelem.flightData, flightTelem.aircraftData);

// ------------------- FUNCTION PROTOTYPES -------------------
void handleFlightLoop();
void handleRacingLoop();
void handleTestingLoop();
void updateFlightHardware();

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        delay(10);
    }

    hw.begin(10);


    Serial.println("AEROLAP SYSTEM ACTIVE");
    Serial.print(deviceModeToString(currentMode).c_str());
    Serial.println(" MODE ACTIVE");
}

void loop() {
    // handle specific case
    switch (currentMode) {
        case FLIGHT:
            handleFlightLoop();
            break;

        case RACING:
            handleRacingLoop();
            break;

        case TESTING:
            handleTestingLoop();
            break;
    }

    // RENDER all changes to the actual outputs
    hw.displayLEDs();
    hw.displaySegmentDisplays();
}

// ------------------- FLIGHT MODE LOGIC -------------------
void handleFlightLoop() {
    // enough data for an actual packet
    if (Serial.available() >= sizeof(TelemetryPacket)) {

        // check first magic
        if (Serial.peek() == 0xAA) {
            TelemetryPacket incoming;
            Serial.readBytes(reinterpret_cast<uint8_t *>(&incoming), sizeof(TelemetryPacket));

            // check second magic
            if (incoming.magic2 == 0xBB) {
                flightTelem.updateFromSim(incoming);
                updateFlightHardware();

                // toggle onboard whenever packet comes
                digitalWrite(Pin::ONBOARD_LED, !digitalRead(Pin::ONBOARD_LED));
            }
        } else {
            // bypass, useless doesnt match
            Serial.read();
        }
    }
}

// ------------------- RACING MODE LOGIC -------------------
void handleRacingLoop() {

}

// ------------------- TESTING -------------------
// for flight testing, prints values received from packet to the console
void handleTestingLoop() {
    handleFlightLoop();

    // dont spam console
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 500) {
        flightHW.printTesting();
        lastPrint = millis();
    }
}

// ------------------- HARDWARE DRIVERS -------------------
void updateFlightHardware() {
    flightHW.updateAllLights();
    flightHW.updateAllDisplays();
}