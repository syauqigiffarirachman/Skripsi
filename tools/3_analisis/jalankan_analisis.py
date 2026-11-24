"""Alur analisis FINAL dalam satu perintah (24 Nov 2026): data mentah -> tabel dan gambar naskah.

Antarmuka: satu perintah. Urutan langkah (dan fakta bahwa analisis_d harus lebih dulu, karena langkah lain membaca
runs.csv-nya) hanya ditulis di SINI, bukan di README, docs, dan kepala tiap skrip.

  python tools/3_analisis/jalankan_analisis.py            jalankan semua langkah, berhenti di kegagalan pertama
  python tools/3_analisis/jalankan_analisis.py --kering   cetak urutan saja

Kalibrasi memakai bawaan tools/2_kalibrasi/kalibrasi_param.py (sama di semua langkah). Data masukan: data/tahap_stop.txt.
Hasil ditulis di data/analisis/stop_final/. Untuk MEMBUKTIKAN hasil dapat diulang tanpa menimpa: tools/4_verifikasi/cek_reproduksi_analisis.py.
"""
import argparse
import subprocess
import time
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)

MANIFEST = "data/tahap_stop.txt"
KELUAR = "data/analisis/stop_final"

# (apa yang dibuat, skrip relatif terhadap tools/3_analisis, argumen). URUTAN PENTING.
LANGKAH = [
    ("statistik utama, runs.csv, gambar dasar", "analisis_d.py", ["--manifest", MANIFEST, "--stop", "--keluar", KELUAR]),
    ("dekomposisi energi (Gambar 4.12)", "dekomposisi_energi.py", []),
    ("sensitivitas frame hilang K4 dan nilai p (B3, B5)", "analisis_sensitivitas.py", []),
    ("proyeksi baterai (C7)", "proyeksi_baterai.py", []),
    ("gambar Bab 4", "gambar_bab4_stop.py", []),
    ("salin gambar dengan nomor naskah", "ekspor_gambar_naskah.py", []),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--kering", action="store_true", help="cetak urutan langkah saja")
    args = ap.parse_args()
    for i, (nama, skrip, arg) in enumerate(LANGKAH, 1):
        perintah = [_sys.executable, str(ROOT / "tools" / "3_analisis" / skrip), *arg]
        print(f"[{i}/{len(LANGKAH)}] {nama}: python tools/3_analisis/{skrip} {' '.join(arg)}".rstrip())
        if args.kering:
            continue
        t0 = time.time()
        if subprocess.run(perintah, cwd=ROOT).returncode != 0:
            raise SystemExit(f"langkah {i} gagal: {skrip}")
        print(f"      selesai ({time.time() - t0:.0f} s)")


if __name__ == "__main__":
    main()
