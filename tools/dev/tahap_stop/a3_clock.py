"""A3: galat jam HSI host terhadap kristal logger (15 Nov 2026).

Host menjalankan k1_sw_auto: jendela W30,1000 (permintaan TIM4 tiap 1000 ms menurut jam HOST),
PB1 naik di awal tiap sign. Logger (kristal 72 MHz) mengukur periode tepi naik PA0 dalam jendela 5 s
setiap 15 s dan mencetak baris:
    periode n=4 min=1005550.611 rata2=1005597.680 maks=1005637.402 us
Skrip ini merekam keluaran logger selama DURASI menit, menyimpan log mentah, lalu menghitung galat.

Laporan yang memuat batas jendela host (jeda 5 s antar jendela, pulsa awal 4 us) dibuang dengan filter:
semua periode di laporan itu harus di [PMIN, PMAX] us.

Pakai: python tools/dev/tahap_stop/a3_clock.py COM18 12 [catatan]
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[2]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import re
import statistics
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

PMIN, PMAX = 1_000_000.0, 1_010_000.0   # us; periode nominal 1 s, galat HSI F103 maks ~1-2 %
RE_PER = re.compile(r"periode n=(\d+) min=([\d.]+) rata2=([\d.]+) maks=([\d.]+) us")


def record(port: str, minutes: float, out: Path) -> str:
    s = serial.Serial(port, 115200, timeout=0.5)
    s.dtr = True
    t_end = time.time() + minutes * 60
    buf = bytearray()
    with out.open("wb") as f:
        while time.time() < t_end:
            chunk = s.read(4096)
            if chunk:
                f.write(chunk)
                f.flush()
                buf += chunk
    s.close()
    return buf.decode(errors="replace")


def analyse(text: str) -> None:
    rows = [(int(n), float(mn), float(av), float(mx)) for n, mn, av, mx in RE_PER.findall(text)]
    ok = [r for r in rows if r[0] >= 1 and r[1] >= PMIN and r[3] <= PMAX]
    print(f"laporan berperiode: {len(rows)}, lolos filter: {len(ok)}, dibuang: {len(rows) - len(ok)}")
    if not ok:
        print("tidak ada data sah (host tersambung? PB1 -> PA0?)")
        return
    n_tot = sum(r[0] for r in ok)
    mean = sum(r[0] * r[2] for r in ok) / n_tot          # rata2 tertimbang jumlah periode
    means = [r[2] for r in ok]
    sd = statistics.stdev(means) if len(means) > 1 else 0.0
    ppm = (mean / 1e6 - 1) * 1e6
    print(f"periode: {n_tot} periode dari {len(ok)} laporan")
    print(f"T rata2 = {mean:.3f} us  (min laporan {min(r[1] for r in ok):.3f}, maks {max(r[3] for r in ok):.3f})")
    print(f"sd rata2 antar-laporan = {sd:.3f} us  ({sd / 1e6 * 1e6:.1f} ppm)")
    print(f"galat jam host = {-ppm:+.0f} ppm (host LAMBAT bila negatif), HSI efektif = {48e6 * 1e6 / mean / 1e6:.4f} MHz")
    # tren: paruh awal vs paruh akhir (pemanasan papan host)
    h = len(ok) // 2
    if h >= 2:
        a = statistics.mean(r[2] for r in ok[:h])
        b = statistics.mean(r[2] for r in ok[h:])
        print(f"tren: paruh awal {a:.3f} us, paruh akhir {b:.3f} us, beda {b - a:+.3f} us ({(b - a) / 1e6 * 1e6:+.1f} ppm)")


def main() -> None:
    if len(sys.argv) >= 2 and sys.argv[1].endswith((".txt", ".log")):
        analyse(Path(sys.argv[1]).read_text(errors="replace"))
        return
    port = sys.argv[1] if len(sys.argv) > 1 else "COM18"
    minutes = float(sys.argv[2]) if len(sys.argv) > 2 else 12
    note = "_".join(sys.argv[3:])
    out_dir = ROOT / "data"
    out_dir.mkdir(exist_ok=True)
    out = out_dir / f"a3_clock_{datetime.now():%Y%m%d_%H%M}{('_' + note) if note else ''}.txt"
    print(f"rekam {minutes} menit dari {port} -> {out}")
    analyse(record(port, minutes, out))


if __name__ == "__main__":
    main()
