cmake_minimum_required(VERSION 4.1)
project(AeroLap)

set(CMAKE_CXX_STANDARD 20)

add_executable(AeroLap main.cpp
        src/FlightTelemetry.cpp
        include/FlightTelemetry.h
        src/RacingTelemetry.cpp
        include/RacingTelemetry.h
        include/TelemetryData.h
        src/BaseHardware.cpp
        include/BaseHardware.h
        src/FlightHardware.cpp
        include/FlightHardware.h)
