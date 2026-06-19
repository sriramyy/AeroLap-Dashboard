import time
import struct
import serial
import math
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

def print_dashboard(scenario_name, fd, alert_reasons=""):
    """Print a clean dashboard display"""
    print("\033[H\033[J", end="")  # Clear screen
    print("="*60)
    print("          🛩️  DEMO AEROLAP DASHBOARD v2.3  🛩️")
    print("="*60)
    print(f"📍 SCENARIO: {scenario_name}")
    print("-"*60)
    print(f" [MOVEMENT]  ALT: {fd.alt_ft:6.0f} ft | RALT: {fd.radio_alt:5.0f} ft")
    print(f"             SPD: {fd.speed:6.1f} kts | VS: {fd.vertical_speed:5.0f} fpm")
    print(f" [ATTITUDE]  HDG: {fd.heading:6.1f}° | PITCH: {fd.pitch:5.1f}° | ROLL: {fd.roll:5.1f}°")
    print("-"*60)
    print(f" [SURFACES]  FLAPS: {fd.flap_actual:5} / 16383 | {'[MOVING]' if fd.flaps_moving else '[STATIC]'}")
    print(f"             GEARS: L:{fd.gear_left:5}  N:{fd.gear_nose:5}  R:{fd.gear_right:5}")
    print("-"*60)
    print(f" [SYSTEMS]   BRAKE: {'[PARKED]' if fd.parking_brake else '[OFF]   '} | SPEEDBRAKE: {'[EXT]' if fd.speedbrakes_ext else '[RET]'}")
    
    alert_str = ""
    if fd.masterWarning:
        alert_str += "[⚠️  WARN] "
    if fd.masterCaution:
        alert_str += "[⚠️  CAUT] "
    if fd.overspeed:
        alert_str += "[⚡ OVRSPD] "
    if fd.gpws:
        alert_str += "[🔊 GPWS] "
    
    print(f"             ALERTS: {alert_str if alert_str else '✅ None'}")
    if alert_reasons:
        print(f"             REASON: {alert_reasons}")
    print("-"*60)
    print(f" [AUTOPILOT] STATUS: {'[ACTIVE]' if fd.ap_active else '[OFF]   '}")
    print(f"             TARGET: HDG {fd.ap_heading:3.0f}° | ALT {fd.ap_alt:5.0f} ft")
    print("="*60)

def run_demo():
    # Try to connect to ESP32
    ser = None
    esp32_connected = False
    
    try:
        ser = serial.Serial('COM3', 115200, timeout=1)
        esp32_connected = True
        print("ESP32: Connected on COM3 ✅\n")
        time.sleep(1)
    except serial.SerialException:
        print("ESP32: NOT CONNECTED - Running in display-only mode ⚠️\n")
        time.sleep(1)
    
    scenarios = []
    
    # =================== SCENARIO 1: NORMAL CRUISE ===================
    scenarios.append({
        "name": "1️⃣  NORMAL CRUISE - All Systems Nominal",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=35000.0, speed=450.0, vertical_speed=0.0, radio_alt=35000.0,
            pitch=2.5, roll=0.0, heading=270.0,
            flap_actual=0,
            gear_nose=0, gear_left=0, gear_right=0,
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=True, flaps_moving=False,
            ap_heading=270.0, ap_alt=35000.0
        ),
        "reason": "✈️  Clean configuration, autopilot engaged"
    })
    
    # =================== SCENARIO 2: DESCENT ===================
    scenarios.append({
        "name": "2️⃣  DESCENT - Vertical Speed Test",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=15000.0, speed=280.0, vertical_speed=-1800.0, radio_alt=14500.0,
            pitch=-3.5, roll=0.0, heading=270.0,
            flap_actual=0,
            gear_nose=0, gear_left=0, gear_right=0,
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=True, flaps_moving=False,
            ap_heading=270.0, ap_alt=10000.0
        ),
        "reason": "📉 Descending at 1800 fpm"
    })
    
    # =================== SCENARIO 3: GEAR WARNING ===================
    scenarios.append({
        "name": "3️⃣  ⚠️  MASTER WARNING - Gear Not Down",
        "duration": 4,
        "data": BridgeFlightData(
            alt_ft=1500.0, speed=150.0, vertical_speed=-500.0, radio_alt=800.0,
            pitch=-2.0, roll=0.0, heading=90.0,
            flap_actual=16383,  # Full flaps
            gear_nose=0, gear_left=0, gear_right=0,  # Gear up!
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=True, masterCaution=False, overspeed=False, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=1000.0
        ),
        "reason": "🚨 FLAPS FULL / GEAR UP - Low altitude warning!"
    })
    
    # =================== SCENARIO 4: GEAR EXTENDING ===================
    scenarios.append({
        "name": "4️⃣  GEAR EXTENDING - Transit State",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=1500.0, speed=150.0, vertical_speed=-500.0, radio_alt=800.0,
            pitch=-2.0, roll=0.0, heading=90.0,
            flap_actual=16383,
            gear_nose=8192, gear_left=8192, gear_right=8192,  # Mid-transit
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=1000.0
        ),
        "reason": "⚙️  Gear in transit (50% extended)"
    })
    
    # =================== SCENARIO 5: LANDING CONFIG ===================
    scenarios.append({
        "name": "5️⃣  LANDING CONFIG - All Green",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=500.0, speed=140.0, vertical_speed=-700.0, radio_alt=200.0,
            pitch=-3.0, roll=0.0, heading=90.0,
            flap_actual=16383,  # Full flaps
            gear_nose=16383, gear_left=16383, gear_right=16383,  # Gear down
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=0.0
        ),
        "reason": "🛬 Configured for landing"
    })
    
    # =================== SCENARIO 6: GPWS WARNING ===================
    scenarios.append({
        "name": "6️⃣  🔊 GPWS ALERT - Terrain Proximity",
        "duration": 4,
        "data": BridgeFlightData(
            alt_ft=350.0, speed=140.0, vertical_speed=-700.0, radio_alt=45.0,
            pitch=-3.0, roll=0.0, heading=90.0,
            flap_actual=16383,
            gear_nose=16383, gear_left=16383, gear_right=16383,
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=True,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=0.0
        ),
        "reason": "🚨 Radio altitude < 50 ft - PULL UP!"
    })
    
    # =================== SCENARIO 7: TOUCHDOWN ===================
    scenarios.append({
        "name": "7️⃣  TOUCHDOWN - Speedbrakes Deployed",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=305.0, speed=130.0, vertical_speed=-120.0, radio_alt=0.0,
            pitch=0.0, roll=0.0, heading=90.0,
            flap_actual=16383,
            gear_nose=16383, gear_left=16383, gear_right=16383,
            speedbrakes_ext=True, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=0.0
        ),
        "reason": "✅ Speedbrakes deployed on touchdown"
    })
    
    # =================== SCENARIO 8: ROLLOUT ===================
    scenarios.append({
        "name": "8️⃣  ROLLOUT - Decelerating",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=305.0, speed=60.0, vertical_speed=0.0, radio_alt=0.0,
            pitch=0.0, roll=0.0, heading=90.0,
            flap_actual=16383,
            gear_nose=16383, gear_left=16383, gear_right=16383,
            speedbrakes_ext=True, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=0.0
        ),
        "reason": "🛬 Slowing down on runway"
    })
    
    # =================== SCENARIO 9: PARKING BRAKE ===================
    scenarios.append({
        "name": "9️⃣  PARKED - Parking Brake Set",
        "duration": 3,
        "data": BridgeFlightData(
            alt_ft=305.0, speed=0.0, vertical_speed=0.0, radio_alt=0.0,
            pitch=0.0, roll=0.0, heading=90.0,
            flap_actual=0,  # Flaps retracted
            gear_nose=16383, gear_left=16383, gear_right=16383,
            speedbrakes_ext=False, parking_brake=True,
            masterWarning=False, masterCaution=False, overspeed=False, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=90.0, ap_alt=0.0
        ),
        "reason": "🅿️  Aircraft secured, parking brake set"
    })
    
    # =================== SCENARIO 10: OVERSPEED TEST ===================
    scenarios.append({
        "name": "🔟 ⚡ OVERSPEED WARNING - Excessive Airspeed",
        "duration": 4,
        "data": BridgeFlightData(
            alt_ft=15000.0, speed=420.0, vertical_speed=-3000.0, radio_alt=14500.0,
            pitch=-15.0, roll=0.0, heading=180.0,
            flap_actual=0,
            gear_nose=0, gear_left=0, gear_right=0,
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=False, overspeed=True, gpws=False,
            ap_active=False, flaps_moving=False,
            ap_heading=180.0, ap_alt=10000.0
        ),
        "reason": "⚡ OVERSPEED - Reduce airspeed immediately!"
    })
    
    # =================== SCENARIO 11: MASTER CAUTION ===================
    scenarios.append({
        "name": "1️⃣1️⃣  ⚠️  MASTER CAUTION - Low Fuel",
        "duration": 4,
        "data": BridgeFlightData(
            alt_ft=25000.0, speed=380.0, vertical_speed=0.0, radio_alt=24500.0,
            pitch=2.0, roll=0.0, heading=180.0,
            flap_actual=0,
            gear_nose=0, gear_left=0, gear_right=0,
            speedbrakes_ext=False, parking_brake=False,
            masterWarning=False, masterCaution=True, overspeed=False, gpws=False,
            ap_active=True, flaps_moving=False,
            ap_heading=180.0, ap_alt=25000.0
        ),
        "reason": "⚠️  Low fuel quantity detected"
    })

    print("🚀 Starting Aerolap Demo Sequence...")
    time.sleep(1)

    for scene in scenarios:
        # 1. Print to your console dashboard
        print_dashboard(scene["name"], scene["data"], scene["reason"])
        
        # 2. Send the packed binary data to the ESP32 (if connected)
        if esp32_connected:
            try:
                packet = pack_flight_data(scene["data"])
                ser.write(packet)
            except Exception as e:
                print(f"Serial Error: {e}")
        
        # 3. Wait for the duration of the scenario
        time.sleep(scene["duration"])

    print("\n✅ Demo Sequence Complete.")
    if ser:
        ser.close()

# This is the "Key" that starts the program
if __name__ == "__main__":
    run_demo()