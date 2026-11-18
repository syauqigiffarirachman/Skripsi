"""Provenance biner: asal-usul sebuah firmware.bin, DITULIS DI LUAR firmware.

Mengapa di luar: gerbang `tools/cek_biner.py` menuntut isi firmware.bin identik bit demi bit. Menanam
hash/commit ke dalam firmware akan mengubah biner. Jadi catatan ditulis sebagai berkas terpisah
(`provenance.json`) di samping firmware.bin, dan dibaca oleh alat PC.

Isi catatan: env, sha256 firmware.bin, git HEAD, apakah sumber "kotor" (ada perubahan belum di-commit
pada src/ lib/ include/ boards/ platformio.ini), daftar berkas yang berubah, waktu.

Hanya-baca terhadap repo: memakai `git --no-optional-locks` supaya tidak membuat/menahan `.git/index.lock`
(bisa bentrok dengan operasi git editor yang sedang berjalan).
"""
from __future__ import annotations

import datetime
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
JALUR_SUMBER = ("src", "lib", "include", "boards", "platformio.ini")
BATAS_DAFTAR = 20


def _git(*args: str, cwd: Path = ROOT) -> str | None:
    """Jalankan git baca-saja. None bila git tak ada, folder tak ada, atau perintah gagal."""
    try:
        p = subprocess.run(["git", "--no-optional-locks", *args], cwd=str(cwd), capture_output=True,
                           text=True, timeout=15, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return None
    # Hanya buang akhir baris: strip() penuh memakan spasi awal kolom status porcelain (" M src/a.cpp").
    return p.stdout.rstrip("\r\n") if p.returncode == 0 else None


def kumpulkan(bin_path: str | Path, env_name: str, root: Path = ROOT) -> dict:
    """Kumpulkan catatan provenance untuk satu firmware.bin (tidak menulis apa pun)."""
    data = Path(bin_path).read_bytes()
    head = _git("rev-parse", "HEAD", cwd=root)
    status = _git("status", "--porcelain", "--", *JALUR_SUMBER, cwd=root)
    berubah = [] if not status else [ln[3:] for ln in status.splitlines()]
    return {
        "env": env_name,
        "bin_sha256": hashlib.sha256(data).hexdigest(),
        "bin_ukuran": len(data),
        "git_head": head.strip() if head else None,
        "dirty": None if status is None else bool(status),       # None = tidak bisa ditentukan
        "berkas_berubah": berubah[:BATAS_DAFTAR],
        "berkas_berubah_total": len(berubah),
        "waktu": datetime.datetime.now().isoformat(timespec="seconds"),
    }


def tulis(bin_path: str | Path, env_name: str, root: Path = ROOT) -> Path:
    """Tulis provenance.json di samping firmware.bin (tulis ke berkas sementara lalu ganti nama)."""
    b = Path(bin_path)
    tujuan = b.parent / "provenance.json"
    sementara = tujuan.with_suffix(".json.tmp")
    sementara.write_text(json.dumps(kumpulkan(b, env_name, root), indent=2), encoding="utf-8")
    sementara.replace(tujuan)
    return tujuan


def baca(env_name: str, root: Path = ROOT) -> dict | None:
    """Baca provenance.json milik .pio/build/<env>/ (None bila belum ada atau rusak)."""
    p = root / ".pio" / "build" / env_name / "provenance.json"
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
