"""Pemeriksa tautan dokumen (24 Nov 2026): setiap rujukan `docs/...`, `laporan/...`, `logbook/...` harus menunjuk berkas/folder yang ada.

Memindai semua berkas teks yang dilacak git (Markdown, kode, ini) kecuali data mentah dan tabel pemetaan path lama.
Pakai:  python tools/4_verifikasi/cek_tautan_dokumen.py        (kode keluar 1 bila ada rujukan putus)
"""
import re
import subprocess
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)

RUJUKAN = re.compile(r"(?<![\w/.-])((?:docs|laporan|logbook)/[A-Za-z0-9_./\-]*[A-Za-z0-9_/])")
TEKS = {".md", ".py", ".cpp", ".h", ".c", ".ini", ".txt", ".sh", ".json"}
ABAIKAN_BERKAS = {"docs/referensi/PEMETAAN_PATH.md"}   # memuat path LAMA dengan sengaja
ABAIKAN_AWALAN = ("data/", "lib/mbedtls/")   # data mentah/hasil tersimpan; pustaka upstream


def _berkas():
    keluar = subprocess.run(["git", "ls-files"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8").stdout
    for rel in keluar.splitlines():
        if rel in ABAIKAN_BERKAS or rel.startswith(ABAIKAN_AWALAN) or _pl.Path(rel).suffix not in TEKS:
            continue
        if (ROOT / rel).is_file():
            yield rel


def _wajar(ref):
    """Rujukan placeholder (YYYY, <...>, *, ...) bukan tautan sungguhan."""
    return any(x in ref for x in ("YYYY", "NN", "<", "*", "..")) or ref.endswith(("/", "-", "_"))


def cek():
    putus = []
    for rel in _berkas():
        isi = (ROOT / rel).read_text(encoding="utf-8", errors="replace")
        for m in RUJUKAN.finditer(isi):
            ref = m.group(1).rstrip(".")
            if _wajar(ref) or (ROOT / ref).exists() or (ROOT / (ref + ".md")).exists():
                continue
            putus.append((rel, ref))
    return sorted(set(putus))


def main():
    putus = cek()
    print(f"rujukan putus: {len(putus)}")
    for rel, ref in putus:
        print(f"  {rel}: {ref}")
    return 1 if putus else 0


if __name__ == "__main__":
    raise SystemExit(main())
