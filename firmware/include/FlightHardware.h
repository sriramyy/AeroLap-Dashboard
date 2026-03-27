#ifndef AEROLAP_FLIGHTHARDWARE_H
#define AEROLAP_FLIGHTHARDWARE_H

#include "BaseHardware.h"
#include "FlightTelemetry.h"

class FlightHardware {
    BaseHardware& hw;
    FlightData& fd;
    AircraftData& ad;

    FlightHardware(BaseHardware& hw, FlightData& fd, AircraftData& ad) : hw(hw), fd(fd), ad(ad) {}

    void updateAllDisplays();







};


#endif //AEROLAP_FLIGHTHARDWARE_H