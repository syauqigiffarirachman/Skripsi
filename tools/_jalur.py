"""Jalur impor bersama semua skrip di tools/.

Skrip tersebar di subfolder (1_akuisisi, 2_kalibrasi, 3_analisis, 4_verifikasi, demo, dev/...), tetapi saling
mengimpor dengan nama modul biasa (mis. `import run_bench`). Berkas ini menambahkan semua subfolder ke sys.path
dan menyediakan ROOT = akar repositori. Tiap skrip memulai dengan:

    sys.path.insert(0, <folder tools/>)      # supaya `_jalur` ditemukan
    from _jalur import ROOT
"""
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parent


def skrip_tools():
    """Semua skrip .py di tools/ (emulator emu_cycles dikecualikan: ia berdiri sendiri)."""
    return (p for p in sorted(TOOLS.rglob("*.py")) if "emu_cycles" not in p.parts and "__pycache__" not in p.parts)


def _subfolder_modul():
    for d in sorted(TOOLS.rglob("*")):
        if d.is_dir() and not d.name.startswith(("__", ".")) and "emu_cycles" not in d.parts and any(d.glob("*.py")):
            yield d


for _d in [TOOLS, *_subfolder_modul()]:
    if str(_d) not in sys.path:
        sys.path.append(str(_d))
