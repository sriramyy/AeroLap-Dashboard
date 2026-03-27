#pragma once

#include <string>

using std::string;

// ---------------------------- FLIGHT ---------------------------- //

// data specific to the aircraft type
struct AircraftData {
    string manufacturer; // manufacturer of the aircraft
    string model; // model of the aircraft

    int flapFull; // maximum flap position
    float maxSpeed; // maximum airspeed in knots
};

enum GearPositon {DOWN, UP}; // DOWN (deployed) or UP (retracted)

struct Gear {
    GearPositon front;
    GearPositon rearLeft;
    GearPositon rearRight;
};

struct Autopilot {
    bool active;
    bool nav; // true->nav , false->gps

    float heading;
    float altitude;
};

// each flight data package, actual telemetry
struct FlightData {
    float altitude;
    float airspeed;
    float verticalSpeed;

    float pitch;
    float roll;
    float heading;

    int flapPosition;
    Gear gear;
    bool speedbrakes; // true->extended (active)

    bool masterWarning;
    bool masterCaution;
    Autopilot autopilot;
};

// ---------------------------- RACING ---------------------------- //

struct RacingData {
    int gear; // 0->N , -1->REV
    int rpm;
    float speed;

    int position;

    int lapsCompleted;
    int lapsTotal;
    float lastLaptime;
    float delta;

    bool abs;
    bool flag;
};