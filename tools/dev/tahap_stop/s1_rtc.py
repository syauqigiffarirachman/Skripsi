"""Deep sleep langkah 6 / S1: Stop + bangun alarm RTC, T_sleep host (RTC) vs logger (PB1) (17 Nov 2026).

Host (env host_s1_rtc) tiap 1 s: bangun alarm RTC -> kerja 50 ms (PB1 = 1) -> Stop (PB1 = 0). Tiap 30
siklus host mengirim lewat USART1 (logger: baris h):
  S1,<jendela>,<K>,<t_obs_us>,<t_sleep_us>,<n_stop>,<bangun_palsu>,<pulih_maks_us>,<tidur_min_us>,<tidur_maks_us>
Logger merekam saja (perintah 7). Untuk tiap pasangan laporan berurutan (jendela j-1 -> j):
  - T_sleep logger = jumlah lama PB1 = 0 di antara kedua baris h (kristal 72 MHz);
  - T_sleep host dikoreksi selisih kristal LSE vs logger (--ppm, hasil langkah 4: -47,3);
  - %S_cpu host = t_sleep / t_obs vs logger = T_sleep_logger / selang antar baris h (ppm saling hapus);
  - arus rata-rata INA226 saat PB1 = 0 (tidur) dan PB1 = 1 (kerja), tepi dipangkas.
Kriteria lulus langkah 6: |selisih T_sleep terkoreksi| < 1 ms per jendela, bangun_palsu = 0,
jumlah tidur logger = n_stop, arus tidur ~ lantai S0 (1,99 mA).

Pakai: python tools/dev/tahap_stop/s1_rtc.py --logger COM18 75 [--catatan ...] [--ppm -47.3] [--rshunt 0.1 --offset -2]
       python tools/dev/tahap_stop/s1_rtc.py --analyse data/dev/s1/<berkas>.csv
"""
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import statistics
import sys
from datetime import datetime
from pathlib import Path

from run_bench import Logger  # noqa: E402

OUT = ROOT / "data" / "dev" / "s1"
LSB_V = 2.5e-6
COLS = ["win", "K", "t_obs", "t_sleep", "n_stop", "palsu", "pulih_maks", "tidur_min", "tidur_maks"]


def analyse(text, ppm=-47.3, rshunt=0.100, offset=-2.0):
    hz, s, edges, rep = 72_000_000, [], [], []
    for ln in text.splitlines():
        ln = ln.strip()
        if ln.startswith("#RUN,BEGIN,"):
            hz = int(ln.split(",")[5])
        elif ln.startswith("s,"):
            _, t, sh, _bus = ln.split(",")
            s.append((int(t), int(sh)))
        elif ln.startswith("e,"):
            _, t, r = ln.split(",")
            edges.append((int(t), r == "1"))
        elif ln.startswith("h,"):
            f = ln.split(",")
            if len(f) >= 3 + len(COLS) and f[2] == "S1" and f[3].isdigit():
                rep.append((int(f[1]), dict(zip(COLS, map(int, f[3:3 + len(COLS)])))))
            elif len(f) > 3 and f[2] == "S1":
                print("  host:", ",".join(f[2:]))
    edges.sort()
    lows = [(a[0], b[0]) for a, b in zip(edges, edges[1:]) if not a[1] and b[1]]     # turun -> naik
    highs = [(a[0], b[0]) for a, b in zip(edges, edges[1:]) if a[1] and not b[1]]
    us = lambda dt: dt * 1e6 / hz  # noqa: E731

    def cur(iv, pre_ms, post_ms):
        v = [x[1] for x in s for (a, b) in [iv] if a + pre_ms * hz / 1000 <= x[0] <= b - post_ms * hz / 1000]
        return v

    hasil = []
    for (ta, ra), (tb, rb) in zip(rep, rep[1:]):
        if rb["win"] != ra["win"] + 1:
            continue
        # Tidur pertama jendela j dimulai tepat setelah laporan j-1 tuntas dikirim; logger mencap baris h
        # sedikit SETELAH itu (polling), jadi tepi turunnya bisa sedikit mendahului ta. Tidur terakhir
        # jendela j-1 berakhir >= S1_WORK_MS (50 ms) sebelum ta, jadi batas ta - 20 ms aman.
        lw = [(a, b) for a, b in lows if ta - 0.020 * hz < a and b < tb]
        hi = [(a, b) for a, b in highs if ta < a and b < tb]
        ts_log = sum(us(b - a) for a, b in lw)
        host_c = rb["t_sleep"] / (1 + ppm * 1e-6)
        sl, sh = [], []
        for iv in lw:
            sl += cur(iv, 3, 1)
        for iv in hi:
            sh += cur(iv, 3, 1)
        to_i = lambda v: (statistics.fmean(v) - offset) * LSB_V / rshunt * 1e3 if v else float("nan")  # noqa: E731
        dur = [us(b - a) for a, b in lw]
        hasil.append({
            "jendela": rb["win"], "n_stop_host": rb["n_stop"], "n_tidur_logger": len(lw), "palsu": rb["palsu"],
            "Tsleep_host_us": rb["t_sleep"], "Tsleep_logger_us": round(ts_log, 1),
            "selisih_mentah_us": round(rb["t_sleep"] - ts_log, 1), "selisih_koreksi_us": round(host_c - ts_log, 1),
            "Scpu_host_%": round(rb["t_sleep"] / rb["t_obs"] * 100, 4),
            "Scpu_logger_%": round(ts_log / us(tb - ta) * 100, 4),
            "tidur_logger_min_maks_us": f"{min(dur):.0f}/{max(dur):.0f}" if dur else "-",
            "tidur_host_min_maks_us": f"{rb['tidur_min']}/{rb['tidur_maks']}",
            "pulih_maks_us(kasar)": rb["pulih_maks"],
            "I_tidur_mA": round(to_i(sl), 3), "I_kerja_mA": round(to_i(sh), 3),
        })
    return {"laporan_host": len(rep), "tepi": len(edges), "jendela_banding": hasil}


def lulus(j):
    return (abs(j["selisih_koreksi_us"]) < 1000 and j["palsu"] == 0 and j["n_stop_host"] == j["n_tidur_logger"])


def cetak(r):
    print(f"  laporan host: {r['laporan_host']}  tepi PB1: {r['tepi']}")
    for j in r["jendela_banding"]:
        print("  " + "  ".join(f"{k}={v}" for k, v in j.items()) + ("  -> LULUS" if lulus(j) else "  -> GAGAL"))
    if not r["jendela_banding"]:
        print("  tidak ada pasangan laporan berurutan (rekam lebih lama? host jalan?)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("detik", nargs="?", type=int, default=75)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--catatan", default="")
    ap.add_argument("--ppm", type=float, default=-47.3)
    ap.add_argument("--rshunt", type=float, default=0.100)
    ap.add_argument("--offset", type=float, default=-2.0)
    ap.add_argument("--analyse", metavar="CSV")
    a = ap.parse_args()
    if a.analyse:
        cetak(analyse(Path(a.analyse).read_text(errors="replace"), a.ppm, a.rshunt, a.offset))
        return
    OUT.mkdir(parents=True, exist_ok=True)
    lg = Logger(a.logger)
    try:
        raw = lg.command(7, a.detik, a.detik + 20)
    finally:
        lg.close()
    f = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_s1_{a.catatan or 'run'}.csv"
    f.write_bytes(raw)
    print(f"log: {f}")
    cetak(analyse(raw.decode(errors="replace"), a.ppm, a.rshunt, a.offset))


if __name__ == "__main__":
    main()
