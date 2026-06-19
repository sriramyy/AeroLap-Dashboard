#ifndef AEROLAP_FLIGHTHARDWARE_H
#define AEROLAP_FLIGHTHARDWARE_H

#include "BaseHardware.h"
#include "FlightTelemetry.h"

// Selection for the current setting to modify using the rotary encoder. Toggled with button
enum rotaryMode {ApHeading, ApAltitude, ApSpeed, Flap};

struct rotaryParam {
    rotaryMode mode = ApHeading;
    float value = 0;
    float range_upper = 360;
    float range_lower = 0;
    float change_delta = 1;
    bool displayOnly = false;
};

class FlightHardware {
    BaseHardware& hw;
    FlightData& fd;
    AircraftData& ad;

    // led assignments
    // TOP BAR [2] [4] [1] [1]
    const LEDZone apZone = {Pin::TOP_BAR, 0, 2};
    const LEDZone alertZone = {Pin::TOP_BAR, 2, 4};
    const LEDZone gpwsZone = {Pin::TOP_BAR, 6, 1}; // terrain, also updated in alert function
    const LEDZone minimumsZone = {Pin::TOP_BAR, 7, 1}; // 200 ft, also updated in alert function

    // BOTTOM BAR [3-gears] [2] [1] [1] [x]
    const LEDZone gearZone = {Pin::BOTTOM_BAR, 0, 3};
    const LEDZone speedbrakeZone = {Pin::BOTTOM_BAR, 3, 2};
    const LEDZone flapsTransitioningZone = {Pin::BOTTOM_BAR, 5, 1};
    const LEDZone parkingBrakeZone = {Pin::BOTTOM_BAR, 6, 1};
    // TODO: one more led index here can make smt

    // vars for autopilot blinking
    bool lastApState = false;
    uint32_t apDisconnectTime = 0;

    // configure the rotary parameter
    rotaryParam rotary_param{};

public:
    FlightHardware(BaseHardware& hw, FlightData& fd, AircraftData& ad) : hw(hw), fd(fd), ad(ad) {}

    // when button is pressed, update the rotary mode and configure all settings
    void updateRotaryMode();
    //
    void updateRotaryValueUp();
    void updateRotaryValueDown();
    // apply the changes made to the rotary value to the actual fd
    void pushUpdatedRotaryValue();


    // actually render all the changes from buffers to the hardware
    void updateAllDisplays();
    void updateAllLights();

    // testing functions
    void printTesting() const;

private:
    // helper light functions
    // update hte gear lights individually based on if transitioning, down, up
    void updateGearLights();
    // update all alert lights at once for overspeed, master caution, master warning (also for GPWS)
    void updateAlertLights();
    // update ap lights and flash for x seconds when disconnected
    void updateAutopilotLights();
    // handle all the other lights
    void updateOtherLights();
};


#endif //AEROLAP_FLIGHTHARDWARE_H