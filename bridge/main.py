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
    # Try to connect to ESP32
    ser = None
    esp32_connected = False
    
    try:
        ser = serial.Serial('COM3', 115200, timeout=1)
        esp32_connected = True
        print("ESP32: Connected on COM3 ✅")
    except serial.SerialException as e:
        print(f"ESP32: NOT CONNECTED (COM3 unavailable) ⚠️")
        print(f"  → Running in DEBUG mode without serial output\n")
    
    prev_flap_pos = 0
    last_move_time = 0
    COOLDOWN_TIME = 0.4
    
    # Configurable low fuel threshold in pounds
    LOW_FUEL_THRESHOLD_LBS = 500
    
    try:
        with FSUIPC() as f:
            print("FSUIPC: Connected ✅\n")
            
            while True:
                # Read all offsets
                raw = f.read([
                    (0x0570, 8), (0x02BC, 4), (0x02C8, 4), (0x0020, 4),  # [0-3] Alt, Spd, VS, GroundElev
                    (0x0578, 4), (0x057C, 4), (0x0580, 4),              # [4-6] Pitch, Roll, Hdg
                    (0x0BE4, 4),                                        # [7] Flaps
                    (0x0BEC, 4), (0x0BF4, 4), (0x0BF0, 4),              # [8-10] Gears (N, L, R)
                    (0x0BD4, 4), (0x0BC8, 2),                           # [11-12] Brakes
                    (0x036D, 1),                                        # [13] Overspeed
                    (0x036C, 1),                                        # [14] Stall warning
                    (0x3366, 1),                                        # [15] Engine fires
                    (0x3367, 1),                                        # [16] Doors open
                    (0x126C, 4),                                        # [17] Total fuel weight in pounds
                    (0x0366, 2),                                        # [18] On ground flag
                    (0x07BC, 4), (0x07CC, 2), (0x07D4, 4),              # [19-21] AP (Active, Hdg, Alt)
                    # Generator switches and active flags
                    (0x3B78, 4), (0x3B7C, 4),                           # [22-23] Engine 1 gen switch, gen active
                    (0x3AB8, 4), (0x3ABC, 4),                           # [24-25] Engine 2 gen switch, gen active
                    (0x39F8, 4), (0x39FC, 4),                           # [26-27] Engine 3 gen switch, gen active
                    (0x3938, 4), (0x393C, 4),                           # [28-29] Engine 4 gen switch, gen active
                    # Engine combustion flags
                    (0x3AE4, 4), (0x3A24, 4), (0x3964, 4), (0x38A4, 4), # [30-33] Eng 1-4 combustion
                ])
                
                now = time.time()
                DEG_CONV = 65536.0 * 65536.0
                
                # Flap movement logic
                current_flap_raw = int(struct.unpack('i', raw[7])[0])
                if current_flap_raw != prev_flap_pos:
                    last_move_time = now
                is_moving = (now - last_move_time) < COOLDOWN_TIME
                prev_flap_pos = current_flap_raw
                
                # Altitude & speed
                alt_ft = (struct.unpack('q', raw[0])[0] / DEG_CONV) * 3.28084
                ground_elev_ft = (struct.unpack('i', raw[3])[0] / 256.0) * 3.28084
                radio_alt = max(0.0, alt_ft - ground_elev_ft)
                speed_kts = struct.unpack('i', raw[1])[0] / 128.0
                
                # Gears
                gear_nose = struct.unpack('H', raw[8][:2])[0]
                gear_left = struct.unpack('H', raw[9][:2])[0]
                gear_right = struct.unpack('H', raw[10][:2])[0]
                
                # On ground flag
                on_ground = struct.unpack('H', raw[18])[0] != 0
                
                # ===================== CUSTOM WARNING/CAUTION LOGIC =====================
                master_warning = False
                master_caution = False
                warning_reasons = []
                caution_reasons = []
                
                # --- MASTER WARNING (Critical) ---
                # 1. Stall warning
                stall_warn = struct.unpack('B', raw[14])[0] != 0
                if stall_warn:
                    master_warning = True
                    warning_reasons.append("STALL")
                
                # 2. Engine fire
                engine_fires = struct.unpack('B', raw[15])[0]
                if engine_fires != 0:
                    master_warning = True
                    warning_reasons.append("FIRE")
                
                # 3. Gear unsafe near ground
                gear_fully_down = (gear_nose >= 16383 and gear_left >= 16383 and gear_right >= 16383)
                if not on_ground and radio_alt < 500 and speed_kts < 180 and not gear_fully_down:
                    master_warning = True
                    warning_reasons.append(f"GEAR@{radio_alt:.0f}ft")
                
                # 4. Flaps full but gear not down (landing configuration warning)
                flaps_full = (current_flap_raw >= 16383)  # Flaps at full extension
                if flaps_full and not gear_fully_down and not on_ground:
                    master_warning = True
                    warning_reasons.append("FLAPS-FULL/GEAR-UP")
                
                # --- MASTER CAUTION (Advisory) ---
                # 1. Low fuel (UNIVERSAL - works for all aircraft)
                fuel_total_lbs = struct.unpack('i', raw[17])[0]
                if fuel_total_lbs < LOW_FUEL_THRESHOLD_LBS:
                    master_caution = True
                    caution_reasons.append(f"FUEL:{fuel_total_lbs}lbs")
                
                # 2. Door open
                doors_open = struct.unpack('B', raw[16])[0]
                if doors_open != 0:
                    master_caution = True
                    caution_reasons.append(f"DOOR:0x{doors_open:02X}")
                
                # 3. Generator failures (engine running but generator off)
                eng1_combustion = struct.unpack('I', raw[30])[0] != 0
                eng2_combustion = struct.unpack('I', raw[31])[0] != 0
                eng3_combustion = struct.unpack('I', raw[32])[0] != 0
                eng4_combustion = struct.unpack('I', raw[33])[0] != 0
                
                gen1_active = struct.unpack('I', raw[23])[0] != 0
                gen2_active = struct.unpack('I', raw[25])[0] != 0
                gen3_active = struct.unpack('I', raw[27])[0] != 0
                gen4_active = struct.unpack('I', raw[29])[0] != 0
                
                gen_failures = []
                if eng1_combustion and not gen1_active:
                    gen_failures.append("GEN1")
                if eng2_combustion and not gen2_active:
                    gen_failures.append("GEN2")
                if eng3_combustion and not gen3_active:
                    gen_failures.append("GEN3")
                if eng4_combustion and not gen4_active:
                    gen_failures.append("GEN4")
                
                if gen_failures:
                    master_caution = True
                    caution_reasons.append(f"GEN-FAIL:{','.join(gen_failures)}")
                
                # ========================================================================
                
                # Build flight data
                fd = BridgeFlightData(
                    alt_ft = alt_ft,
                    speed = speed_kts,
                    vertical_speed = (struct.unpack('i', raw[2])[0] * 60 * 3.28084) / 256.0,
                    radio_alt = radio_alt,
                    pitch = (struct.unpack('i', raw[4])[0] * 360.0) / DEG_CONV,
                    roll = (struct.unpack('i', raw[5])[0] * 360.0) / DEG_CONV,
                    heading = (struct.unpack('i', raw[6])[0] * 360.0) / DEG_CONV,
                    
                    flap_actual = current_flap_raw,
                    
                    gear_nose = gear_nose,
                    gear_left = gear_left,
                    gear_right = gear_right,
                    
                    speedbrakes_ext = struct.unpack('i', raw[11])[0] > 0,
                    parking_brake = struct.unpack('H', raw[12])[0] > 0,
                    
                    masterWarning = master_warning,
                    masterCaution = master_caution,
                    overspeed = struct.unpack('b', raw[13])[0] != 0,
                    gpws = False,  # calc on esp
                    
                    ap_active = struct.unpack('i', raw[19])[0] == 1,
                    flaps_moving = is_moving,
                    
                    ap_heading = struct.unpack('H', raw[20])[0] * 360.0 / 65536.0,
                    ap_alt = (struct.unpack('i', raw[21])[0] / 65536.0) * 3.28084
                )
                
                # Send to ESP32 only if connected
                if esp32_connected and ser:
                    try:
                        ser.write(pack_flight_data(fd))
                    except serial.SerialException:
                        esp32_connected = False
                        print("\n⚠️ ESP32 disconnected during operation!")
                
                # Build alert string with reasons
                alert_str = ""
                if fd.masterWarning:
                    alert_str += f"[WARN:{','.join(warning_reasons)}] "
                if fd.masterCaution:
                    alert_str += f"[CAUT:{','.join(caution_reasons)}] "
                if fd.overspeed:
                    alert_str += "[OVRSPD] "
                if fd.gpws:
                    alert_str += "[GPWS] "
                
                # Debug dashboard
                print("\033[H\033[J", end="") 
                print(f"==================================================")
                print(f"        AEROLAP TELEMETRY DASHBOARD v2.3          ")
                if not esp32_connected:
                    print(f"       ⚠️  !! NOT CONNECTED TO ESP32 !!  ⚠️       ")
                print(f"==================================================")
                print(f" [MOVEMENT]  ALT: {fd.alt_ft:6.0f} | RALT: {fd.radio_alt:5.0f} | SPD: {fd.speed:6.1f} | VS: {fd.vertical_speed:5.0f}")
                print(f" [ATTITUDE]  HDG: {fd.heading:6.1f}° | PITCH: {fd.pitch:5.1f}° | ROLL: {fd.roll:5.1f}°")
                print(f"--------------------------------------------------")
                print(f" [SURFACES]  FLAPS: {fd.flap_actual:5} / 16383 | {'[MOVING]' if fd.flaps_moving else '[STATIC]'}")
                print(f"             GEARS: L:{fd.gear_left:5}  N:{fd.gear_nose:5}  R:{fd.gear_right:5}")
                print(f"--------------------------------------------------")
                print(f" [SYSTEMS]   BRAKE: {'[PARKED]' if fd.parking_brake else '[OFF]   '} | SPEEDBRAKE: {'[EXT]' if fd.speedbrakes_ext else '[RET]'}")
                print(f"             ALERTS: {alert_str if alert_str else 'None'}")
                print(f"--------------------------------------------------")
                print(f" [AUTOPILOT] STATUS: {'[ACTIVE]' if fd.ap_active else '[OFF]   '}")
                print(f"             TARGET: HDG {fd.ap_heading:3.0f}° | ALT {fd.ap_alt:5.0f} ft")
                print(f"==================================================")
                
                time.sleep(0.05) 
                
    except Exception as e:
        print(f"\nCRITICAL ERROR: {e}")
    finally:
        if ser and esp32_connected:
            ser.close()
            print("\nESP32 connection closed.")

if __name__ == "__main__":
    run_bridge()