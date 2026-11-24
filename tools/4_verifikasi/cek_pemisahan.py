"""Bukti bahwa kode FINAL tidak bergantung pada kode PENGEMBANGAN (24 Nov 2026).

Dua pemeriksaan statis, tanpa perangkat keras dan tanpa membangun:
  1. FIRMWARE: tiap `#include "..."` di src/host/final, src/host/bersama, src/logger, src/attacker diselesaikan
     (relatif terhadap berkas pemintanya, lalu include/) dan TIDAK boleh jatuh ke src/host/dev, include/dev,
     atau tools/. Penyertaan transitif ikut diikuti.
  2. SKRIP PC: modul yang diimpor skrip di tools/1_akuisisi, 2_kalibrasi, 3_analisis, 4_verifikasi
     tidak boleh berasal dari tools/dev atau tools/demo.
  (tests/ sengaja tidak diperiksa: ia menguji juga alat dev dan demo.)

Pakai:  python tools/4_verifikasi/cek_pemisahan.py        (kode keluar 1 bila ada pelanggaran)
"""
import re
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT, skrip_tools  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)

FIRMWARE_FINAL = ["src/host/final", "src/host/bersama", "src/logger", "src/attacker"]
DILARANG_FW = ["src/host/dev", "include/dev"]
PC_FINAL = ["tools/1_akuisisi", "tools/2_kalibrasi", "tools/3_analisis", "tools/4_verifikasi"]
DILARANG_PC = ["tools/dev", "tools/demo"]

INC = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
IMP = re.compile(r"^\s*(?:import|from)\s+([A-Za-z_][\w]*)", re.M)


def _di_dalam(p, daftar):
    p = p.resolve()
    return any(p.is_relative_to((ROOT / d).resolve()) for d in daftar)


def cek_firmware():
    masalah, dilihat = [], 0
    antre = [f for d in FIRMWARE_FINAL for f in (ROOT / d).rglob("*") if f.suffix in (".cpp", ".h", ".c")]
    sudah = set()
    while antre:
        f = antre.pop()
        if f in sudah:
            continue
        sudah.add(f)
        dilihat += 1
        for m in INC.finditer(f.read_text(encoding="utf-8", errors="replace")):
            nama = m.group(1)
            for dasar in (f.parent, ROOT / "include"):
                t = (dasar / nama).resolve()
                if t.exists():
                    if _di_dalam(t, DILARANG_FW):
                        masalah.append(f"{f.relative_to(ROOT).as_posix()} meng-include {t.relative_to(ROOT).as_posix()}")
                    else:
                        antre.append(t)
                    break
    return masalah, dilihat


def _modul_tools():
    """nama modul -> berkas, untuk semua skrip di tools/."""
    return {p.stem: p for p in skrip_tools()}


def cek_skrip():
    modul = _modul_tools()
    masalah, dilihat = [], 0
    for d in PC_FINAL:
        for f in (ROOT / d).rglob("*.py"):
            dilihat += 1
            for m in IMP.finditer(f.read_text(encoding="utf-8", errors="replace")):
                p = modul.get(m.group(1))
                if p is not None and _di_dalam(p, DILARANG_PC):
                    masalah.append(f"{f.relative_to(ROOT).as_posix()} mengimpor {m.group(1)} ({p.relative_to(ROOT).as_posix()})")
    return masalah, dilihat


def main():
    mf, nf = cek_firmware()
    ms, ns = cek_skrip()
    print(f"firmware final: {nf} berkas diperiksa, {len(mf)} pelanggaran")
    print(f"skrip PC final: {ns} berkas diperiksa, {len(ms)} pelanggaran")
    for x in mf + ms:
        print("  PELANGGARAN:", x)
    return 1 if (mf or ms) else 0


if __name__ == "__main__":
    raise SystemExit(main())
