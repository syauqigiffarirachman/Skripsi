"""Deep sleep langkah 5 / S0: lantai arus papan host di Stop mode (17 Nov 2026).

Host menjalankan env host_s0_stop: PB1 = 1 selama jendela aman 5 s setelah boot (CPU sibuk), lalu
PB1 = 0 dan Stop selamanya. Logger merekam saja (perintah 7): cuplikan INA226 (s) + tepi PB1 (e).
Skrip memotong rekaman jadi dua segmen:
  AKTIF = antara tepi naik dan tepi turun PB1 (dipangkas 0,2 s di tiap ujung), bila terekam;
  STOP  = dari tepi turun + 1 s sampai akhir (atau seluruh rekaman bila tak ada tepi = host sudah Stop).

PERINGATAN RESOLUSI: INA226 dengan shunt 0,1 Ohm = 25 uA/LSB, offset ~-2 LSB (diukur 15 Nov, bisa
bergeser). Lantai Stop yang diharapkan ~40 uA = ~1,6 LSB -> hasil di sini hanya uji KASAR (rata-rata
ribuan cuplikan memberi resolusi sub-LSB hanya bila derau cukup). Angka final butuh instrumen uA.
Arus bias INA226 (IN-, VBUS ~830 kOhm) ikut lewat shunt sebagai beban kecil (~uA): catat untuk alat uA.

Pakai:
  python tools/dev/tahap_stop/s0_stop.py --logger COM18 60 [--catatan ...] [--rshunt 0.1] [--offset -2]
  python tools/dev/tahap_stop/s0_stop.py --analyse data/dev/s0/<berkas>.csv
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[2]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import statistics
import sys
from datetime import datetime
from pathlib import Path

from run_bench import Logger  # noqa: E402

OUT = ROOT / "data" / "dev" / "s0"
LSB_V, BUS_LSB_V = 2.5e-6, 1.25e-3


def seg_stats(rows, rshunt, offset):
    if not rows:
        return None
    sh = [r[1] for r in rows]
    m = statistics.fmean(sh)
    sd = statistics.pstdev(sh)
    i_ua = (m - offset) * LSB_V / rshunt * 1e6
    return {"n": len(sh), "raw_rata": round(m, 3), "raw_sd": round(sd, 3), "raw_min": min(sh), "raw_maks": max(sh),
            "I_uA": round(i_ua, 1), "u_rata_uA": round(sd / len(sh) ** 0.5 * LSB_V / rshunt * 1e6, 2),
            "V": round(statistics.fmean(r[2] for r in rows) * BUS_LSB_V, 3)}


def analyse(text, rshunt=0.100, offset=-2.0):
    hz, s, rise, fall = 72_000_000, [], [], []
    for ln in text.splitlines():
        ln = ln.strip()
        if ln.startswith("#RUN,BEGIN,"):
            hz = int(ln.split(",")[5])
        elif ln.startswith("s,"):
            _, t, sh, bus = ln.split(",")
            s.append((int(t), int(sh), int(bus)))
        elif ln.startswith("e,"):
            _, t, r = ln.split(",")
            (rise if r == "1" else fall).append(int(t))
    out = {"cuplikan": len(s), "tepi_naik": len(rise), "tepi_turun": len(fall)}
    if fall:
        tf = fall[-1]
        tr = max([t for t in rise if t < tf], default=None)
        if tr is not None:
            out["aktif"] = seg_stats([x for x in s if tr + 0.2 * hz <= x[0] <= tf - 0.2 * hz], rshunt, offset)
        out["stop"] = seg_stats([x for x in s if x[0] >= tf + 1.0 * hz], rshunt, offset)
    else:
        out["stop"] = seg_stats(s, rshunt, offset)
        out["catatan"] = "tanpa tepi PB1: seluruh rekaman dianggap Stop"
    out["rshunt"], out["offset_lsb"] = rshunt, offset
    return out


def cetak(r):
    for k, v in r.items():
        if isinstance(v, dict):
            print(f"  [{k}] " + "  ".join(f"{a}={b}" for a, b in v.items()))
        else:
            print(f"  {k}: {v}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("detik", nargs="?", type=int, default=60)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--catatan", default="")
    ap.add_argument("--rshunt", type=float, default=0.100)
    ap.add_argument("--offset", type=float, default=-2.0)
    ap.add_argument("--analyse", metavar="CSV")
    a = ap.parse_args()
    if a.analyse:
        cetak(analyse(Path(a.analyse).read_text(errors="replace"), a.rshunt, a.offset))
        return
    OUT.mkdir(parents=True, exist_ok=True)
    lg = Logger(a.logger)
    try:
        raw = lg.command(7, a.detik, a.detik + 20)
    finally:
        lg.close()
    f = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_s0_{a.catatan or 'run'}.csv"
    f.write_bytes(raw)
    print(f"log: {f}")
    cetak(analyse(raw.decode(errors="replace"), a.rshunt, a.offset))


if __name__ == "__main__":
    main()
