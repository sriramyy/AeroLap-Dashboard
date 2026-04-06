import time
import struct
from fsuipc import FSUIPC

def run_bridge():
    print("\n" + "="*40)
    print("   AEROLAP TELEMETRY BRIDGE (RAW BYTES)   ")
    print("="*40)
    
    try:
        with FSUIPC() as f:
            print("SUCCESS: Linked to FSX Shared Memory.")
            print("Status: Reading Raw Telemetry...\n")

            while True:
                # REQUEST RAW BYTES:
                # Instead of a letter like "q", we give it the exact BYTE COUNT.
                # 0x0570 needs 8 bytes | 0x02BC needs 4 bytes
                data = f.read([
                    (0x0570, 8), 
                    (0x02BC, 4)
                ])

                # data[0] is now a raw byte buffer (type: bytes)
                # data[1] is also a raw byte buffer
                
                # --- MANUAL BIT UNPACKING ---
                # 'q' = 8-byte signed integer (long long)
                # 'i' = 4-byte signed integer (int)
                # This bypasses the library's internal issues.
                alt_raw = struct.unpack('q', data[0])[0]
                spd_raw = struct.unpack('i', data[1])[0]

                # --- CONVERSION LOGIC ---
                # Altitude: Value / 2^32 = Meters -> Feet
                alt_feet = (alt_raw / (65536.0 * 65536.0)) * 3.28084
                
                # Airspeed: Knots * 128
                spd_knots = spd_raw / 128.0

                # Output to terminal
                print(f"ALTITUDE: {int(alt_feet):5} ft  |  AIRSPEED: {int(spd_knots):3} kts", end='\r')

                time.sleep(0.1)

    except Exception as e:
        print(f"\nCRITICAL ERROR: {e}")

if __name__ == "__main__":
    run_bridge()