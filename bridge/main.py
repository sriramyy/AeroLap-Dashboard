import time
import struct
import serial
from fsuipc import FSUIPC
from dataclasses import dataclass

@dataclass
class BridgeFlightData:
    # 7 Floats
    alt_ft: float
    speed: float
    vertical_speed: float
    radio_alt: float
    pitch: float
    roll: float
    heading: float
    
    # 2 Signed Shorts (int16)
    flap_handle: int
    flap_actual: int
    
    # 3 Unsigned Shorts (uint16)
    gear_nose: int
    gear_left: int
    gear_right: int
    
    # 8 Booleans (Now includes Parking Brake)
    speedbrakes_ext: bool
    parking_brake: bool
    masterWarning: bool
    masterCaution: bool
    overspeed: bool
    gpws: bool
    ap_active: bool
    ap_nav_mode: bool
    
    # 2 Floats
    ap_heading: float
    ap_alt: float

def pack_flight_data(data):
    """
    Packs data into the 56-byte binary contract.
    Format: < (Little Endian)
    BB      : Magic Bytes (0xAA, 0xBB)
    7f      : alt_ft, speed, v_speed, radio_alt, pitch, roll, heading
    2h      : flap_handle, flap_actual
    3H      : gear_nose, gear_left, gear_right
    8?      : speedbrakes, parking_brake, warn, caution, overspeed, gpws, ap_active, ap_nav
    2f      : ap_heading, ap_alt
    """
    return struct.pack(
        '<BB 7f 2h 3H 8? 2f', 
        0xAA, 0xBB,
        data.alt_ft, data.speed, data.vertical_speed, data.radio_alt,
        data.pitch, data.roll, data.heading,
        data.flap_handle, data.flap_actual,
        data.gear_nose, data.gear_left, data.gear_right,
        data.speedbrakes_ext, data.parking_brake,
        data.masterWarning, data.masterCaution, data.overspeed, data.gpws,
        data.ap_active, data.ap_nav_mode,
        data.ap_heading, data.ap_alt
    )

def run_bridge():
    # COM PORT HERE
    ser = serial.Serial('COM3', 115200, timeout=1)
    
    print("\n" + "="*40)
    print("   AEROLAP TELEMETRY BRIDGE v1.2   ")
    print("="*40)
    
    try:
        with FSUIPC() as f:
            print("SUCCESS: Connected to Sim.\n")

            while True:
                # READ RAW
                raw = f.read([
                    (0x0570, 8), (0x02BC, 4), (0x02C8, 4), (0x31E4, 8), # General
                    (0x0578, 4), (0x057C, 4), (0x0580, 4),             # Axis
                    (0x0BE0, 4), (0x0BE4, 4),                          # Flaps
                    (0x0BEC, 4), (0x0BE8, 4), (0x0BD0, 4),             # Gears
                    (0x0BD4, 4), (0x0BC8, 2),                          # SpdBrakes, ParkingBrake (0x0BC8)
                    (0x0D6C, 2), (0x036D, 1),                          # Warnings
                    (0x3300, 2),                                       # GPWS/Alerts
                    (0x07BC, 4), (0x132C, 1), (0x07CC, 2), (0x07D0, 4) # AP
                ])

                DEG_CONV = 65536.0 * 65536.0

                # UNPACK AND CONVERT
                flight_data = BridgeFlightData(
                    alt_ft = (struct.unpack('q', raw[0])[0] / DEG_CONV) * 3.28084,
                    speed = struct.unpack('i', raw[1])[0] / 128.0,
                    vertical_speed = (struct.unpack('i', raw[2])[0] * 60 * 3.28084) / 256.0,
                    radio_alt = struct.unpack('d', raw[3])[0] * 3.28084,
                    
                    pitch = (struct.unpack('i', raw[4])[0] * 360.0) / DEG_CONV,
                    roll = (struct.unpack('i', raw[5])[0] * 360.0) / DEG_CONV,
                    heading = (struct.unpack('i', raw[6])[0] * 360.0) / DEG_CONV,
                    
                    flap_handle = int(struct.unpack('i', raw[7])[0]),
                    flap_actual = int(struct.unpack('i', raw[8])[0]),
                    
                    gear_nose = int(struct.unpack('i', raw[9])[0]),
                    gear_left = int(struct.unpack('i', raw[10])[0]),
                    gear_right = int(struct.unpack('i', raw[11])[0]),
                    
                    speedbrakes_ext = struct.unpack('i', raw[12])[0] > 0,
                    parking_brake = struct.unpack('H', raw[13])[0] > 0, 
                    
                    masterWarning = bool(struct.unpack('H', raw[14])[0] & 0x01),
                    masterCaution = bool(struct.unpack('H', raw[14])[0] & 0x02),
                    overspeed = bool(struct.unpack('b', raw[15])[0]),
                    gpws = bool(struct.unpack('H', raw[16])[0] & 0x01),
                    
                    ap_active = struct.unpack('i', raw[17])[0] == 1,
                    ap_nav_mode = struct.unpack('b', raw[18])[0] == 0,
                    
                    ap_heading = struct.unpack('H', raw[19])[0] * 360.0 / 65536.0,
                    ap_alt = (struct.unpack('i', raw[20])[0] * 3.28084) / 65536.0
                )

                # ship it to serial
                ser.write(pack_flight_data(flight_data))

                # debug print
                print("\033[H\033[J", end="") 
                print(f"==========================================")
                print(f"        AEROLAP TELEMETRY DASHBOARD       ")
                print(f"==========================================")
                print(f" [MOVEMENT]")
                print(f"  ALT: {flight_data.alt_ft:6.0f} ft  |  RALT: {flight_data.radio_alt:6.0f} ft")
                print(f"  SPD: {flight_data.speed:6.1f} kt  |  V/S:  {flight_data.vertical_speed:6.0f} fpm")
                print(f"  HDG: {flight_data.heading:6.1f}°  |  P/R:  {flight_data.pitch:5.1f}° / {flight_data.roll:5.1f}°")
                print(f"------------------------------------------")
                print(f" [SURFACES]")
                print(f"  FLAPS:  Handle({flight_data.flap_handle:5}) Actual({flight_data.flap_actual:5})")
                print(f"  GEARS:  N:{flight_data.gear_nose:5} L:{flight_data.gear_left:5} R:{flight_data.gear_right:5}")
                print(f"------------------------------------------")
                print(f" [SYSTEMS]")
                print(f"  BRAKE:  {'[PARKED]' if flight_data.parking_brake else '[OFF]   '} | SBRAKE: {'[EXT]' if flight_data.speedbrakes_ext else '[RET]'}")
                print(f"  ALERTS: {'[WARN] ' if flight_data.masterWarning else ''}{'[CAUT] ' if flight_data.masterCaution else ''}{'[OVRSPD] ' if flight_data.overspeed else ''}{'[GPWS]' if flight_data.gpws else ''}")
                print(f"------------------------------------------")
                print(f" [AUTOPILOT]")
                print(f"  STATUS: {'[ACTIVE]' if flight_data.ap_active else '[OFF]   '} | NAV: {'[NAV]' if flight_data.ap_nav_mode else '[HDG]'}")
                print(f"  TARGET: HDG {flight_data.ap_heading:3.0f}° | ALT {flight_data.ap_alt:5.0f} ft")
                print(f"==========================================")


                time.sleep(0.05) 

    except Exception as e:
        print(f"\nCRITICAL ERROR: {e}")

if __name__ == "__main__":
    run_bridge()