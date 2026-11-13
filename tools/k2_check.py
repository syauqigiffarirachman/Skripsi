"""Uji PC untuk K2 hardware (DS28E38), TAHAP 1 (base: sign + latensi). Kriteria di logbook.

Pakai:  python tools/k2_check.py COM12 [n]      (n default 5; n=30 untuk uji penuh)

Yang diperiksa (semuanya harus lulus):
  1. firmware melaporkan clock 48 MHz dan n/n tanda tangan sukses (tanpa ERR);
  2. kunci publik (halaman 4,5 chip) berada di kurva P-256;
  3. SETIAP r||s lolos verifikasi ECDSA P-256 di PC terhadap SHA-256(ROM||page0||challenge||
     page#(0)||MANID) (pesan yang di-hash dan ditandatangani chip), DAN tanda tangan yang
     dirusak (s xor 1) DITOLAK (bukti pemeriksa tidak asal menerima);
  4. challenge yang dicetak firmware == digest konstan K1 (sama dengan k1_check.py);
  5. ms_sign konsisten dengan cyc_sign / 48 MHz;
  6. sebaran latensi kecil (max-min terhadap rata-rata <= 1%).
Terakhir dicetak perbandingan dengan data K2 lama (firmware monolit, termasuk ROM search per
sign) dan dengan K1 software terukur terakhir.
"""
import hashlib
import os
import sys
import time

import serial

sys.path.insert(0, os.path.dirname(__file__))
from p256 import on_curve, verify  # noqa: E402

DIGEST = bytes.fromhex("D72C103978443C67EAD7542910F0EBF1C8BAF63182C07E062BCC6FC2E9A8A50E")
MAX_SPREAD = 0.01
K2_LAMA_MS = 224.1          # data/k2_hw_msg_ayah_48mhz.csv (sign + ROM search + hash), n=30
K1_1C_MS = 644.1            # K1 software tahap 2 (konfigurasi 1c), logbook 13 Nov


def baca_sampai(ser, penanda, batas_s):
    baris, t_akhir = [], time.time() + batas_s
    while time.time() < t_akhir:
        raw = ser.readline()
        if not raw:
            continue
        t = raw.decode("ascii", "replace").strip()
        if t:
            baris.append(t)
        if penanda in t:
            return baris
    raise TimeoutError(f"tidak ada '{penanda}' dalam {batas_s:.0f} s; baris terakhir: {baris[-3:]}")


def nilai(info, kunci):
    return next((b.split(":", 1)[1].strip() for b in info if b.startswith(kunci)), "")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    port, n = sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 5
    gagal = []

    def cek(ok, pesan):
        print(("  [OK]    " if ok else "  [GAGAL] ") + pesan)
        if not ok:
            gagal.append(pesan)

    with serial.Serial(port, 115200, timeout=1) as ser:
        ser.reset_input_buffer()
        ser.write(b"I\n")
        info = baca_sampai(ser, "PubKey XY", 15)
        ser.write(f"R{n}\n".encode())
        data = baca_sampai(ser, "SELESAI", n * 1 + 40)

    print("\n".join(info[-9:]))
    clock = next((int(b.split(":")[1].split()[0]) for b in info if b.startswith("# MCU clock")), 0)
    rom = bytes.fromhex(nilai(info, "# ROM ID"))
    manid = bytes.fromhex(nilai(info, "# MANID"))
    page0 = bytes.fromhex(nilai(info, "# Page0"))
    digest_fw = bytes.fromhex(nilai(info, "# Digest"))
    pub_hex = nilai(info, "# PubKey")
    pub = (int(pub_hex[:64], 16), int(pub_hex[64:], 16)) if len(pub_hex) == 128 else (0, 0)

    print("\nTahap 1 K2 - pemeriksaan:")
    cek(clock == 48_000_000, f"clock = {clock} Hz (harus 48000000)")
    cek(len(rom) == 8 and len(manid) == 2 and len(page0) == 32, "ROM ID (8 B), MANID (2 B), page0 (32 B) terbaca")
    cek(on_curve(pub), "kunci publik chip (halaman 4,5) berada di kurva P-256")
    cek(digest_fw == DIGEST, "challenge firmware == digest konstan K1")

    sampel, galat = [], 0
    for b in data:
        f = b.split(",")
        if len(f) == 4 and f[0].isdigit():
            sampel.append((int(f[0]), int(f[1]), f[2], f[3]))
        elif len(f) == 3 and f[1] == "ERR":
            galat += 1
    cek(len(sampel) == n and galat == 0, f"sampel diterima {len(sampel)}/{n} (ERR: {galat})")

    pesan = hashlib.sha256(rom + page0 + DIGEST + b"\x00" + manid).digest()
    n_valid = n_tolak = 0
    for _, _, _, rs in sampel:
        r, s = int(rs[:64], 16), int(rs[64:], 16)
        n_valid += verify(pub, pesan, r, s)
        n_tolak += not verify(pub, pesan, r, s ^ 1)
    cek(n_valid == len(sampel) and sampel, f"verifikasi ECDSA di PC {n_valid}/{len(sampel)} valid")
    cek(n_tolak == len(sampel) and sampel, f"tanda tangan dirusak ditolak {n_tolak}/{len(sampel)}")

    salah_ms = sum(abs(float(ms) - cyc / 48_000) > 0.002 for _, cyc, ms, _ in sampel)
    cek(salah_ms == 0, f"ms_sign konsisten dengan cyc_sign/48MHz ({salah_ms} menyimpang)")

    if sampel:
        c = [s[1] for s in sampel]
        rata = sum(c) / len(c)
        sebar = (max(c) - min(c)) / rata
        print(f"\n  latensi: mean={rata / 48_000:.3f} ms  min={min(c) / 48_000:.3f}  "
              f"max={max(c) / 48_000:.3f}  sebaran={sebar * 100:.3f}%")
        cek(sebar <= MAX_SPREAD, f"sebaran {sebar * 100:.3f}% <= {MAX_SPREAD * 100:.0f}%")
        hw = rata / 48_000
        print(f"\n  Pembanding: K2 lama = {K2_LAMA_MS} ms (termasuk ROM search per sign; selisih "
              f"{K2_LAMA_MS - hw:+.1f} ms)")
        print(f"  K1 software (1c) = {K1_1C_MS} ms -> rasio software/DS28E38 = {K1_1C_MS / hw:.2f}x")

    print("\nHASIL TAHAP 1 K2:", "LULUS" if not gagal else f"GAGAL ({len(gagal)} butir)")
    sys.exit(1 if gagal else 0)


if __name__ == "__main__":
    main()
