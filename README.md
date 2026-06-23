# KarmanCore Dashboard ✈️🏎️

**KarmanCore** is an intelligent, high-performance aerospace telemetry engine and embedded dashboard system. It bridges real-time simulation data from atmospheric flight simulators (MSFS/FSX) to orbital spaceflights (Kerbal Space Program) using a multi-node ESP32 architecture. 

The platform integrates on-chip **Edge AI** (TinyML) to serve as a virtual flight engineer—processing complex data streams locally to deliver predictive aerodynamic stall warnings, vehicle state classification, and intelligent flight-phase detection that drives physical annunciator alerts and digital instrumentation.

_NOTE: rest of readme has not been updated from the AeroLap (old) readme!_
## 🚀 Overview

The project consists of two primary components:
1.  **The Bridge (Python):** A multi-threaded interface that pulls high-fidelity telemetry from simulation memory (via FSUIPC for flight) and serializes it into a custom binary protocol.
2.  **The Firmware (C++ / ESP32):** A real-time embedded application that de-serializes telemetry packets to drive 7-segment displays and NeoPixel LED bars.

## 🛠️ System Architecture

AeroLap utilizes a **binary struct-packing approach** over Serial (USB-C) to achieve sub-10ms latency. Unlike traditional ASCII-based bridges, this method allows for a high-density data stream without the overhead of string parsing on the microcontroller.

* **Communication:** 115200 (or 921600) Baud Rate.
* **Packet Sync:** Magic-byte header synchronization (`0xAA 0xBB`) for robust data alignment.
* **Hardware Abstraction:** A tiered class structure separates raw telemetry processing from hardware-specific pin mapping.

## ✨ Key Features

### Flight Mode (Current Implementation)
_Implemented primarily for **FSX** using FSUIPC for bridging telemetry_
* **Annunciator Bars:** Dedicated Top and Bottom LED strips for "Master Alerts" (Warning/Caution/Overspeed) and "Configuration" (Landing Gear/Flaps/Speedbrakes).
* **Phase-of-Flight Logic:** Automated "Minimums" alerting based on Radio Altitude (AGL) and Autopilot state tracking.
* **Digital Gauges:** Triple 7-segment displays for real-time Airspeed, Altitude, and Flap Position.
* **Autopilot Adjustment:** (In development) Be able to change the state/position of the autopilot with rotary encoder (continious potentiameter) inputs.
* **Edge AI Predictive Flight Engineer**: Evaluates multi-variable physics patterns utilizing an onboard EdgeAI machine learning model in real-time to alert you to an aerodynamic stall before instrument alarms sound

### Racing Mode (In Development)
_Implemented primarily for **Assetto Corsa** using a direct serial connection for bridging telemetry_
* **Shift Lights:** Adaptive RPM-based LED color gradients.
* **Race Stats:** Lap time delta and flag status (etc. Yellow Flag).
* **Car Stats:** Gear indicator, fuel status, tire status.

## ⚙️ Firmware Technical Overview
The AeroLap firmware is written in C++ using the Arduino framework for the ESP32. It utilizes a layered architecture to separate low-level hardware control from high-level flight logic.

### Pins
Hardware assignments are centralized in `Pin.h` to ensure easy modification for different ESP32 board layouts. Key assignments include:

### BaseHardware
The `BaseHardware` class acts as the **Hardware Abstraction Layer (HAL)**. It manages the raw interface with external libraries and componenets while handling memory buffering for the outputs.

* **Abstraction:** Wraps `Adafruit_NeoPixel` and `DIYables_4Digit7Segment_74HC595` logic.
* **Buffering:** Maintains internal `LEDBar` and `DisplayBuffer` structures. This allows the system to calculate changes in memory and "render" them to the physical hardware in a single batch call, preventing flickering.
* **Methods:** Provides specialized functions like `updateLEDZone()` and `updateSegmentDisplay()` that allow higher-level classes to manipulate specific hardware regions by index rather than raw pin numbers.

### FlightHardware
`FlightHardware` contains the high-level behavioral logic of the cockpit. It maps flight-specific states (e.g., "Gears Transitioning") to physical colors and numbers.
* **Alert Precedence:** Implements a hierarchy for the `alertZone`. A Master Warning (Red Blink) will override a Master Caution (Yellow), ensuring the most critical information is always prioritized.
* **LED Mapping:**

| Index | Zone / Function | Logic & Color |
| :--- | :--- | :--- |
| **0, 1** | **Autopilot Master** | **Green**: Active \| **Red Blink (3s)**: Disconnected |
| **2-5** | **Priority Alerts** | **Red Blink**: Master Warning \| **Yellow**: Caution \| **Blue**: Overspeed |
| **6** | **GPWS / Terrain** | **Yellow Blink**: Terrain/Sink Rate warning active |
| **7** | **Minimums (DH)** | **Yellow**: Aircraft is below 200ft AGL (Decision Height) |
| **--** | **--- Bottom Bar ---** | **----------------------------------------------** |
| **0-2** | **Landing Gear** | **Green**: Locked Down \| **Yellow**: In Transition |
| **3** | **Speedbrake Armed**| **White**: Spoilers armed for auto-deployment |
| **4** | **Speedbrake Ext** | **Yellow**: Spoilers/Speedbrakes physically deployed |
| **5** | **Flap Transition** | **Yellow**: Flap surfaces are currently in motion |
| **6** | **Parking Brake** | **Red**: Parking brake is engaged |
| **7** | **Data Heartbeat** | **Blue Blink**: Valid telemetry packet received |

#### 7-segment Displays
The board features three 4-digit 7-segment displays. In flight mode, they are configured to be the following:
* LEFT Display - **Airspeed** (knots)
* CENTER Display - Currently modifying setting (See below)
* RIGHT Display - **Altitude** (ft, where 3,500 ft -> 3.50 k ft)

The center display works in conjunction with a **rotary encoder** and a **button** to be able to modify various settings in the airplane (mostly autopilot settings), this gives more granular control over settings that require it.
\
The goal is to be able to use the button to select a setting to configure, seen by the leftmost digit of the 7-segment display (see the key below), then the other digits show the current value of that field. There are also display-only fields, marked with **(D)**, that can be switched to using the button for the center display.
| Setting | Index 0 Key | Value Range | Change Delta | Display Example (1+3) |
| :--- | :--- | :--- | :--- | :--- |
| Autopilot Heading | H | 0-360 degrees | 1 | H250 |
| Autopilot Altitude | A | 0-x ft | 500 | A035 -> 3,500 ft , A100 -> 10,000 ft |
| Autopilot Speed | S | 0-x knots | 25 | S200 |
| (D) Flap | F | 0-~4 | N/A | F  4 |

*change delta means one click of the rotary encoder changes the value by x*


### FlightTelemetry
The `FlightTelemetry` class manages the communication contract between the Python Bridge and the ESP32.

* **Binary Protocol:** Uses a packed 55-byte C-struct to minimize overhead. Data is transmitted in a fixed-width binary format to eliminate the latency of string parsing.
* **Synchronization:** Implements a "Magic Byte" handshake. The system peeks for `0xAA` (Header) and validates `0xBB` (Footer) to ensure the serial buffer is perfectly aligned before processing. Throws it out if it is not formatted correctly to avoid processing junk data. 
* **State Conversion:** Converts raw simulator values into usable flight enums:
    * **Gear:** `0` or `16383` are mapped to `UP` or `DOWN`, while intermediate values trigger the `TRANSITIONING` state.
    * **Flaps:** Scaled from raw sim units ($0$ to $16383$) to a standard $0$ to $8$ index.

### Flight Data Flow
_From the Raw Bytes being sent from the simulator to the Hardware Pin_
Flight data arrives to the FSUIPC bridge then is translated to be sent to the ESP32 via the **Python Bridge**. From the bridge, flight data makes it to **FlightTelemetry** which takes the data and decodes the raw bytes to the custom data structure that contains the various states to be tracked and displayed on the hardware. After this translation, the data, now wrapped in the custom data structure, makes it to the **FlightHardware** class which takes it and sets the **BaseHardware** objects depending on what values map to what hardware outputs. Finally, **BaseHardware** communicates with the individual pins and sends custom byte messages to set the LEDs or 7-segment displays to display desired result. 

### RacingHardware
To be implemented. High level behavioral logic for the vehicle. 

### RacingTelemetry
To be implemented. Contains the communication contract between the bridge and the ESP32, managing high level states and values to send to the RacingHardware class.

## 💻 Tech Stack

* **Firmware:** C++, Arduino Core (ESP32), Adafruit NeoPixel, DIYables 7-Segment.
* **Bridge:** Python 3.x, `pyfsuipc` (MSFS/FSX Interface), `pyserial`.
  * **IMPORTANT NOTE**: When using the bridge you **must** have a **32-bit** install of Python if using FSX as FSUIPC communicates packages utilizing 32-bit architecture.
* **Tools:** PlatformIO / CLion, VSCode

## 🎯 Project Goals
The goal of AeroLap is to make a personal project that provides an extensible hardware platform that can adapt to different simulation genres on the fly. Future iterations aim to include automatic game detection and a **custom PCB** for a unified "plug-and-play" dashboard experience.

_Sriram Yerramsetty, 2026_
