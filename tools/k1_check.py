"""Uji PC untuk K1 software, TAHAP 1 (base standar: ECDSA P-256 + nonce RFC 6979). Kriteria di logbook.

Pakai:  python tools/k1_check.py COM12 [n]      (n default 5; n=30 untuk uji penuh)

Yang diperiksa (semuanya harus lulus):
  1. firmware melaporkan clock 48 MHz dan n/n tanda tangan sukses;
  2. kunci publik firmware == d x G yang dihitung ulang di PC (bukan dipercaya begitu saja);
  3. SETIAP r||s lolos verifikasi ECDSA P-256 di PC terhadap digest konstan;
  4. SETIAP r||s sama persis dengan referensi RFC 6979 independen (tools/rfc6979_ref.py, yang
     sendiri lolos vektor resmi RFC 6979 A.2.5) -> bukti nonce deterministik sesuai standar;
  5. ms_sign konsisten dengan cyc_sign / 48 MHz;
  6. sebaran latensi kecil (max-min terhadap rata-rata <= 1%).
Terakhir dicetak perbandingan latensi dengan DS28E38 (data/k2_hw_msg_ayah_48mhz.csv).
"""
import csv
import os
import sys
import time

import serial

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ds9481_comm"))
from ds9481.ecdsa_verify import G, _mul, verify  # noqa: E402

sys.path.insert(0, os.path.dirname(__file__))
import rfc6979_ref  # noqa: E402

D = bytes.fromhex("643EC908DC6AF993A3287913" "13241E693FAAD7A2334208F12C99021AD3E0223D")
DIGEST = bytes.fromhex("D72C103978443C67EAD7542910F0EBF1C8BAF63182C07E062BCC6FC2E9A8A50E")
MAX_SPREAD = 0.01
DS28E38_CSV = os.path.join(os.path.dirname(__file__), "..", "data", "k2_hw_msg_ayah_48mhz.csv")


def latensi_ds28e38():
    """-> (rata-rata ms sign saja = total-hash, rata-rata ms total, n) dari data terukur K2, atau None."""
    try:
        with open(DS28E38_CSV, encoding="utf-8") as f:
            baris = list(csv.DictReader(f))
        sign = [(int(b["total_us"]) - int(b["hash_us"])) / 1000 for b in baris]
        total = [int(b["total_us"]) / 1000 for b in baris]
        return sum(sign) / len(sign), sum(total) / len(total), len(baris)
    except (OSError, KeyError, ValueError, ZeroDivisionError):
        return None


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
        info = baca_sampai(ser, "PubKey XY", 10)
        ser.write(f"R{n}\n".encode())
        data = baca_sampai(ser, "SELESAI", n * 8 + 30)

    print("\n".join(info[-8:]))
    clock = next((int(b.split(":")[1].split()[0]) for b in info if b.startswith("# MCU clock")), 0)
    pub_fw = next((b.split(":")[1].strip() for b in info if b.startswith("# PubKey")), "")

    print("\nTahap 1 - pemeriksaan:")
    cek(clock == 48_000_000, f"clock = {clock} Hz (harus 48000000)")

    px, py = _mul(int.from_bytes(D, "big"), G)
    pub_pc = f"{px:064X}{py:064X}"
    cek(pub_fw.upper() == pub_pc, "kunci publik firmware == d x G hitung ulang di PC")

    sampel = []
    for b in data:
        f = b.split(",")
        if len(f) == 4 and f[0].isdigit():
            sampel.append((int(f[0]), int(f[1]), f[2], f[3]))
    cek(len(sampel) == n, f"sampel diterima {len(sampel)}/{n}")

    n_valid = 0
    for i, cyc, ms, rs in sampel:
        r, s = int(rs[:64], 16), int(rs[64:], 16)
        n_valid += verify(px, py, DIGEST, r, s)
    cek(n_valid == len(sampel) and sampel, f"verifikasi ECDSA di PC {n_valid}/{len(sampel)} valid")

    cek(rfc6979_ref.selftest(), "referensi PC lolos vektor resmi RFC 6979 A.2.5")
    r_ref, s_ref = rfc6979_ref.sign(int.from_bytes(D, "big"), DIGEST)
    rs_ref = f"{r_ref:064X}{s_ref:064X}"
    n_sama = sum(rs.upper() == rs_ref for _, _, _, rs in sampel)
    cek(n_sama == len(sampel) and sampel, f"r||s == referensi RFC 6979: {n_sama}/{len(sampel)} identik")

    salah_ms = sum(abs(float(ms) - cyc / 48_000) > 0.002 for _, cyc, ms, _ in sampel)
    cek(salah_ms == 0, f"ms_sign konsisten dengan cyc_sign/48MHz ({salah_ms} menyimpang)")

    if sampel:
        c = [s[1] for s in sampel]
        rata = sum(c) / len(c)
        sebar = (max(c) - min(c)) / rata
        print(f"\n  latensi: mean={rata / 48_000:.3f} ms  min={min(c) / 48_000:.3f}  "
              f"max={max(c) / 48_000:.3f}  sebaran={sebar * 100:.3f}%")
        cek(sebar <= MAX_SPREAD, f"sebaran {sebar * 100:.3f}% <= {MAX_SPREAD * 100:.0f}%")

    if sampel:
        ds = latensi_ds28e38()
        if ds:
            sw = rata / 48_000
            print(f"\n  Perbandingan (sign, ms): software K1 = {sw:.1f}  |  DS28E38 = {ds[0]:.1f} "
                  f"(total termasuk hash {ds[1]:.1f}, n={ds[2]})")
            print(f"  rasio software/DS28E38 = {sw / ds[0]:.2f}x  ->  "
                  + ("DS28E38 lebih cepat" if sw > ds[0] else "software lebih cepat"))

    print("\nHASIL TAHAP 1:", "LULUS" if not gagal else f"GAGAL ({len(gagal)} butir)")
    sys.exit(1 if gagal else 0)


if __name__ == "__main__":
    main()
