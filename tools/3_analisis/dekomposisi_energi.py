"""C1: dekomposisi energi per tanda tangan (CPU aktif, IC menghitung, tidur) dari jejak arus mentah.

Hanya skenario Normal (K1, K2): satu episode = satu tanda tangan, jeda antar-episode cukup panjang.
Fase (menurut pin status PB1 dari logger, tick 72 MHz):
  aktif      : pin tinggi (CPU terjaga)
  menunggu   : pin rendah DI DALAM episode (K2: CPU tidur, IC DS28E38 menghitung; K1: kosong)
  tidur      : pin rendah di luar episode
Energi dinamis = integral (P - P_idle) dt, P_idle = rerata daya IDLE blok yang sama (kolom P_idle_mW runs.csv).
Hasil: data/analisis/stop_final/dekomposisi_energi.csv (+ .md). Memakai rshunt/offset seperti analisis utama.
"""
from __future__ import annotations

import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd

import run_bench as rb  # noqa: E402
import kalibrasi_param as kp  # noqa: E402

D = ROOT / "data" / "analisis" / "stop_final"
T = rb.TICK_HZ


def fase_run(path, rshunt, offset, p_idle_mw, i_idle_ma):
    text = Path(path).read_text(errors="replace")
    r = rb.parse(text)
    slack = T // 500
    ed = [(t, lv) for t, lv in r["e"] if r["t_start"] - slack <= t <= r["t_endline"]]
    rises = [t for t, lv in ed if lv == 1]
    falls = [t for t, lv in ed if lv == 0]
    t0, t1 = rises[0], falls[-1]
    # pulsa tinggi >= 1 ms; episode = pulsa dipisah jeda < 200 ms
    pulsa, naik = [], None
    for t, lv in ed:
        if lv == 1 and naik is None:
            naik = t
        elif lv == 0 and naik is not None:
            pulsa.append([naik, t])
            naik = None
    pulsa = [p for p in pulsa if (p[1] - p[0]) / T >= 0.001]
    ep = [pulsa[0][:]]
    for a, b in pulsa[1:]:
        if (a - ep[-1][1]) / T < 0.200:
            ep[-1][1] = b
        else:
            ep.append([a, b])
    ep_start = np.array([e[0] for e in ep]); ep_end = np.array([e[1] for e in ep])

    pts = [(x[0], (x[1] - offset) * rb.SHUNT_LSB_V / rshunt, x[2] * rb.BUS_LSB_V) for x in r["s"]]
    out = {"aktif": [0.0, 0.0, 0.0], "menunggu": [0.0, 0.0, 0.0], "tidur": [0.0, 0.0, 0.0]}  # dt(s), Q(C), E(J)
    j, state = 0, 0
    for a, b in zip(pts, pts[1:]):
        if b[0] <= t0 or a[0] >= t1:
            continue
        while j < len(ed) and ed[j][0] <= a[0]:
            state = ed[j][1]
            j += 1
        ts = max(a[0], t0)
        if state:
            f = "aktif"
        else:
            k = np.searchsorted(ep_start, ts, side="right") - 1
            f = "menunggu" if (k >= 0 and ts < ep_end[k]) else "tidur"
        dt = (min(b[0], t1) - ts) / T
        o = out[f]
        o[0] += dt
        o[1] += (a[1] + b[1]) / 2 * dt
        o[2] += (a[1] * a[2] + b[1] * b[2]) / 2 * dt
    n = len(ep)
    Tw = (t1 - t0) / T
    return out, n, Tw


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF)   # kalibrasi 21 Nov 2026
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB)
    a = ap.parse_args()
    runs = pd.read_csv(D / "runs.csv")
    runs = runs[runs.sah & runs.kelompok.isin(["K1", "K2"])]
    # path berkas
    rows = []
    for _, x in runs.iterrows():
        p = ROOT / "data" / "runs" / x["sesi"] / x["file"]
        out, n, Tw = fase_run(p, a.rshunt, a.offset, x["P_idle_mW"], x["I_idle_mA"])
        pid = x["P_idle_mW"] / 1e3          # W
        iid = x["I_idle_mA"] / 1e3          # A
        row = {"kelompok": x["kelompok"], "file": x["file"], "sesi": x["sesi"], "n_ep": n, "T_s": Tw,
               "E_dyn_ref_mJ": x["E_dyn_mJ"], "N_lyn": x["N_lyn"]}
        tot = 0.0
        for f, (dt, q, e) in out.items():
            ed_mj = (e - pid * dt) * 1e3
            row[f"dt_{f}_ms_per_sign"] = dt / x["N_lyn"] * 1e3
            row[f"I_{f}_mA"] = q / dt * 1e3 if dt else np.nan
            row[f"Edyn_{f}_mJ_per_sign"] = ed_mj / x["N_lyn"]
            tot += ed_mj
        row["Edyn_total_mJ_per_sign"] = tot / x["N_lyn"]
        row["selisih_vs_ref_pct"] = (tot / (x["E_dyn_mJ"]) - 1) * 100
        rows.append(row)
    d = pd.DataFrame(rows)
    d.to_csv(D / "dekomposisi_energi_per_run.csv", index=False)
    kol = [c for c in d.columns if c.startswith(("dt_", "I_", "Edyn_")) or c == "selisih_vs_ref_pct"]
    g = d.groupby("kelompok")[kol].agg(["mean", "std"])
    g.to_csv(D / "dekomposisi_energi.csv")
    # tabel markdown
    L = ["# Dekomposisi energi dinamis per tanda tangan (Normal)\n",
         f"rshunt = {a.rshunt} Ohm, offset = {a.offset} LSB; n = " + ", ".join(f"{k}: {len(v)} run" for k, v in d.groupby('kelompok')) + ".",
         f"Energi dinamis = integral (P - P_idle) dt per fase; dibagi jumlah tanda tangan terlayani. Nilai mutlak mengikuti kalibrasi shunt di atas (u_kal bawaan {kp.U_REL_KAL * 100:g} %, kalibrasi 21 Nov 2026).\n",
         "| Kelompok | Fase | Durasi (ms/sign) | Arus rerata (mA) | E dinamis (mJ/sign) | Porsi (%) |", "|---|---|---|---|---|---|"]
    for k in ("K1", "K2"):
        s = d[d.kelompok == k]
        tot = s["Edyn_total_mJ_per_sign"].mean()
        for f, nama in (("aktif", "CPU aktif"), ("menunggu", "IC menghitung (CPU tidur)"), ("tidur", "tidur di luar episode")):
            L.append(f"| {k} | {nama} | {s[f'dt_{f}_ms_per_sign'].mean():.2f} | {s[f'I_{f}_mA'].mean():.3f} | "
                     f"{s[f'Edyn_{f}_mJ_per_sign'].mean():.3f} (sd {s[f'Edyn_{f}_mJ_per_sign'].std():.3f}) | "
                     f"{s[f'Edyn_{f}_mJ_per_sign'].mean() / tot * 100:.1f} |")
        L.append(f"| {k} | **jumlah** | | | **{tot:.3f}** | 100 |")
        L.append(f"|  | selisih terhadap E_lyn utama | | | {s['selisih_vs_ref_pct'].mean():+.2f} % | |")
    (D / "dekomposisi_energi.md").write_text("\n".join(L) + "\n", encoding="utf-8")
    print("\n".join(L))
    gambar_412(d)


def gambar_412(d):
    """Gambar 4.12 (baru): energi dinamis per tanda tangan tiap fase, K1 vs K2 (batang bertumpuk, desimal koma)."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    plt.rcParams.update({"font.size": 10, "savefig.dpi": 300, "axes.spines.top": False, "axes.spines.right": False})
    out = D / "gambar_bab4"
    out.mkdir(exist_ok=True)
    fase = (("aktif", "CPU aktif", "#444444"), ("menunggu", "IC DS28E38 menghitung (CPU tidur)", "#e07b00"),
            ("tidur", "Tidur di luar episode", "#bbbbbb"))
    fig, (ax, az) = plt.subplots(1, 2, figsize=(9.0, 3.9), gridspec_kw={"width_ratios": [1.1, 1]})
    bawah = [0.0, 0.0]
    for f, nama, w in fase:
        v = [d[d.kelompok == k][f"Edyn_{f}_mJ_per_sign"].mean() for k in ("K1", "K2")]
        ax.bar(["K1\nSoftware", "K2\nDS28E38"], v, bottom=bawah, color=w, label=nama, width=0.55)
        bawah = [b + x for b, x in zip(bawah, v)]
    for i, t in enumerate(bawah):
        ax.text(i, t + 0.8, f"{t:.2f}".replace(".", ",") + " mJ", ha="center", fontsize=10, fontweight="bold")
    ax.set_ylabel("Energi dinamis per tanda tangan (mJ)")
    ax.set_ylim(0, max(bawah) * 1.15)
    ax.set_title("(a) K1 dan K2, skala sama", fontsize=10)
    # (b) zoom K2
    b2 = 0.0
    tot2 = sum(d[d.kelompok == "K2"][f"Edyn_{f}_mJ_per_sign"].mean() for f, _, _ in fase)
    for f, nama, w in fase:
        x = d[d.kelompok == "K2"][f"Edyn_{f}_mJ_per_sign"].mean()
        az.bar(["K2\nDS28E38"], [x], bottom=[b2], color=w, width=0.45, label=nama)
        az.text(0.30, b2 + x / 2, f"{x:.2f} mJ ({x / tot2 * 100:.0f} %)".replace(".", ","), va="center", fontsize=9)
        b2 += x
    az.set_xlim(-0.4, 1.0)
    az.set_ylim(0, tot2 * 1.12)
    az.set_title("(b) K2 diperbesar", fontsize=10)
    h, l = ax.get_legend_handles_labels()
    fig.legend(h, l, frameon=False, fontsize=8, loc="lower center", ncol=3, bbox_to_anchor=(0.5, -0.06))
    fig.suptitle("Dekomposisi energi dinamis per tanda tangan, skenario Normal", fontsize=10)
    fig.savefig(out / "gambar_4_12_dekomposisi_energi.png", bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    main()
