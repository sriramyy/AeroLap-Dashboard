import time
import struct
import serial
from fsuipc import FSUIPC
from dataclasses import dataclass

@dataclass
class BridgeFlightData:
    # 7 Floats
    alt_ft: float; speed: float; vertical_speed: float; radio_alt: float
    pitch: float; roll: float; heading: float
    # 1 Short
    flap_actual: int
    # 3 Shorts
    gear_nose: int; gear_left: int; gear_right: int
    # 8 Booleans
    speedbrakes_ext: bool; parking_brake: bool
    masterWarning: bool; masterCaution: bool; overspeed: bool; gpws: bool
    ap_active: bool; flaps_moving: bool
    # 2 Floats
    ap_heading: float; ap_alt: float

def pack_flight_data(data):
    """ Packs 54-byte binary payload for ESP32 """
    return struct.pack(
        '<BB 7f 1h 3H 8? 2f', 
        0xAA, 0xBB,
        data.alt_ft, data.speed, data.vertical_speed, data.radio_alt,
        data.pitch, data.roll, data.heading,
        data.flap_actual,
        data.gear_nose, data.gear_left, data.gear_right,
        data.speedbrakes_ext, data.parking_brake,
        data.masterWarning, data.masterCaution, data.overspeed, data.gpws,
        data.ap_active, data.flaps_moving,
        data.ap_heading, data.ap_alt
    )

def run_bridge():
    ser = serial.Serial('COM3', 115200, timeout=1)
    prev_flap_pos = 0; last_move_time = 0; COOLDOWN_TIME = 0.4
    
    print("\033[H\033[J", end="") 
    print("="*50 + "\n   AEROLAP TELEMETRY BRIDGE v2.1\n" + "="*50)
    
    try:
        with FSUIPC() as f:
            print("SIM STATUS: Connected ✅\n")
            while True:
                # read
                raw = f.read([
                    (0x0570, 8), (0x02BC, 4), (0x02C8, 4), (0x31E4, 8), # [0-3]
                    (0x0578, 4), (0x057C, 4), (0x0580, 4),             # [4-6]
                    (0x0BE4, 4),                                       # [7] Flaps
                    (0x0BEC, 4), (0x0BF4, 4), (0x0BF0, 4),             # [8-10] Gears (N, L, R)
                    (0x0BD4, 4), (0x0BC8, 2),                          # [11-12] Brakes
                    (0x0D6C, 2), (0x036D, 1),                          # [13-14] Warnings
                    (0x3300, 2),                                       # [15] GPWS
                    (0x07BC, 4), (0x07CC, 2), (0x07D4, 4)              # [16-18] AP (Active, Hdg, Alt)
                ])

                now = time.time()
                DEG_CONV = 65536.0 * 65536.0

                # movement Logic
                current_flap_raw = int(struct.unpack('i', raw[7])[0])
                if current_flap_raw != prev_flap_pos: last_move_time = now
                is_moving = (now - last_move_time) < COOLDOWN_TIME
                prev_flap_pos = current_flap_raw

                # unpacking
                fd = BridgeFlightData(
                    alt_ft = (struct.unpack('q', raw[0])[0] / DEG_CONV) * 3.28084,
                    speed = struct.unpack('i', raw[1])[0] / 128.0,
                    vertical_speed = (struct.unpack('i', raw[2])[0] * 60 * 3.28084) / 256.0,
                    radio_alt = struct.unpack('d', raw[3])[0] * 3.28084,
                    pitch = (struct.unpack('i', raw[4])[0] * 360.0) / DEG_CONV,
                    roll = (struct.unpack('i', raw[5])[0] * 360.0) / DEG_CONV,
                    heading = (struct.unpack('i', raw[6])[0] * 360.0) / DEG_CONV,
                    
                    flap_actual = current_flap_raw,
                    
                    gear_nose  = struct.unpack('H', raw[8][:2])[0],
                    gear_left  = struct.unpack('H', raw[9][:2])[0],
                    gear_right = struct.unpack('H', raw[10][:2])[0],
                    
                    speedbrakes_ext = struct.unpack('i', raw[11])[0] > 0,
                    parking_brake   = struct.unpack('H', raw[12])[0] > 0, 
                    
                    masterWarning = bool(struct.unpack('H', raw[13])[0] & 0x01),
                    masterCaution = bool(struct.unpack('H', raw[13])[0] & 0x02),
                    overspeed     = bool(struct.unpack('b', raw[14])[0]),
                    gpws          = bool(struct.unpack('H', raw[15])[0] & 0x01),
                    
                    ap_active    = struct.unpack('i', raw[16])[0] == 1,
                    flaps_moving = is_moving,
                    
                    ap_heading = struct.unpack('H', raw[17])[0] * 360.0 / 65536.0,
                    ap_alt = (struct.unpack('i', raw[18])[0] / 65536.0) * 3.28084
                )

                ser.write(pack_flight_data(fd))

                # debug dashboard
                print("\033[H\033[J", end="") 
                print(f"==================================================")
                print(f"        AEROLAP TELEMETRY DASHBOARD v2.1          ")
                print(f"==================================================")
                print(f" [MOVEMENT]  ALT: {fd.alt_ft:6.0f} | SPD: {fd.speed:6.1f} | VS: {fd.vertical_speed:5.0f}")
                print(f" [ATTITUDE]  HDG: {fd.heading:6.1f}° | PITCH: {fd.pitch:5.1f}° | ROLL: {fd.roll:5.1f}°")
                print(f"--------------------------------------------------")
                print(f" [SURFACES]  FLAPS: {fd.flap_actual:5} / 16383 | {'[MOVING]' if fd.flaps_moving else '[STATIC]'}")
                print(f"             GEARS: L:{fd.gear_left:5}  N:{fd.gear_nose:5}  R:{fd.gear_right:5}")
                print(f"--------------------------------------------------")
                print(f" [SYSTEMS]   BRAKE: {'[PARKED]' if fd.parking_brake else '[OFF]   '} | SPEEDBRAKE: {'[EXT]' if fd.speedbrakes_ext else '[RET]'}")
                print(f"             ALERTS: {'[WARN]' if fd.masterWarning else ''} {'[CAUT]' if fd.masterCaution else ''} {'[OVRSPD]' if fd.overspeed else ''} {'[GPWS]' if fd.gpws else ''}")
                print(f"--------------------------------------------------")
                print(f" [AUTOPILOT] STATUS: {'[ACTIVE]' if fd.ap_active else '[OFF]   '}")
                print(f"             TARGET: HDG {fd.ap_heading:3.0f}° | ALT {fd.ap_alt:5.0f} ft")
                print(f"==================================================")

                time.sleep(0.05) 

    except Exception as e:
        print(f"\nCRITICAL ERROR: {e}")

if __name__ == "__main__":
    run_bridge()