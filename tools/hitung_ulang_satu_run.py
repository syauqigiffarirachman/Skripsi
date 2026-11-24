"""Hitung ulang SATU run dari CSV mentah, TANPA memakai run_bench.py (hanya pustaka standar).

Tujuan belajar/pembuktian: menunjukkan bahwa angka di tabel (runs.csv) memang keluar dari rumus yang
ditulis di paper, langsung dari baris mentah logger. Tiap baris kode di bawah menunjuk rumusnya.

Pakai:
  python tools/hitung_ulang_satu_run.py data/runs/20261118_115044_K2S/033_K2S_A.csv --p-idle 12.766 --i-idle 2.449
  tambahkan --banding data/analisis/stop_final/runs.csv --sesi 20261118_115044_K2S  untuk membandingkan dengan tabel

P_idle / I_idle = rerata run IDLE arsitektur dan blok yang sama (kolom P_idle_mW dan I_idle_mA di runs.csv).
Kalibrasi bawaan --rshunt/--offset = hasil 21 Nov 2026 (0,0821 Ohm, -2,83; tools/kalibrasi_param.py, modul kecil yang
juga hanya memakai pustaka standar). Nilai nominal lama: --rshunt 0.100 --offset -2.0 (dengan --p-idle 10.37 --i-idle 1.99).
"""
import argparse
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import kalibrasi_param as kp  # noqa: E402

TICK_HZ = 72_000_000          # jam logger (kristal) = satuan semua cap waktu
SHUNT_LSB_V = kp.SHUNT_LSB_V   # INA226: 2,5 uV per LSB register shunt
BUS_LSB_V = 1.25e-3           # INA226: 1,25 mV per LSB register bus


def baca(path):
    s, e, h = [], [], []
    for baris in Path(path).read_text(errors="replace").splitlines():
        f = baris.strip().split(",")
        if f[0] == "s":
            s.append((int(f[1]), int(f[2]), int(f[3])))        # (tik, shunt_raw, bus_raw)
        elif f[0] == "e":
            e.append((int(f[1]), int(f[2])))                   # (tik, 1=naik/0=turun) pin status PB1 host
        elif f[0] == "h":
            h.append((int(f[1]), f[2:]))
    return s, e, h


def hitung(path, rshunt, offset, p_idle_mw=None, i_idle_ma=None):
    s, e, h = baca(path)
    t_start = next(t for t, f in h if f[0] == ">START")
    t_end_line, end = next((t, f[1:]) for t, f in h if f[0] == ">END")
    (arch, secs, p_ms, uart, t_obs_us, t_sleep_us, act_us, resid_us, arrived, served, gagal, wfi, spurious,
     pin_salah, rx_ok, rx_bad, rx_lost, lat_n, lat_min, lat_mean, lat_max, lat_drop, clk_hz) = \
        [x if i == 0 else int(x) for i, x in enumerate(end)]

    # --- jendela ukur menurut LOGGER: tepi naik pertama PB1 ... tepi turun terakhir (Bab 3.5.1) ---
    ed = [(t, v) for t, v in e if t_start - TICK_HZ // 500 <= t <= t_end_line]
    t0 = next(t for t, v in ed if v == 1)
    t1 = [t for t, v in ed if v == 0][-1]
    T = (t1 - t0) / TICK_HZ                                      # T_obs (detik, jam kristal logger)

    # --- S_cpu = waktu PB1 rendah / T_obs (pin rendah = CPU tidur)  [rumus 3.5.1 butir 3, sisi logger] ---
    rendah, turun = 0, None
    for t, v in ed:
        if t0 <= t <= t1:
            if v == 0:
                turun = t
            elif turun is not None:
                rendah += t - turun
                turun = None
    s_cpu_log = rendah / (t1 - t0) * 100
    s_cpu_host = t_sleep_us / t_obs_us * 100                     # sisi firmware: Tsleep / Tobs x 100

    # --- energi: E = integral V.I dt, arus I = (shunt - offset) x LSB / Rshunt, trapesium pada [t0, t1] ---
    pts = [(t, (sh - offset) * SHUNT_LSB_V / rshunt, bus * BUS_LSB_V) for t, sh, bus in s]

    def pada(t):                                                  # interpolasi linear di batas jendela
        for a, b in zip(pts, pts[1:]):
            if a[0] <= t <= b[0]:
                w = (t - a[0]) / (b[0] - a[0]) if b[0] != a[0] else 0
                return (t, a[1] + w * (b[1] - a[1]), a[2] + w * (b[2] - a[2]))
    win = [pada(t0)] + [p for p in pts if t0 < p[0] < t1] + [pada(t1)]
    E = Q = 0.0
    for a, b in zip(win, win[1:]):
        dt = (b[0] - a[0]) / TICK_HZ
        E += (a[1] * a[2] + b[1] * b[2]) / 2 * dt                 # joule
        Q += (a[1] + b[1]) / 2 * dt                               # coulomb
    out = {"T_obs_log_s": T, "S_cpu_log": s_cpu_log, "S_cpu_host": s_cpu_host,
           "I_avg_mA": Q / T * 1e3, "P_avg_mW": E / T * 1e3, "E_mJ": E * 1e3,
           "N_dtg": arrived, "N_lyn": served}

    # --- latensi: siklus x 1000 / f_clk terukur, lalu dikoreksi rasio jam logger terhadap jam host ---
    host_T = t_obs_us / 1e6
    lat_ms_nyata = lat_mean * 1e3 / clk_hz * (T / host_T)
    out["lat_mean_ms_nyata"] = lat_ms_nyata
    # --- tingkat layanan TL = N_lyn / N_dtg x 100 ; model TL = min(1, 1/(lambda t_sign)) x 100 ---
    out["TL_pct"] = served / arrived * 100
    out["TL_model_pct"] = min(1.0, 1 / (arrived / T * lat_ms_nyata / 1e3)) * 100
    out["N_rusak"] = rx_bad + rx_lost
    # --- metrik dinamis: P_dyn = P_avg - P_idle ; E_lyn = P_dyn x T / N_lyn (= (E - P_idle T)/N_lyn) ---
    if p_idle_mw is not None:
        out["P_dyn_mW"] = out["P_avg_mW"] - p_idle_mw
        out["E_lyn_mJ"] = out["P_dyn_mW"] * T / served
    if i_idle_ma is not None:
        out["I_dyn_mA"] = out["I_avg_mA"] - i_idle_ma
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF)
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB)
    ap.add_argument("--p-idle", type=float, default=None, help="P_idle (mW) blok yang sama")
    ap.add_argument("--i-idle", type=float, default=None, help="I_idle (mA) blok yang sama")
    ap.add_argument("--banding", help="runs.csv hasil analisis_d.py")
    ap.add_argument("--sesi", help="nama folder sesi (untuk memilih baris di runs.csv)")
    a = ap.parse_args()

    ref = {}
    if a.banding:
        nama = Path(a.csv).name
        for r in csv.DictReader(open(a.banding, encoding="utf-8")):
            if r["file"] == nama and (a.sesi is None or r["sesi"] == a.sesi):
                ref = r
                break
        if a.p_idle is None and ref.get("P_idle_mW"):
            a.p_idle = float(ref["P_idle_mW"])
        if a.i_idle is None and ref.get("I_idle_mA"):
            a.i_idle = float(ref["I_idle_mA"])
    hasil = hitung(a.csv, a.rshunt, a.offset, a.p_idle, a.i_idle)
    print(f"{'metrik':20s} {'hitung ulang':>16s} {'runs.csv':>16s} {'selisih rel.':>14s}")
    for k, v in hasil.items():
        r = ref.get(k)
        try:
            rv = float(r)
            sel = f"{(v - rv) / rv * 100:+.4f} %" if rv else "-"
            print(f"{k:20s} {v:16.6f} {rv:16.6f} {sel:>14s}")
        except (TypeError, ValueError):
            print(f"{k:20s} {v:16.6f} {'-':>16s} {'-':>14s}")


if __name__ == "__main__":
    main()
