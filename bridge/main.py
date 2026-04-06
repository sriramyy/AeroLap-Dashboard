import time
import struct
import serial  # You'll need: pip install pyserial
from fsuipc import FSUIPC
from dataclasses import dataclass

@dataclass
class BridgeFlightData:
    alt_ft: float
    speed: float
    vertical_speed: float
    pitch: float
    roll: float
    heading: float
    flap_pos: int
    # Updated to 3 gears to match our binary struct
    gear_nose: int
    gear_left: int
    gear_right: int
    speedbrakes_ext: bool
    masterWarning: bool
    masterCaution: bool
    overspeed: bool
    ap_active: bool
    ap_nav_mode: bool
    ap_heading: float
    ap_alt: float

def pack_flight_data(data):
    # Format: 2xByte, 6xFloat, 1xInt, 3xUnsignedShort, 6xBool, 2xFloat
    return struct.pack(
        '<BB6fi3H6?2f', 
        0xAA, 0xBB,
        data.alt_ft, data.speed, data.vertical_speed,
        data.pitch, data.roll, data.heading,
        data.flap_pos,
        data.gear_nose, data.gear_left, data.gear_right,
        data.speedbrakes_ext,
        data.masterWarning, data.masterCaution, data.overspeed,
        data.ap_active, data.ap_nav_mode,
        data.ap_heading, data.ap_alt
    )

def run_bridge():
    # REPLACE 'COM3' with your actual ESP32 port (e.g., /dev/ttyUSB0 on Mac/Linux)
    ser = serial.Serial('COM3', 115200, timeout=1)
    
    print("\n" + "="*40)
    print("   AEROLAP FULL TELEMETRY BRIDGE   ")
    print("="*40)
    
    try:
        with FSUIPC() as f:
            print("SUCCESS: Linked to FSX/MSFS Shared Memory.\n")

            while True:
                # 1. THE BIG PACK READ
                raw_data = f.read([
                    (0x0570, 8), (0x02BC, 4), (0x02C8, 4), # General
                    (0x0578, 4), (0x057C, 4), (0x0580, 4), # Axis
                    (0x0BE0, 4),                           # Flaps
                    (0x0BEC, 4), (0x0BE8, 4), (0x0BD0, 4), # Gears (Nose, Left, Right)
                    (0x0BD4, 4),                           # Speedbrakes
                    (0x0D6C, 2), (0x036D, 1),              # Warnings
                    (0x07BC, 4), (0x132C, 1), (0x07CC, 2), (0x07D0, 4) # AP
                ])

                DEG_CONV = 65536.0 * 65536.0

                # 2. PROCESSING
                flight_data = BridgeFlightData(
                    alt_ft = (struct.unpack('q', raw_data[0])[0] / DEG_CONV) * 3.28084,
                    speed = struct.unpack('i', raw_data[1])[0] / 128.0,
                    vertical_speed = (struct.unpack('i', raw_data[2])[0] * 60 * 3.28084) / 256.0,
                    pitch = (struct.unpack('i', raw_data[3])[0] * 360.0) / DEG_CONV,
                    roll = (struct.unpack('i', raw_data[4])[0] * 360.0) / DEG_CONV,
                    heading = (struct.unpack('i', raw_data[5])[0] * 360.0) / DEG_CONV,
                    flap_pos = struct.unpack('i', raw_data[6])[0],
                    # Map 0-16383 gears
                    gear_nose = struct.unpack('i', raw_data[7])[0],
                    gear_left = struct.unpack('i', raw_data[8])[0],
                    gear_right = struct.unpack('i', raw_data[9])[0],
                    speedbrakes_ext = struct.unpack('i', raw_data[10])[0] > 0,
                    masterWarning = bool(struct.unpack('H', raw_data[11])[0] & 0x01),
                    masterCaution = bool(struct.unpack('H', raw_data[11])[0] & 0x02),
                    overspeed = bool(struct.unpack('b', raw_data[12])[0]),
                    ap_active = struct.unpack('i', raw_data[13])[0] == 1,
                    ap_nav_mode = struct.unpack('b', raw_data[14])[0] == 0,
                    ap_heading = struct.unpack('H', raw_data[15])[0] * 360.0 / 65536.0,
                    ap_alt = (struct.unpack('i', raw_data[16])[0] * 3.28084) / 65536.0
                )

                # 3. SEND TO ESP32
                binary_packet = pack_flight_data(flight_data)
                ser.write(binary_packet)

                print(f"ALT: {int(flight_data.alt_ft):5} | SPD: {int(flight_data.speed):3} | AP: {'ON' if flight_data.ap_active else 'OFF'}", end='\r')
                time.sleep(0.05) 

    except Exception as e:
        print(f"\nCRITICAL ERROR: {e}")

if __name__ == "__main__":
    run_bridge()