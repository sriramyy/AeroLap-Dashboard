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

enum GearPositon {DOWN, TRANSITIONING, UP}; // DOWN (deployed) or UP (retracted)

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
    bool flapsMoving;
    Gear gear;
    bool speedbrakes; // true->extended (active)

    bool masterWarning;
    bool masterCaution;
    bool overspeed;
    bool gpws; // terrain warning

    bool minimums; // calc from radio alt (200)

    Autopilot autopilot;

    [[nodiscard]] bool isAutopilotActive() const {
        return autopilot.active;
    }

    // checks masterCaution, masterWarning, and overspeed
    [[nodiscard]] bool isAlert() const {
        return masterCaution || masterWarning || overspeed;
    }

    // true means extended/down/landing/takeoff/taxi
    [[nodiscard]] bool isGearDown() const {
        return gear.front == DOWN && gear.rearLeft == DOWN && gear.rearRight == DOWN;
    }
};

// ---------------------------- RACING ---------------------------- //

enum FlagType {NO_FLAG, BLUE_FLAG, YELLOW_FLAG, RED_FLAG};

struct VehicleData {
    string manufacturer; // manufacturer of the car
    string model; // model of the car

    int rpmMax; // maximum rpm / redline
    int gearMax; // max gear
};

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
    FlagType flag;

    [[nodiscard]] bool isFlagActive() const {
        return flag != FlagType::NO_FLAG;
    }
};
