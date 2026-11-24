"""Monitor serial sederhana (pengganti `pio device monitor`).

Pakai di terminal VS Code:   python monitor.py          (otomatis cari STM32 VID 0483:5740)
                              python monitor.py COM12
Ketik perintah lalu Enter, seperti Serial Monitor Arduino IDE. Ctrl+C untuk keluar.
Otomatis menyambung ulang setelah board di-upload / dicolok ulang.
"""
import sys
import threading
import time

import serial
import serial.tools.list_ports


def cari_port():
    for p in serial.tools.list_ports.comports():
        if p.vid == 0x0483 and p.pid == 0x5740:
            return p.device
    return None


def main():
    port_arg = sys.argv[1] if len(sys.argv) > 1 else None
    ser = None
    lock = threading.Lock()
    berhenti = False

    def pembaca():
        nonlocal ser
        while not berhenti:
            with lock:
                s = ser
            if s is None:
                time.sleep(0.2)
                continue
            try:
                data = s.read(256)
                if data:
                    sys.stdout.write(data.decode("utf-8", "replace"))
                    sys.stdout.flush()
            except (serial.SerialException, OSError):
                with lock:
                    try:
                        s.close()
                    except Exception:
                        pass
                    ser = None
                print("\n[terputus - menunggu board tersambung lagi...]")

    def penyambung():
        nonlocal ser
        while not berhenti:
            with lock:
                ada = ser is not None
            if not ada:
                port = port_arg or cari_port()
                if port:
                    try:
                        s = serial.Serial(port, 115200, timeout=0.2)
                        s.dtr = True          # firmware mencetak banner saat DTR naik
                        with lock:
                            ser = s
                        print(f"[tersambung ke {port}]")
                    except (serial.SerialException, OSError) as e:
                        print(f"[gagal membuka {port}: {e}] - tutup program lain yang memakai port ini")
                        time.sleep(2)
            time.sleep(0.5)

    threading.Thread(target=pembaca, daemon=True).start()
    threading.Thread(target=penyambung, daemon=True).start()
    print("Monitor serial. Ketik perintah + Enter. Ctrl+C untuk keluar.")
    try:
        while True:
            baris = input()
            with lock:
                s = ser
            if s is None:
                print("[belum tersambung]")
                continue
            try:
                s.write((baris + "\n").encode())
            except (serial.SerialException, OSError):
                print("[gagal kirim]")
    except (KeyboardInterrupt, EOFError):
        pass
    berhenti = True
    with lock:
        if ser:
            ser.close()


if __name__ == "__main__":
    main()
