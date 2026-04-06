# AeroLap Telemetry Bridge

A high-performance telemetry interface designed to bridge legacy 32-bit flight simulation data with modern embedded hardware. This Python-based middleware extracts real-time physics state from **Microsoft Flight Simulator X** and streams it to an **ESP32** microcontroller via a custom serial protocol.

## Technical Challenges & Solutions
* **Architecture Mismatch (The "32-bit Wall"):** Resolved **IPC Error 13** by deploying a dedicated **32-bit Python 3.11 runtime**. This ensured 32-bit pointer alignment when accessing the simulator's memory map from a 64-bit OS.
* **Inter-Process Communication (IPC):** Bypassed unstable SimConnect SDKs in favor of **FSUIPC Shared Memory Mapping**. 
* **Data Serialization:** Implemented manual **bit-unpacking** using the `struct` library to translate raw 8-byte doubles and 4-byte integers into human-readable telemetry.

## Prerequisites

### Software
* **Python 3.11.x (32-bit):** **CRITICAL:** Do not use 64-bit Python; the memory bridge will fail.
* **FSUIPC4:** Required FSX Plugin (The free/unregistered version is sufficient).
* **Flight Simulator X:** (Steam Edition or Retail).

### Hardware
* **ESP32 Microcontroller** (Running the AeroLap BaseHardware logic).
* **USB Data Cable.**

## Installation

1. **Create the Compatibility Environment:**
   Navigate to the `bridge/` folder and create a 32-bit virtual environment:
   ```powershell
   & "C:\Path\To\Your\Python311-32\python.exe" -m venv venv32

2. **Install dependencies**
```
.\venv32\Scripts\activate
pip install -r requirements.txt
```