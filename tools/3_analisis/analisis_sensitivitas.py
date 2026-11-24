"""B3 + B5: sensitivitas toleransi frame hilang (K4) dan pelaporan nilai p.

Membaca data/analisis/stop_final/runs.csv (keluaran analisis_d.py --stop). Hanya-baca terhadap data mentah.
Keluaran di data/analisis/stop_final/sensitivitas/:
  toleransi_k4.csv   : jumlah run K4 yang lolos pada tiap ambang toleransi + rerata metrik K4
  banding_k4_semua.csv: K3 vs K4 (Welch) memakai 30 run sah vs semua 41 run K4
  p_nilai.csv        : p tiap perbandingan K1-K2, K3-K4 dilaporkan t, df, p (log10) dan teks "< 1e-100"
"""
from __future__ import annotations

import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import math
from pathlib import Path

import numpy as np
import pandas as pd
from scipy import stats

D = ROOT / "data" / "analisis" / "stop_final"
OUT = D / "sensitivitas"
M = [("lat_mean_ms_nyata", "ms"), ("P_dyn_mW", "mW"), ("S_cpu_log", "%"), ("E_lyn_mJ", "mJ"),
     ("I_dyn_mA", "mA"), ("TL_pct", "%")]


def welch(a, b):
    """t, df, p dua sisi; p lewat log agar tak jatuh ke 0 (log10 p dari fungsi survival)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    va, vb = a.var(ddof=1) / len(a), b.var(ddof=1) / len(b)
    t = (a.mean() - b.mean()) / math.sqrt(va + vb)
    df = (va + vb) ** 2 / (va ** 2 / (len(a) - 1) + vb ** 2 / (len(b) - 1))
    logp = (stats.t.logsf(abs(t), df) + math.log(2)) / math.log(10)      # log10 p
    return t, df, logp


def teks_p(logp):
    if logp < -100:
        return "< 1e-100"
    if logp < -3:
        return f"{10 ** logp:.1e}"
    return f"{10 ** logp:.4f}"


def main():
    r = pd.read_csv(D / "runs.csv")
    ok = r[r.status.eq("OK") & ~r.kelompok.str.contains("IDLE")]
    k4 = ok[ok.kelompok.eq("K4")]
    OUT.mkdir(exist_ok=True)

    # 1. ambang toleransi (frame hilang / run yang tiba)
    rows = []
    frac = k4.rx_lost / k4.N_dtg
    for amb in (0.0, 0.0015, 0.0025, 0.0035, 0.005, 0.01):
        s = k4[frac <= amb] if amb == 0 else k4[frac < amb]
        row = {"ambang_pct": amb * 100, "n_run": len(s)}
        for c, _ in M:
            row[c] = s[c].mean() if len(s) else np.nan
        rows.append(row)
    pd.DataFrame(rows).to_csv(OUT / "toleransi_k4.csv", index=False)

    # 2. K3 vs K4: sah (30) vs semua (41)
    k3 = ok[ok.kelompok.eq("K3")]
    hasil = []
    for nama, s4 in (("K4 sah (30)", k4[k4.sah]), ("K4 semua (%d)" % len(k4), k4)):
        for c, _ in M:
            t, df, lp = welch(k3[c], s4[c])
            hasil.append({"set": nama, "metrik": c, "K3_mean": k3[c].mean(), "K4_mean": s4[c].mean(),
                          "K4_n": len(s4), "gain_pct": (k3[c].mean() - s4[c].mean()) / k3[c].mean() * 100,
                          "t": t, "df": df, "log10_p": lp, "p": teks_p(lp), "arah_sama": np.sign(t)})
    h = pd.DataFrame(hasil)
    # arah sama antara dua set?
    h["kesimpulan_sama"] = h.groupby("metrik")["arah_sama"].transform(lambda x: x.nunique() == 1) & \
        h.groupby("metrik")["log10_p"].transform(lambda x: (x < -3).all() or (x >= -3).all())
    h.drop(columns="arah_sama").to_csv(OUT / "banding_k4_semua.csv", index=False)

    # 3. nilai p (B5)
    pr = []
    for (a, b, lab) in (("K1", "K2", "Normal"), ("K3", "K4", "Barrage")):
        for c, _ in M:
            x, y = ok[(ok.kelompok == a) & ok.sah][c], ok[(ok.kelompok == b) & ok.sah][c]
            if x.std() == 0 and y.std() == 0:
                continue
            t, df, lp = welch(x, y)
            pr.append({"skenario": lab, "metrik": c, "t": t, "df": df, "log10_p": lp, "p_teks": teks_p(lp)})
    pd.DataFrame(pr).to_csv(OUT / "p_nilai.csv", index=False)
    print(pd.read_csv(OUT / "toleransi_k4.csv").to_string())
    print(h.to_string())
    print(pd.DataFrame(pr).to_string())


if __name__ == "__main__":
    main()
