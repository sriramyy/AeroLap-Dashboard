#ifndef AEROLAP_FLIGHTHARDWARE_H
#define AEROLAP_FLIGHTHARDWARE_H

#include "BaseHardware.h"
#include "FlightTelemetry.h"

class FlightHardware {
    BaseHardware& hw;
    FlightData& fd;
    AircraftData& ad;

    // led assignments
    // TOP BAR [2] [4] [2]
    const LEDZone apZone = {Pin::TOP_BAR, 0, 2};
    const LEDZone alertZone = {Pin::TOP_BAR, 2, 4};
    const LEDZone gpwsZone = {Pin::TOP_BAR, 6, 1}; // terrain, also updated in alert function
    const LEDZone minimumsZone = {Pin::TOP_BAR, 7, 1}; // 200 ft, also updated in alert function

    // BOTTOM BAR [3-gears] [2] [3]
    const LEDZone gearZone = {Pin::BOTTOM_BAR, 0, 3};
    const LEDZone speedbrakeZone = {Pin::BOTTOM_BAR, 3, 2};
    const LEDZone flapsTransitioningZone = {Pin::BOTTOM_BAR, 5, 1};
    const LEDZone parkingBrakeZone = {Pin::BOTTOM_BAR, 6, 1};

public:
    FlightHardware(BaseHardware& hw, FlightData& fd, AircraftData& ad) : hw(hw), fd(fd), ad(ad) {}


    void updateAllDisplays();
    void updateAllLights();

    // helper light functions
    // update hte gear lights individually based on if transitioning, down, up
    void updateGearLights();
    // update all alert lights at once for overspeed, master caution, master warning (also for GPWS)
    void updateAlertLights();
    //
    // handle all the other lights
    void updateOtherLights();
};


#endif //AEROLAP_FLIGHTHARDWARE_H