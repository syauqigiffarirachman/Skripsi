"""Cek reproduksi biner firmware semua board (16 Nov 2026).

Membangun env PlatformIO dari BERSIH lalu menyimpan / membandingkan sha256 `firmware.bin` dengan acuan
di tools/4_verifikasi/biner_acuan.json. Dipakai setiap refaktor atau perapian kode: isi Flash identik = perilaku
identik, jadi data yang sudah diambil tetap sah untuk kode baru. Yang dibandingkan .bin, BUKAN .elf
(ELF ikut berubah bila path berkas / nomor baris berubah karena memuat info debug).

Build selalu dari bersih (folder .pio/build/<env> dihapus dulu) supaya tidak ada objek basi, mis. Mbed TLS
yang tidak ikut dibangun ulang saat include/mbedtls_*_config.h berubah.

Pakai (dari folder proyek):
  python tools/4_verifikasi/cek_biner.py simpan  [env ...]   bangun + simpan sebagai acuan (semua env bila kosong)
  python tools/4_verifikasi/cek_biner.py banding [env ...]   bangun + bandingkan dengan acuan
  python tools/4_verifikasi/cek_biner.py banding --tanpa-build  hanya hash biner yang sudah ada di .pio/build
  python tools/4_verifikasi/cek_biner.py daftar              daftar env di platformio.ini

Keluaran build tiap env: .pio/cek_biner/<env>.log. Kode keluar 1 bila ada env BEDA / GAGAL / tanpa acuan.
Bila firmware memang sengaja diubah: jalankan `simpan <env>` untuk env itu dan catat alasannya di logbook.
"""
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

ACUAN = ROOT / "tools" / "4_verifikasi" / "biner_acuan.json"
LOGDIR = ROOT / ".pio" / "cek_biner"
PIO_HOME = Path.home() / ".platformio"
PIO = PIO_HOME / "penv" / "Scripts" / "pio.exe"
SIZE = PIO_HOME / "packages" / "toolchain-gccarmnoneeabi" / "bin" / "arm-none-eabi-size.exe"
PAKET = {                                     # versi yang menentukan hasil kompilasi
    "platform-ststm32": PIO_HOME / "platforms" / "ststm32" / "platform.json",
    "framework-arduinoststm32": PIO_HOME / "packages" / "framework-arduinoststm32" / "package.json",
    "toolchain-gccarmnoneeabi": PIO_HOME / "packages" / "toolchain-gccarmnoneeabi" / "package.json",
}


def daftar_env():
    teks = (ROOT / "platformio.ini").read_text(encoding="utf-8")
    return re.findall(r"^\[env:([^\]]+)\]", teks, flags=re.M)


def versi_paket():
    hasil = {}
    for nama, berkas in PAKET.items():
        try:
            hasil[nama] = json.loads(berkas.read_text(encoding="utf-8")).get("version", "?")
        except (OSError, ValueError):
            hasil[nama] = "?"
    return hasil


def commit_git():
    try:
        rev = subprocess.run(["git", "rev-parse", "--short=12", "HEAD"], cwd=ROOT, capture_output=True,
                             text=True, check=True).stdout.strip()
        kotor = subprocess.run(["git", "status", "--porcelain", "--", "src", "lib", "include",
                                "platformio.ini", "boards"], cwd=ROOT, capture_output=True,
                               text=True).stdout.strip()
        return rev + ("+ubah" if kotor else "")
    except (OSError, subprocess.CalledProcessError):
        return "?"


def bangun(env):
    """Bangun satu env dari bersih. True bila berhasil."""
    shutil.rmtree(ROOT / ".pio" / "build" / env, ignore_errors=True)
    LOGDIR.mkdir(parents=True, exist_ok=True)
    pio = str(PIO) if PIO.exists() else "pio"
    t0 = time.time()
    with open(LOGDIR / f"{env}.log", "w", encoding="utf-8", errors="replace") as log:
        p = subprocess.run([pio, "run", "-e", env], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    print(f"  build {env:<22} {'OK' if p.returncode == 0 else 'GAGAL'}  ({time.time() - t0:.0f} s)",
          flush=True)
    return p.returncode == 0


def ukur(env):
    """sha256 + ukuran firmware.bin, serta text/data/bss dari ELF. None bila belum ada biner."""
    bdir = ROOT / ".pio" / "build" / env
    binf = bdir / "firmware.bin"
    if not binf.exists():
        return None
    data = binf.read_bytes()
    hasil = {"sha256": hashlib.sha256(data).hexdigest(), "ukuran": len(data)}
    elf = bdir / "firmware.elf"
    if SIZE.exists() and elf.exists():
        out = subprocess.run([str(SIZE), str(elf)], capture_output=True, text=True).stdout.splitlines()
        if len(out) >= 2:
            text, dat, bss = (int(x) for x in out[1].split()[:3])
            hasil.update({"text": text, "data": dat, "bss": bss})
    return hasil


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("aksi", choices=["simpan", "banding", "daftar"])
    ap.add_argument("env", nargs="*", help="env yang diproses (bawaan: semua env di platformio.ini)")
    ap.add_argument("--tanpa-build", action="store_true", help="jangan membangun, hash biner yang sudah ada")
    a = ap.parse_args()

    semua = daftar_env()
    if a.aksi == "daftar":
        print("\n".join(semua))
        return 0
    envs = a.env or semua
    asing = [e for e in envs if e not in semua]
    if asing:
        print(f"env tidak ada di platformio.ini: {', '.join(asing)}")
        return 1

    gagal = []
    if not a.tanpa_build:
        print(f"membangun {len(envs)} env dari bersih (log: {LOGDIR.relative_to(ROOT)})")
        gagal = [e for e in envs if not bangun(e)]

    paket = versi_paket()
    hasil = {e: ukur(e) for e in envs if e not in gagal}

    if a.aksi == "simpan":
        acuan = json.loads(ACUAN.read_text(encoding="utf-8")) if ACUAN.exists() else {"env": {}}
        acuan["diperbarui"] = datetime.now().isoformat(timespec="seconds")
        acuan["commit"] = commit_git()
        acuan["paket"] = paket
        for e, h in hasil.items():
            if h:
                acuan["env"][e] = h
        ACUAN.write_text(json.dumps(acuan, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        for e in envs:
            h = hasil.get(e)
            print(f"  {e:<22} {h['sha256'][:12] + '  ' + str(h['ukuran']) + ' B' if h else 'TIDAK ADA BINER'}")
        print(f"acuan disimpan: {ACUAN.relative_to(ROOT)} (commit {acuan['commit']})")
        return 1 if gagal else 0

    if not ACUAN.exists():
        print(f"acuan belum ada: jalankan dulu `python tools/4_verifikasi/cek_biner.py simpan`")
        return 1
    acuan = json.loads(ACUAN.read_text(encoding="utf-8"))
    beda_paket = {k: (acuan.get("paket", {}).get(k), v) for k, v in paket.items()
                  if acuan.get("paket", {}).get(k) != v}
    for k, (lama, baru) in beda_paket.items():
        print(f"PERINGATAN: versi {k} berubah {lama} -> {baru}; biner bisa beda walau sumber sama")

    buruk = 0
    print(f"{'env':<22} {'acuan':<14} {'sekarang':<14} status")
    for e in envs:
        ref = acuan["env"].get(e)
        h = hasil.get(e)
        if e in gagal or h is None:
            status, sek = "GAGAL BUILD" if e in gagal else "TIDAK ADA BINER", "-"
        else:
            sek = h["sha256"][:12]
            if not ref:
                status = "TANPA ACUAN"
            elif ref["sha256"] == h["sha256"]:
                status = "IDENTIK"
            else:
                status = f"BEDA (ukuran {ref['ukuran']} -> {h['ukuran']} B)"
        buruk += status != "IDENTIK"
        print(f"{e:<22} {(ref or {}).get('sha256', '-')[:12]:<14} {sek:<14} {status}")
    print("SEMUA IDENTIK" if buruk == 0 else f"{buruk} env tidak identik / gagal")
    return 1 if buruk else 0


if __name__ == "__main__":
    sys.exit(main())
