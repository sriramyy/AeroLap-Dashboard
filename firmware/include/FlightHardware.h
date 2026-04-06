#ifndef AEROLAP_FLIGHTHARDWARE_H
#define AEROLAP_FLIGHTHARDWARE_H

#include "BaseHardware.h"
#include "FlightTelemetry.h"

enum DisplayMode {};

class FlightHardware {
    BaseHardware& hw;
    FlightData& fd;
    AircraftData& ad;

    FlightHardware(BaseHardware& hw, FlightData& fd, AircraftData& ad) : hw(hw), fd(fd), ad(ad) {}

    // led assignments
    // TOP BAR [2] [4] [2]
    const LEDZone someZone = {Pin::TOP_BAR, 0, 2};
    const LEDZone alertZone = {Pin::TOP_BAR, 2, 4};
    const LEDZone otherZone = {Pin::TOP_BAR, 5, 2};

    // BOTTOM BAR [3-gears] [2] [3]
    const LEDZone gearZone = {Pin::BOTTOM_BAR, 0, 3};
    const LEDZone speedbrakeZone = {Pin::BOTTOM_BAR, 2, 2};

    void updateAllDisplays();
    void updateAllLights();

    // helper light functions
    // update hte gear lights individually based on if transitioning, down, up
    void updateGearLights();
    // update all alert lights at once for overspeed, master caution, master warning
    void updateAlertLights();
    // handle all the other lights
    void updateOtherLights();
};


#endif //AEROLAP_FLIGHTHARDWARE_H