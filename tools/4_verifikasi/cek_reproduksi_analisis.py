"""Bukti analisis dapat diulang (24 Nov 2026): jalankan analisis_d.py ke folder SEMENTARA dan bandingkan dengan hasil tersimpan.

Antarmuka: satu perintah, kode keluar 0 bila identik. Tidak menimpa data/analisis/stop_final/.
  python tools/4_verifikasi/cek_reproduksi_analisis.py

Dibandingkan: semua CSV/MD dan gambar yang dihasilkan analisis_d.py (akhir baris CRLF/LF diabaikan; baris 'Dibuat <waktu>'
di ringkasan.md diabaikan karena berisi stempel waktu). Hasil skrip lain (dekomposisi, sensitivitas, proyeksi, gambar Bab 4)
dibuktikan lewat jalankan_analisis.py lalu `git status` (tidak ada perubahan berarti identik).
"""
import re
import subprocess
import tempfile
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)

ACUAN = ROOT / "data" / "analisis" / "stop_final"
STAMPEL = re.compile(rb"(?m)^Dibuat \d{4}-\d{2}-\d{2} \d{2}:\d{2};")


def _isi(p):
    b = p.read_bytes()
    if p.suffix in (".csv", ".md"):
        b = STAMPEL.sub(b"Dibuat <waktu>;", b.replace(b"\r\n", b"\n"))
    return b


def bandingkan(baru):
    """Daftar berkas yang BEDA/hilang antara folder `baru` (hasil analisis_d) dan ACUAN."""
    masalah, n = [], 0
    for p in sorted(baru.rglob("*")):
        if not p.is_file():
            continue
        n += 1
        rel = p.relative_to(baru)
        ref = ACUAN / rel
        if not ref.exists():
            masalah.append(f"tidak ada di acuan: {rel.as_posix()}")
        elif _isi(p) != _isi(ref):
            masalah.append(f"beda: {rel.as_posix()}")
    return masalah, n


def main():
    with tempfile.TemporaryDirectory() as tmp:
        perintah = [_sys.executable, str(ROOT / "tools" / "3_analisis" / "analisis_d.py"), "--manifest",
                    str(ROOT / "data" / "tahap_stop.txt"), "--stop", "--keluar", tmp]
        if subprocess.run(perintah, cwd=ROOT, stdout=subprocess.DEVNULL).returncode != 0:
            raise SystemExit("analisis_d.py gagal")
        masalah, n = bandingkan(_pl.Path(tmp))
    print(f"{n} berkas dibandingkan dengan data/analisis/stop_final: {len(masalah)} beda")
    for m in masalah:
        print("  ", m)
    return 1 if masalah else 0


if __name__ == "__main__":
    raise SystemExit(main())
