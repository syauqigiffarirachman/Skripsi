"""Kumpulkan gambar Bab 5 skripsi dengan NOMOR dan NAMA sesuai naskah (21 Nov 2026).

Gambar dibuat oleh tools/3_analisis/gambar_bab4_stop.py (gambar_bab4/gambar_4_*.png, penomoran lama "4.x") dan
tools/3_analisis/dekomposisi_energi.py. Skrip ini hanya MENYALIN ke data/analisis/stop_final/gambar_naskah/ dengan nama
`Gambar_5.<n>_<isi>.png` (pemetaan dari v4: SKRIPSI_FULL_Syauqi_v4_2026-11-20.pdf) dan menulis README.md.

Pemeriksaan sebelum menyalin (berhenti bila gagal, kecuali --paksa):
  1. ringkasan.md harus memuat kalibrasi yang sama dengan tools/2_kalibrasi/kalibrasi_param.py (rshunt, offset, u_rel);
  2. setiap gambar tidak boleh lebih tua daripada runs.csv (gambar basi = dibuat sebelum analisis diulang).

Pakai:  python tools/3_analisis/ekspor_gambar_naskah.py [--paksa]
Urutan lengkap bila kalibrasi berubah: analisis_d.py --stop ..., dekomposisi_energi.py, gambar_bab4_stop.py, lalu skrip ini.
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import re
import shutil
import sys
from datetime import datetime
from pathlib import Path

import kalibrasi_param as kp  # noqa: E402

ANA = ROOT / "data" / "analisis" / "stop_final"
SRC = ANA / "gambar_bab4"
OUT = ANA / "gambar_naskah"

# (nomor naskah, berkas sumber, nama keluaran, status terhadap v4, keterangan)
PETA = [
    ("5.1", "gambar_4_10_frame_hilang_K4.png", "frame_hilang_K4", "tetap", "jumlah frame; tidak bergantung kalibrasi"),
    ("5.2", "gambar_4_11_bland_altman_scpu.png", "bland_altman_durasi_tidur", "tetap", "poin persentase; tidak bergantung kalibrasi"),
    ("5.3", "gambar_4_9_arus_diam_antar_blok.png", "arus_diam_antar_blok", "BERUBAH", "arus diam 1,99 -> ~2,45 mA; sumbu mengikuti data"),
    ("5.4", "gambar_4_1_jejak_arus.png", "jejak_arus_3_detik", "BERUBAH", "sumbu arus 0-26 mA; arus diam 2,45 mA; puncak K2/K4 23,8 mA utuh"),
    ("5.5", "gambar_4_1b_jejak_arus_rinci.png", "jejak_arus_rinci", "BERUBAH", "sumbu arus 0-26 mA"),
    ("5.6", "gambar_4_2_latensi.png", "latensi_tanda_tangan", "tetap", "ms; tidak bergantung kalibrasi"),
    ("5.7", "gambar_4_3_durasi_tidur_cpu.png", "durasi_tidur_cpu", "tetap", "% waktu; tidak bergantung kalibrasi"),
    ("5.8", "gambar_4_4_daya_dinamis.png", "daya_dinamis", "BERUBAH", "mW x1,218; batang galat Ugab 1,5 %; teks +-1,5 %"),
    ("5.9", "gambar_4_5_energi_per_permintaan.png", "energi_per_permintaan", "BERUBAH", "mJ x1,218; batang galat Ugab 1,5 %; teks +-1,5 %"),
    ("5.10", "gambar_4_6_interaksi.png", "interaksi_arsitektur_skenario", "BERUBAH", "panel (b) daya dinamis mW; panel (a) durasi tidur sama"),
    ("5.11", "gambar_4_7_tingkat_layanan_vs_model.png", "tingkat_layanan_vs_model", "tetap", "% permintaan; tidak bergantung kalibrasi"),
    ("5.12", "gambar_4_8_throughput_vs_batas.png", "throughput_vs_batas", "tetap", "sign/s; tidak bergantung kalibrasi"),
    ("baru", "gambar_4_12_dekomposisi_energi.png", "dekomposisi_energi_opsional", "BARU (opsional)", "memperkuat Tabel 5.10; v4 hanya memuat tabel"),
]


def cek_kalibrasi():
    teks = (ANA / "ringkasan.md").read_text(encoding="utf-8")
    m = re.search(r"rshunt = ([\d.]+) Ohm, offset = (-?[\d.]+) LSB, u_rel kalibrasi = ([\d.]+)%", teks)
    if not m:
        return False, "baris kalibrasi tidak ditemukan di ringkasan.md"
    r, o, u = float(m.group(1)), float(m.group(2)), float(m.group(3)) / 100
    ok = abs(r - kp.RSHUNT_EFEKTIF) < 1e-9 and abs(o - kp.OFFSET_LSB) < 1e-9 and abs(u - kp.U_REL_KAL) < 1e-9
    return ok, f"ringkasan.md: rshunt={r}, offset={o}, u_rel={u}; kalibrasi_param: {kp.RSHUNT_EFEKTIF}, {kp.OFFSET_LSB}, {kp.U_REL_KAL}"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--paksa", action="store_true", help="salin walau pemeriksaan gagal")
    a = ap.parse_args()

    masalah = []
    ok, info = cek_kalibrasi()
    print(info)
    if not ok:
        masalah.append("kalibrasi analisis tidak sama dengan kalibrasi_param")
    t_data = (ANA / "runs.csv").stat().st_mtime
    for _, f, *_ in PETA:
        p = SRC / f
        if not p.exists():
            masalah.append(f"tidak ada: {f}")
        elif p.stat().st_mtime < t_data:
            masalah.append(f"basi (lebih tua dari runs.csv): {f}")
    if masalah and not a.paksa:
        print("BERHENTI:\n  " + "\n  ".join(masalah))
        raise SystemExit(1)
    for m_ in masalah:
        print("PERINGATAN:", m_)

    OUT.mkdir(parents=True, exist_ok=True)
    for f in OUT.glob("Gambar_*.png"):
        f.unlink()
    baris = []
    for no, f, nama, status, ket in PETA:
        tujuan = f"Gambar_{no}_{nama}.png"
        shutil.copy2(SRC / f, OUT / tujuan)
        baris.append(f"| {no} | `{tujuan}` | `gambar_bab4/{f}` | {status} | {ket} |")
    (OUT / "README.md").write_text(
        "# Gambar Bab 5 untuk naskah (kalibrasi 21 Nov 2026)\n\n"
        f"Dibuat {datetime.now():%Y-%m-%d %H:%M} oleh `tools/3_analisis/ekspor_gambar_naskah.py`. {info}.\n\n"
        "Penomoran = naskah v4 (Gambar 5.x); sumber memakai penomoran lama 4.x. Ganti gambar di Word dengan berkas pada kolom 2.\n"
        "Status: BERUBAH = nilai atau sumbu berbeda dari v4 (wajib diganti); tetap = isi sama (boleh dibiarkan, berkas disertakan agar seragam).\n\n"
        "| Gambar | Berkas | Sumber | Status | Perubahan |\n|---|---|---|---|---|\n" + "\n".join(baris) + "\n\n"
        "Diagram Bab 2-3 (Gambar 2.1, 3.1-3.5) digambar terpisah dan TIDAK dibuat skrip ini; lihat laporan/laporan_perubahan_skripsi_v4_kalibrasi_2026-11-21.md bagian H.\n",
        encoding="utf-8")
    print(f"{len(PETA)} gambar disalin ke {OUT}")


if __name__ == "__main__":
    main()
