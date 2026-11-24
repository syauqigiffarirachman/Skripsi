"""Diagnosis tanda tangan lambat (~210 ms) K2S saat barrage (18 Nov 2026), firmware env host_k2_stop_long_auto.

Menjalankan N siklus skenario B (attacker 10 Hz, jendela 60 s). Tiap siklus: kumpulkan baris h >END (frame hilang) dan
>LNG,idx,lat_us,ens_us,send_us,busy_us,read_us,dstop,err (tiap tanda tangan > 150 ms), lalu cetak tabel dan ringkasan.
Pakai: python tools/4_verifikasi/diag_long.py [--n 6] [--secs 60] [--logger COM18] [--attacker COM24]
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import sys
import time
from datetime import datetime
from pathlib import Path

import run_bench as rb  # noqa: E402

OUT = ROOT / "data" / "dev" / "long"
GAP_S, CAL_S, KIRIM_S = 8, 2, 8


def satu_siklus(lg, at, secs):
    def on_line(line):
        if line.startswith(b"h,") and b">START" in line:
            at.send("b")
        elif line.startswith(b"h,") and b">END" in line:
            at.send("x")
    siklus = GAP_S + secs + CAL_S + KIRIM_S
    lg.command(7, siklus + 25, siklus + 55)
    return lg.command(7, siklus + 20, siklus + 50, on_line).decode(errors="replace")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--n", type=int, default=6)
    ap.add_argument("--secs", type=int, default=60)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--attacker", default="COM24")
    a = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    at = rb.Attacker(a.attacker)
    lg = rb.Logger(a.logger)
    tot_lost = tot_long = 0
    try:
        at.stop()
        for i in range(a.n):
            text = satu_siklus(lg, at, a.secs)
            f = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_long_{i + 1}.csv"
            f.write_text(text)
            r = rb.parse(text)
            en = r["end"]
            lng = [ln.split(",") for ln in text.splitlines() if ",>LNG," in ln]
            lost = en["rx_lost"] if en else -1
            tot_lost += max(lost, 0)
            tot_long += len(lng)
            print(f"siklus {i + 1}: hilang={lost} bad={en['rx_bad'] if en else '?'} gagal={en['gagal'] if en else '?'} tanda_tangan_lambat={len(lng)}")
            wk = [ln.split(",") for ln in text.splitlines() if ",>WK," in ln]
            for x in lng:
                print("   LNG lat_us,ens_us,send,busy,read,dstop,err =", ",".join(x[4:]))
                for w in wk:
                    if w[3] == x[3]:
                        print(f"      bangun {w[4]}: sebab={w[5]} (0 alarm,1 frame ok,2 frame rusak,3 palsu) tidur_us={w[6]} pada_t_us={w[7]}")
    finally:
        try:
            at.stop()
        except Exception as e:  # noqa: BLE001
            print("peringatan attacker:", e)
        lg.close()
        at.close()
    print(f"TOTAL hilang={tot_lost} tanda_tangan_lambat={tot_long}")


if __name__ == "__main__":
    main()
