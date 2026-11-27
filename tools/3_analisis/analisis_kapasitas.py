"""Analisis skenario C (kapasitas, rho = 1; 27 Nov 2026) terhadap skenario A dan B data Stop (stop_final).

Kelompok (kerangka K1-K4 analisis_d, varian Stop):
  K1 = SW normal (A)   K2 = HW normal (A)   K3 = SW barrage (B)   K4 = HW barrage (B)
  K5 = SW kapasitas (C)                      K6 = HW kapasitas (C)
Sesi C tidak punya IDLE (keputusan peneliti): P_idle/I_idle = rerata IDLE sah stop_final per arsitektur, dihitung
ulang di sini dengan rshunt/offset yang sama. Karena itu metrik UTAMA = besaran mutlak (tidak butuh IDLE);
P_dyn/E_lyn/I_dyn/Q_lyn = sekunder.

Uji (sama dengan Bab 3): Welch (utama) + Mann-Whitney (cek) + Holm atas keluarga metrik utama x 3 pasangan,
IK 95 % bootstrap persentil 10.000 untuk selisih rerata.
  K5 vs K6 : pengaruh arsitektur tepat di kapasitas
  K3 vs K5 : SW, barrage (rho 6,6) dibanding kapasitas -> biaya frame kelebihan
  K4 vs K6 : HW, barrage (rho 1,2) dibanding kapasitas

Pakai (venv analisis):
  .venv-analisis/Scripts/python tools/3_analisis/analisis_kapasitas.py
      [--manifest-c data/tahap_kapasitas.txt] [--manifest-stop data/tahap_stop.txt] [--keluar data/analisis/kapasitas]
Desain: docs/superpowers/specs/2026-11-27-skenario-c-kapasitas-design.md
"""
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402
import argparse
import json
import warnings
from datetime import datetime
from pathlib import Path

import numpy as np
import pandas as pd
from scipy import stats
from statsmodels.stats.multitest import multipletests

import run_bench as rb  # noqa: E402
import analisis_d as ad  # noqa: E402
import kalibrasi_param as kp  # noqa: E402

KEL_C = {"K1": "K5", "K2": "K6"}
INFO = {"K1": ("Software", "A normal 1/s"), "K2": ("DS28E38", "A normal 1/s"),
        "K3": ("Software", "B barrage 10/s"), "K4": ("DS28E38", "B barrage 10/s"),
        "K5": ("Software", "C kapasitas rho=1"), "K6": ("DS28E38", "C kapasitas rho=1")}
UTAMA = ["lat_mean_ms_nyata", "TL_pct", "S_cpu_log", "I_avg_mA", "P_avg_mW", "E_per_sign_mJ"]
SEKUNDER = ["P_dyn_mW", "E_lyn_mJ", "I_dyn_mA", "Q_lyn_mC", "rho_nyata", "lambda_eff_rps", "Theta_sps",
            "R_lwt_pct", "I_tidur_mA", "V_avg"]
PASANGAN = [("K5", "K6"), ("K3", "K5"), ("K4", "K6")]


def idle_dari_df(df):
    """{arch 'K1S'/'K2S': (P_idle_mW, I_idle_mA)} dari run IDLE sah sebuah DataFrame hasil analisis_d.muat."""
    m = (df["scen"] == "IDLE") & df["sah"]
    out = {}
    for a, g in df[m].groupby("arch"):
        out[a if a.endswith("S") else a + "S"] = (float(g["P_avg_mW"].mean()), float(g["I_avg_mA"].mean()))
    return out


def muat_c(sesi, rshunt, offset, periode, idle):
    """Sesi skenario C -> DataFrame dengan kolom kelompok K5/K6 (arsitektur dinormalkan ke K1/K2)."""
    rows = []
    for blok, d in sesi:
        if not d.is_dir():
            raise SystemExit(f"sesi tidak ditemukan: {d}")
        rs = []
        for i, f in enumerate(sorted(d.glob("[0-9][0-9][0-9]_*.csv")), 1):
            scen = f.stem.split("_", 2)[2]
            res, bad = rb.analyse(f.read_text(errors="replace"), rshunt, offset, scen)   # toleransi hilang per skenario
            res.update(scen=scen, file=f.name, urut=i, masalah="; ".join(bad), sesi=d.name,
                       blok=blok)
            rb.tambah_kapasitas(res, periode)
            rs.append(res)
        rb.energi_sesi(rs, idle)
        rows += rs
    df = pd.DataFrame(rows)
    df["arch"] = df["arch"].str.replace(r"S$", "", regex=True)
    df["kelompok"] = [KEL_C.get(a, f"{a}-{s}") if s == "C" else f"{a}-{s}" for a, s in zip(df["arch"], df["scen"])]
    df["sah"] = df["masalah"].fillna("") == ""
    return df


def banding(sah, pasangan, utama, sekunder, rng, n_boot=ad.N_BOOT):
    """Satu baris per (pasangan, metrik): rerata, selisih (kelompok 2 - kelompok 1), IK95 bootstrap, Welch, MWU.
    Holm diterapkan atas semua baris metrik UTAMA (seluruh pasangan sekaligus); sekunder tanpa Holm (p_holm NaN)."""
    out = []
    for k1, k2 in pasangan:
        for col in utama + sekunder:
            if col not in sah:
                continue
            a = sah.loc[sah["kelompok"] == k1, col].dropna().to_numpy(float)
            b = sah.loc[sah["kelompok"] == k2, col].dropna().to_numpy(float)
            if len(a) < 2 or len(b) < 2:
                continue
            ba = a[rng.integers(0, len(a), size=(n_boot, len(a)))].mean(axis=1)
            bb = b[rng.integers(0, len(b), size=(n_boot, len(b)))].mean(axis=1)
            r = {"pembanding": f"{k1} vs {k2}", "metrik": col, "utama": col in utama, "n_1": len(a), "n_2": len(b),
                 "rerata_1": a.mean(), "rerata_2": b.mean(), "selisih_2_min_1": b.mean() - a.mean(),
                 "selisih_IK95_bawah": float(np.percentile(bb - ba, 2.5)),
                 "selisih_IK95_atas": float(np.percentile(bb - ba, 97.5)),
                 "rasio_2_per_1": b.mean() / a.mean() if a.mean() else np.nan}
            if a.std() > 0 or b.std() > 0:
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore")
                    r["p_welch"] = stats.ttest_ind(a, b, equal_var=False).pvalue
                    r["p_mwu_cek"] = stats.mannwhitneyu(a, b, alternative="two-sided").pvalue
            out.append(r)
    kolom = ["pembanding", "metrik", "utama", "n_1", "n_2", "rerata_1", "rerata_2", "selisih_2_min_1",
             "selisih_IK95_bawah", "selisih_IK95_atas", "rasio_2_per_1", "p_welch", "p_mwu_cek"]
    t = pd.DataFrame(out, columns=kolom)                 # kolom tetap walau kosong (kelompok belum ada datanya)
    t["p_holm"] = np.nan
    m = t["utama"].astype(bool) & t["p_welch"].notna()
    if m.any():
        t.loc[m, "p_holm"] = multipletests(t.loc[m, "p_welch"], method="holm")[1]
    t["keputusan"] = np.where(t["p_holm"].notna(), np.where(t["p_holm"] < 0.05, "tolak H0", "gagal menolak H0"), "")
    return t


def deskriptif_kel(sah, kolom):
    out = []
    for k in sorted(INFO):
        g = sah[sah["kelompok"] == k]
        for col in kolom:
            if col in g:
                d = ad.deskriptif(g[col].dropna())
                if d:
                    out.append({"kelompok": k, "arsitektur": INFO[k][0], "skenario": INFO[k][1], "metrik": col, **d})
    return pd.DataFrame(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--manifest-c", default=str(ROOT / "data" / "tahap_kapasitas.txt"))
    ap.add_argument("--manifest-stop", default=str(ROOT / "data" / "tahap_stop.txt"))
    ap.add_argument("--periode", default=str(rb.PERIODE_JSON))
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF)
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB)
    ap.add_argument("--keluar", default=str(ROOT / "data" / "analisis" / "kapasitas"))
    args = ap.parse_args()
    out = Path(args.keluar)
    out.mkdir(parents=True, exist_ok=True)
    periode = json.loads(Path(args.periode).read_text(encoding="utf-8"))

    st, _ = ad.muat(ad.baca_manifest(Path(args.manifest_stop)), args.rshunt, args.offset, stop=True)
    idle = idle_dari_df(st)
    c = muat_c(ad.baca_manifest(Path(args.manifest_c)), args.rshunt, args.offset, periode, idle)
    df = pd.concat([st.assign(sumber="stop_final"), c.assign(sumber="kapasitas")], ignore_index=True)
    sah = df[df["sah"]]
    df.to_csv(out / "runs.csv", index=False)

    kol = UTAMA + SEKUNDER
    des = deskriptif_kel(sah, kol)
    des.to_csv(out / "deskriptif.csv", index=False)
    t = banding(sah, PASANGAN, UTAMA, SEKUNDER, np.random.default_rng(ad.SEED))
    t.to_csv(out / "banding.csv", index=False)

    n = sah.groupby("kelompok").size()
    gugur = c[~c["sah"]]
    lines = [f"# Ringkasan skenario C (kapasitas, rho = 1)", "",
             f"Dibuat {datetime.now():%Y-%m-%d %H:%M}; rshunt {args.rshunt}, offset {args.offset}.",
             f"P_idle sesi C dari IDLE stop_final: " + ", ".join(f"{a} {p:.3f} mW / {i:.4f} mA" for a, (p, i) in idle.items()),
             "", "## Periode attacker", "", "| arsitektur | t_sign (ms) | n sign | lambda_B (/s) | k | dikirim (us) | nyata (us) | sesi |",
             "|---|---|---|---|---|---|---|---|"]
    for a, p in sorted(periode.items()):
        lines.append(f"| {a} | {p['t_sign_ms']:.3f} | {p['n_sign']} | {p['lambda_B']:.4f} | {p['k']:.5f} | "
                     f"{p['periode_kirim_us']} | {p['periode_nyata_us']} | {p.get('sesi', '')} |")
    lines += ["", "## Jumlah run sah", "", "| kelompok | n |", "|---|---|"]
    lines += [f"| {k} ({INFO.get(k, ('', ''))[0]}, {INFO.get(k, ('', ''))[1]}) | {v} |" for k, v in n.items()]
    lines += ["", f"Run C gugur: {len(gugur)}" + ("".join(f"\n- {r.sesi}/{r.file}: {r.masalah}" for r in gugur.itertuples()))]
    lines += ["", "## Rerata per kelompok (run sah)", "", "| metrik | " + " | ".join(sorted(INFO)) + " |",
              "|---|" + "---|" * len(INFO)]
    for col in kol:
        v = des[des["metrik"] == col].set_index("kelompok")["rerata"]
        lines.append(f"| {col} | " + " | ".join(f"{v[k]:.4g}" if k in v else "-" for k in sorted(INFO)) + " |")
    lines += ["", "## Perbandingan (selisih = kelompok 2 - kelompok 1; Holm atas metrik utama)", "",
              "| pembanding | metrik | rerata 1 | rerata 2 | selisih [IK95] | p Welch | p Holm | keputusan |",
              "|---|---|---|---|---|---|---|---|"]
    for r in t.itertuples():
        lines.append(f"| {r.pembanding} | {r.metrik}{'' if r.utama else ' (sek.)'} | {r.rerata_1:.4g} | {r.rerata_2:.4g} | "
                     f"{r.selisih_2_min_1:.4g} [{r.selisih_IK95_bawah:.4g}; {r.selisih_IK95_atas:.4g}] | "
                     f"{getattr(r, 'p_welch', float('nan')):.3g} | {r.p_holm:.3g} | {r.keputusan} |")
    (out / "ringkasan.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"-> {out}  (run sah per kelompok: {dict(n)})")


if __name__ == "__main__":
    main()
