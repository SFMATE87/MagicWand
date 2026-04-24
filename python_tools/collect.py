import serial
import os
import time

# --- BEÁLLÍTÁSOK ---
COM_PORT = 'COM5'
BAUD_RATE = 115200
TARGET_GESTURE = 'Idle' # Ezt kell átírni a felvétel előtt (I, Z, O, Idle)
SAMPLES_PER_WINDOW = 100 # 2 másodpercnyi adat 50 Hz-en
# -------------------

def main():
    save_dir = os.path.join('dataset', TARGET_GESTURE)
    
    # Ellenőrzés: létezik-e a mappa?
    if not os.path.exists(save_dir):
        print(f"HIBA: A '{save_dir}' mappa nem létezik! Kérlek, hozd létre.")
        return

    # Meglévő fájlok számolása az automatikus sorszámozáshoz
    existing_files = [f for f in os.listdir(save_dir) if f.endswith('.csv')]
    file_number = len(existing_files) + 1
    filename = f"{TARGET_GESTURE}_{file_number:03d}.csv"
    filepath = os.path.join(save_dir, filename)

    print(f"Cél: {TARGET_GESTURE} gesztus rögzítése.")
    print(f"Mentés ide: {filepath}")
    print(f"Csatlakozás a {COM_PORT} porthoz...")

    try:
        # Csatlakozás a soros porthoz
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=2)
        print("\nSikeres csatlakozás!")
        print(">>> Nyomd meg a BOOT gombot az ESP32-n, és végezd el a mozdulatot! <<<")
        
        data_window = []
        
        # 1. fázis: Várakozás az első érvényes adatsorra
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                values = line.split(',')
                if len(values) == 6:
                    data_window.append(line)
                    print(f"Adatfolyam elindult. Rögzítés: 1/{SAMPLES_PER_WINDOW}")
                    break

        # 2. fázis: A maradék 99 sor rögzítése
        while len(data_window) < SAMPLES_PER_WINDOW:
            line = ser.readline().decode('utf-8').strip()
            if line:
                values = line.split(',')
                if len(values) == 6:
                    data_window.append(line)
                    if len(data_window) % 10 == 0:
                        print(f"Rögzítve: {len(data_window)}/{SAMPLES_PER_WINDOW}")

        # 3. fázis: Mentés CSV fájlba
        with open(filepath, 'w') as f:
            f.write("ax,ay,az,gx,gy,gz\n") # Fejléc
            for row in data_window:
                f.write(f"{row}\n")
                
        print(f"\nSikeresen elmentve: {filepath}")

    except serial.SerialException as e:
        print(f"\nHIBA: Nem tudok csatlakozni a portra. ({e})")
        print("Megoldás: Zárd be a Soros Monitort az Arduino IDE-ben, mert foglalja a portot!")
    except Exception as e:
        print(f"\nVáratlan hiba történt: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == '__main__':
    main()