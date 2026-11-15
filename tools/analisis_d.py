"""Langkah 11: analisis statistik data tahap D (15 Nov 2026).

Membaca CSV mentah semua sesi tahap D yang terdaftar di manifest, menganalisis ulang tiap run dengan
run_bench.analyse() (kalibrasi --rshunt/--offset bisa diganti tanpa mengambil data lagi), lalu:
  1. pemeriksaan standar data (jumlah run sah per kelompok, IDLE pembuka/penutup, pergeseran IDLE,
     kecocokan model TL, pencilan, efek blok);
  2. statistik deskriptif per kelompok (rerata, sd, CV, median, IQR, IK 95 %);
  3. uji beda per skenario (K1 vs K2, K3 vs K4): Shapiro-Wilk, Levene, uji-t Welch, Mann-Whitney U,
     Hedges g, IK 95 % selisih, koreksi Holm atas tiga metrik utama (paper v3 Sub-bab 3.6.3);
  4. gain analysis (paper 3.6.2): reduksi latensi, penghematan daya dinamis, kenaikan S_cpu, + E_lyn/Q_lyn;
  5. ANOVA dua arah arsitektur x skenario (klasik dan robust HC3);
  6. gambar box plot per metrik.

Pakai:
  python tools/analisis_d.py                               manifest bawaan data/tahap_d.txt, rshunt 0.100
  python tools/analisis_d.py --rshunt 0.082 --offset -2    setelah kalibrasi shunt

Keluaran: data/analisis/<tanggal_jam>_r<rshunt>/ berisi runs.csv, pemeriksaan.md, deskriptif.csv,
uji_beda.csv, gain.csv, anova.csv, ringkasan.md, gambar/*.png.
"""
import argparse
import math
import sys
import warnings
from datetime import datetime
from pathlib import Path

import numpy as np
import pandas as pd
from scipy import stats
import statsmodels.formula.api as smf
from statsmodels.stats.anova import anova_lm
from statsmodels.stats.multitest import multipletests

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_bench as rb  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
KELOMPOK = {("K1", "A"): "K1", ("K2", "A"): "K2", ("K1", "B"): "K3", ("K2", "B"): "K4"}
INFO_KEL = {"K1": ("Software", "Normal"), "K2": ("DS28E38", "Normal"),
            "K3": ("Software", "Barrage"), "K4": ("DS28E38", "Barrage")}
N_TARGET = 30

# metrik: (kolom, label, satuan). Tiga metrik utama paper (diuji dengan koreksi Holm) ditandai UTAMA.
METRIK = [
    ("lat_mean_ms_nyata", "Latensi sign (waktu nyata)", "ms"),
    ("P_dyn_mW", "Daya dinamis", "mW"),
    ("S_cpu_log", "Durasi tidur CPU", "%"),
    ("I_dyn_mA", "Arus dinamis", "mA"),
    ("P_avg_mW", "Daya rata-rata", "mW"),
    ("I_avg_mA", "Arus rata-rata", "mA"),
    ("E_mJ", "Energi jendela", "mJ"),
    ("E_lyn_mJ", "Energi dinamis per layanan", "mJ"),
    ("Q_lyn_mC", "Muatan dinamis per layanan", "mC"),
    ("TL_pct", "Tingkat layanan", "%"),
    ("Theta_sps", "Throughput", "sign/s"),
    ("V_avg", "Tegangan bus", "V"),
]
UTAMA = ["lat_mean_ms_nyata", "P_dyn_mW", "S_cpu_log"]
UJI = ["lat_mean_ms_nyata", "P_dyn_mW", "S_cpu_log", "I_dyn_mA", "P_avg_mW", "I_avg_mA", "E_mJ",
       "E_lyn_mJ", "Q_lyn_mC"]
DETERMINISTIK = ["TL_pct", "Theta_sps"]      # varians ~0: hanya deskriptif + validasi model


# --------------------------------------------------------------------------- muat data
def baca_manifest(path):
    sesi = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            blok, nama = line.split()
            sesi.append((blok, ROOT / "data" / "runs" / nama))
    return sesi


def muat(sesi, rshunt, offset):
    rows = []
    for blok, d in sesi:
        if not d.is_dir():
            raise SystemExit(f"sesi tidak ditemukan: {d}")
        rs = []
        for i, f in enumerate(sorted(d.glob("[0-9][0-9][0-9]_*.csv")), 1):
            text = f.read_text(errors="replace")
            res, bad = rb.analyse(text, rshunt, offset)
            begin = rb.parse(text)["begin"] or []
            res.update(scen=f.stem.split("_", 2)[2], file=f.name, urut=i, masalah="; ".join(bad),
                       sesi=d.name, blok=blok, logger_build=begin[4] if len(begin) > 4 else "")
            rs.append(res)
        rb.energi_sesi(rs)
        rows += rs
    df = pd.DataFrame(rows)
    df["kelompok"] = [KELOMPOK.get((a, s), f"{a}-{s}") for a, s in zip(df["arch"], df["scen"])]
    df["sah"] = df["masalah"].fillna("") == ""
    return df


# --------------------------------------------------------------------------- statistik dasar
def deskriptif(x):
    x = np.asarray(x, dtype=float)
    x = x[~np.isnan(x)]
    n = len(x)
    if n == 0:
        return {}
    m, sd = x.mean(), x.std(ddof=1) if n > 1 else 0.0
    q1, med, q3 = np.percentile(x, [25, 50, 75])
    h = stats.t.ppf(0.975, n - 1) * sd / math.sqrt(n) if n > 1 else float("nan")
    return {"n": n, "rerata": m, "sd": sd, "CV_pct": sd / m * 100 if m else float("nan"), "median": med,
            "Q1": q1, "Q3": q3, "IQR": q3 - q1, "IK95_bawah": m - h, "IK95_atas": m + h,
            "min": x.min(), "maks": x.max()}


def hedges_g(a, b):
    n1, n2 = len(a), len(b)
    sp = math.sqrt(((n1 - 1) * a.var(ddof=1) + (n2 - 1) * b.var(ddof=1)) / (n1 + n2 - 2))
    if sp == 0:
        return float("nan")
    return (1 - 3 / (4 * (n1 + n2) - 9)) * (a.mean() - b.mean()) / sp


def uji_dua(a, b):
    """a = software, b = DS28E38 (skenario sama)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    o = {"n_sw": len(a), "n_hw": len(b), "rerata_sw": a.mean(), "rerata_hw": b.mean(),
         "selisih_sw_hw": a.mean() - b.mean()}
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        o["shapiro_p_sw"] = stats.shapiro(a).pvalue if a.std() > 0 else float("nan")
        o["shapiro_p_hw"] = stats.shapiro(b).pvalue if b.std() > 0 else float("nan")
        o["levene_p"] = stats.levene(a, b, center="median").pvalue if a.std() > 0 and b.std() > 0 else float("nan")
        t = stats.ttest_ind(a, b, equal_var=False)
        o["welch_t"], o["welch_p"] = t.statistic, t.pvalue
        va, vb = a.var(ddof=1) / len(a), b.var(ddof=1) / len(b)
        df = (va + vb) ** 2 / (va ** 2 / (len(a) - 1) + vb ** 2 / (len(b) - 1)) if va + vb > 0 else float("nan")
        h = stats.t.ppf(0.975, df) * math.sqrt(va + vb) if va + vb > 0 else float("nan")
        o["welch_df"], o["IK95_selisih_bawah"], o["IK95_selisih_atas"] = df, o["selisih_sw_hw"] - h, o["selisih_sw_hw"] + h
        u = stats.mannwhitneyu(a, b, alternative="two-sided")
        o["mwu_U"], o["mwu_p"] = u.statistic, u.pvalue
        o["r_rank_biserial"] = 1 - 2 * u.statistic / (len(a) * len(b))
    o["hedges_g"] = hedges_g(a, b)
    normal = (o["shapiro_p_sw"] > 0.05) and (o["shapiro_p_hw"] > 0.05)
    o["uji_dipakai"] = "Welch t" if normal else "Mann-Whitney U"
    o["p_dipakai"] = o["welch_p"] if normal else o["mwu_p"]
    return o


# --------------------------------------------------------------------------- pemeriksaan standar
def pemeriksaan(df, sah):
    L = ["# Pemeriksaan standar data tahap D", ""]
    lulus = True

    L += ["## 1. Run per sesi", "", "| Blok | Sesi | Run | Sah | Gugur | Build logger |", "|---|---|---|---|---|---|"]
    for (blok, s), g in df.groupby(["blok", "sesi"], sort=False):
        L.append(f"| {blok} | {s} | {len(g)} | {int(g['sah'].sum())} | {int((~g['sah']).sum())} | "
                 f"{', '.join(sorted(set(g['logger_build'])))} |")
    gug = df[~df["sah"]]
    if len(gug):
        L += ["", "Run gugur:", ""]
        for _, r in gug.iterrows():
            L.append(f"- {r['blok']} {r['file']}: {r['masalah']}")
    lr = df[df["sah"] & df.get("late_ringan", pd.Series(dtype=float)).fillna(0).gt(0)] if "late_ringan" in df else df.iloc[0:0]
    L += ["", f"Run sah dengan 'late ringan' (<= {rb.LATE_RINGAN_MAX} cuplikan terlambat, selang < "
          f"{rb.LATE_RINGAN_US} us): {len(lr)}"]

    L += ["", "## 2. Jumlah run sah per kelompok (target 30)", "", "| Kelompok | Arsitektur | Skenario | Sah | Status |",
          "|---|---|---|---|---|"]
    for k in ("K1", "K2", "K3", "K4"):
        n = int((sah["kelompok"] == k).sum())
        ok = n == N_TARGET
        lulus &= n >= N_TARGET
        L.append(f"| {k} | {INFO_KEL[k][0]} | {INFO_KEL[k][1]} | {n} | {'OK' if ok else ('LEBIH' if n > N_TARGET else 'KURANG')} |")

    L += ["", "## 3. IDLE pembuka dan penutup tiap sesi (P_idle untuk koreksi pergeseran)", "",
          "| Blok | IDLE sah | I_idle awal | I_idle akhir | Selisih | V awal | V akhir | Status |", "|---|---|---|---|---|---|---|---|"]
    for (blok, s), g in df.groupby(["blok", "sesi"], sort=False):
        idl = g[(g["scen"] == "IDLE")].sort_values("urut")
        idl_ok = idl[idl["sah"]]
        if len(idl_ok) < 1:
            lulus = False
            L.append(f"| {blok} | 0 | - | - | - | - | - | GAGAL: tidak ada IDLE sah |")
            continue
        a, z = idl.iloc[0], idl.iloc[-1]
        d = z["I_avg_mA"] - a["I_avg_mA"]
        st = "OK" if abs(d) <= 0.05 and len(idl_ok) == 2 else ("PERIKSA" if abs(d) <= 0.10 else "GAGAL")
        lulus &= st != "GAGAL"
        L.append(f"| {blok} | {len(idl_ok)} | {a['I_avg_mA']:.3f} | {z['I_avg_mA']:.3f} | {d:+.3f} | "
                 f"{a['V_avg']:.3f} | {z['V_avg']:.3f} | {st} |")
    L.append("Batas: |selisih| <= 0,05 mA = OK, <= 0,10 mA = PERIKSA, lebih = GAGAL.")

    L += ["", "## 4. Kecocokan model tingkat layanan (barrage)", "", "| Kelompok | TL terukur | TL model | Selisih | Status |",
          "|---|---|---|---|---|"]
    for k in ("K3", "K4"):
        g = sah[sah["kelompok"] == k]
        if len(g):
            t, m = g["TL_pct"].mean(), g["TL_model_pct"].mean()
            st = "OK" if abs(t - m) <= 1.0 else "PERIKSA"
            L.append(f"| {k} | {t:.3f} % | {m:.3f} % | {t - m:+.3f} | {st} |")
    for k in ("K1", "K2"):
        g = sah[sah["kelompok"] == k]
        if len(g):
            ok = bool((g["TL_pct"] == 100).all())
            lulus &= ok
            L.append(f"| {k} | {g['TL_pct'].min():.1f}-{g['TL_pct'].max():.1f} % | 100 % | - | {'OK' if ok else 'GAGAL: ada permintaan terlewat di skenario normal'} |")

    L += ["", "## 5. Konsistensi pengukuran (run sah)", ""]
    d_s = (sah["S_cpu_log"] - sah["S_cpu_host"]).abs().max()
    d_m = (sah["I_model_mA"] - sah["I_avg_mA"]).abs().max() if "I_model_mA" in sah else float("nan")
    nr = int(sah["N_rusak"].fillna(0).sum())
    L += [f"- Selisih S_cpu logger vs host terbesar: {d_s:.4f} poin (batas 0,5)",
          f"- Selisih arus model duty-cycle vs terukur terbesar: {d_m:.4f} mA",
          f"- Frame rusak/hilang di UART (total): {nr}",
          f"- Galat jam host: {sah['clk_ppm'].min():.0f} s.d. {sah['clk_ppm'].max():.0f} ppm",
          f"- Selang cuplik INA226 terbesar: {sah['dt_max_us'].max():.0f} us"]
    lulus &= d_s <= 0.5 and nr == 0

    L += ["", "## 6. Pencilan (pagar Tukey 3 x IQR, hanya dilaporkan, tidak dibuang)", ""]
    ada = False
    for col, lab, sat in METRIK:
        if col in DETERMINISTIK or col not in sah:
            continue
        for k, g in sah.groupby("kelompok"):
            x = g[col].dropna()
            if len(x) < 4:
                continue
            q1, q3 = np.percentile(x, [25, 75])
            iqr = q3 - q1
            out = g[(g[col] < q1 - 3 * iqr) | (g[col] > q3 + 3 * iqr)]
            for _, r in out.iterrows():
                ada = True
                L.append(f"- {k} {lab}: {r[col]:.4f} {sat} ({r['blok']} {r['file']}; median {np.median(x):.4f})")
    if not ada:
        L.append("- tidak ada")

    L += ["", "## 7. Efek blok (kelompok yang diambil di lebih dari satu blok)", "",
          "| Kelompok | Metrik | " + " | ".join(["Blok (n): rerata"]) + " | Rentang antar blok / sd dalam blok |", "|---|---|---|---|"]
    for k, g in sah.groupby("kelompok"):
        if g["blok"].nunique() < 2:
            continue
        for col in ("I_avg_mA", "I_dyn_mA", "P_dyn_mW", "E_lyn_mJ", "lat_mean_ms_nyata", "V_avg"):
            gb = g.groupby("blok", sort=False)[col]
            means = gb.mean()
            sd_in = math.sqrt(gb.var(ddof=1).fillna(0).mean())
            rng = means.max() - means.min()
            L.append(f"| {k} | {col} | " + "; ".join(f"{b} ({int(gb.count()[b])}): {m:.4f}" for b, m in means.items())
                     + f" | {rng:.4f} / {sd_in:.4f} = {rng / sd_in if sd_in else float('nan'):.1f} |")
    L.append("Rasio > ~3 berarti pergeseran antar blok lebih besar dari sebaran dalam blok; untuk metrik absolut "
             "(I_avg, P_avg) ini wajar (tegangan USB), metrik terkoreksi IDLE (I_dyn, P_dyn, E_lyn) seharusnya kecil.")

    L += ["", f"## Kesimpulan pemeriksaan: {'MEMENUHI STANDAR' if lulus else 'BELUM MEMENUHI STANDAR'}", ""]
    return lulus, "\n".join(L)


# --------------------------------------------------------------------------- utama
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--manifest", default=str(ROOT / "data" / "tahap_d.txt"))
    ap.add_argument("--rshunt", type=float, default=0.100)
    ap.add_argument("--offset", type=float, default=-2.0)
    ap.add_argument("--keluar", default=None)
    ap.add_argument("--tanpa-gambar", action="store_true")
    args = ap.parse_args()

    sesi = baca_manifest(Path(args.manifest))
    df = muat(sesi, args.rshunt, args.offset)
    out = Path(args.keluar) if args.keluar else ROOT / "data" / "analisis" / f"{datetime.now():%Y%m%d_%H%M%S}_r{args.rshunt:g}"
    out.mkdir(parents=True, exist_ok=True)
    df.to_csv(out / "runs.csv", index=False)

    sah = df[df["sah"] & df["kelompok"].isin(["K1", "K2", "K3", "K4"])].copy()
    lulus, teks = pemeriksaan(df, sah)
    (out / "pemeriksaan.md").write_text(teks, encoding="utf-8")
    print(teks)

    # --- deskriptif ---
    des = []
    for k in ("K1", "K2", "K3", "K4"):
        g = sah[sah["kelompok"] == k]
        for col, lab, sat in METRIK:
            if col in g:
                d = deskriptif(g[col])
                if d:
                    des.append({"kelompok": k, "arsitektur": INFO_KEL[k][0], "skenario": INFO_KEL[k][1],
                                "metrik": col, "label": lab, "satuan": sat, **d})
    for a in ("K1", "K2"):
        g = df[df["sah"] & (df["scen"] == "IDLE") & (df["arch"] == a)]
        for col in ("I_avg_mA", "P_avg_mW", "V_avg"):
            d = deskriptif(g[col])
            if d:
                des.append({"kelompok": f"{a}-IDLE", "arsitektur": "Software" if a == "K1" else "DS28E38",
                            "skenario": "IDLE", "metrik": col, "label": col, "satuan": "", **d})
    des = pd.DataFrame(des)
    des.to_csv(out / "deskriptif.csv", index=False)

    # --- uji beda per skenario + Holm atas 3 metrik utama ---
    uji = []
    for sk, (ksw, khw) in (("Normal", ("K1", "K2")), ("Barrage", ("K3", "K4"))):
        a_ = sah[sah["kelompok"] == ksw]
        b_ = sah[sah["kelompok"] == khw]
        for col in UJI:
            if col in sah and a_[col].notna().sum() > 2 and b_[col].notna().sum() > 2:
                uji.append({"skenario": sk, "pembanding": f"{ksw} vs {khw}", "metrik": col,
                            **uji_dua(a_[col].dropna(), b_[col].dropna())})
    uji = pd.DataFrame(uji)
    uji["utama"] = uji["metrik"].isin(UTAMA)
    uji["p_holm"] = np.nan
    for sk in uji["skenario"].unique():
        m = (uji["skenario"] == sk) & uji["utama"]
        uji.loc[m, "p_holm"] = multipletests(uji.loc[m, "p_dipakai"], method="holm")[1]
    uji["keputusan"] = np.where(uji["utama"], np.where(uji["p_holm"] < 0.05, "tolak H0", "terima H0"),
                                np.where(uji["p_dipakai"] < 0.05, "berbeda (tanpa koreksi)", "tidak berbeda"))
    uji.to_csv(out / "uji_beda.csv", index=False)

    # --- gain analysis ---
    def mean(k, c):
        return sah.loc[sah["kelompok"] == k, c].mean()
    gain = []
    for sk, (ksw, khw) in (("Normal", ("K1", "K2")), ("Barrage", ("K3", "K4"))):
        gain.append({"skenario": sk,
                     "G_latensi_pct": (mean(ksw, "lat_mean_ms_nyata") - mean(khw, "lat_mean_ms_nyata")) / mean(ksw, "lat_mean_ms_nyata") * 100,
                     "G_Pdyn_pct": (mean(ksw, "P_dyn_mW") - mean(khw, "P_dyn_mW")) / mean(ksw, "P_dyn_mW") * 100,
                     "G_Idyn_pct": (mean(ksw, "I_dyn_mA") - mean(khw, "I_dyn_mA")) / mean(ksw, "I_dyn_mA") * 100,
                     "G_Pavg_pct": (mean(ksw, "P_avg_mW") - mean(khw, "P_avg_mW")) / mean(ksw, "P_avg_mW") * 100,
                     "G_Elyn_pct": (mean(ksw, "E_lyn_mJ") - mean(khw, "E_lyn_mJ")) / mean(ksw, "E_lyn_mJ") * 100,
                     "G_Qlyn_pct": (mean(ksw, "Q_lyn_mC") - mean(khw, "Q_lyn_mC")) / mean(ksw, "Q_lyn_mC") * 100,
                     "dS_cpu_poin": mean(khw, "S_cpu_log") - mean(ksw, "S_cpu_log"),
                     "dTL_poin": mean(khw, "TL_pct") - mean(ksw, "TL_pct"),
                     "rasio_Theta": mean(khw, "Theta_sps") / mean(ksw, "Theta_sps")})
    gain = pd.DataFrame(gain)
    gain.to_csv(out / "gain.csv", index=False)

    # --- ANOVA dua arah (klasik + robust HC3) ---
    sah["arsitektur"] = sah["kelompok"].map(lambda k: INFO_KEL[k][0])
    sah["skenario"] = sah["kelompok"].map(lambda k: INFO_KEL[k][1])
    an = []
    for col in UJI:
        d = sah[[col, "arsitektur", "skenario"]].dropna()
        if d[col].std() == 0:
            continue
        mdl = smf.ols(f"{col} ~ C(arsitektur) * C(skenario)", data=d).fit()
        for robust in (None, "hc3"):
            with warnings.catch_warnings():
                warnings.simplefilter("ignore")
                t = anova_lm(mdl, typ=2, robust=robust)
            ss_tot = t["sum_sq"].sum()
            for src, r in t.iterrows():
                if src == "Residual":
                    continue
                an.append({"metrik": col, "galat": robust or "klasik", "sumber": src.replace("C(", "").replace(")", ""),
                           "F": r["F"], "p": r["PR(>F)"],
                           "eta2_parsial": r["sum_sq"] / (r["sum_sq"] + t.loc["Residual", "sum_sq"]) if robust is None else np.nan,
                           "porsi_SS": r["sum_sq"] / ss_tot if robust is None else np.nan})
    an = pd.DataFrame(an)
    an.to_csv(out / "anova.csv", index=False)

    # --- gambar ---
    if not args.tanpa_gambar:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        gdir = out / "gambar"
        gdir.mkdir(exist_ok=True)
        for col, lab, sat in METRIK:
            if col not in sah or col in ("V_avg",):
                continue
            data = [sah.loc[sah["kelompok"] == k, col].dropna().values for k in ("K1", "K2", "K3", "K4")]
            fig, ax = plt.subplots(figsize=(6, 4))
            ax.boxplot(data, showfliers=True)
            ax.set_xticks([1, 2, 3, 4], ["K1\nSW normal", "K2\nHW normal", "K3\nSW barrage", "K4\nHW barrage"])
            ax.set_ylabel(f"{lab} ({sat})")
            ax.set_title(f"{lab} (n = {', '.join(str(len(x)) for x in data)})")
            ax.grid(axis="y", alpha=0.3)
            fig.tight_layout()
            fig.savefig(gdir / f"{col}.png", dpi=150)
            plt.close(fig)

    # --- ringkasan ---
    R = [f"# Ringkasan analisis tahap D", "", f"Dibuat {datetime.now():%Y-%m-%d %H:%M}; rshunt = {args.rshunt} Ohm, "
         f"offset = {args.offset} LSB; manifest {Path(args.manifest).name}; pemeriksaan: "
         f"{'MEMENUHI STANDAR' if lulus else 'BELUM MEMENUHI STANDAR'}.", "",
         "## Rerata (sd) per kelompok", "", "| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |", "|---|---|---|---|---|"]
    for col, lab, sat in METRIK:
        cells = []
        for k in ("K1", "K2", "K3", "K4"):
            r = des[(des["kelompok"] == k) & (des["metrik"] == col)]
            cells.append(f"{r['rerata'].iloc[0]:.4g} ({r['sd'].iloc[0]:.2g})" if len(r) else "-")
        R.append(f"| {lab} ({sat}) | " + " | ".join(cells) + " |")
    R += ["", "## Uji beda (metrik utama, koreksi Holm per skenario)", "",
          "| Skenario | Metrik | Uji | p | p Holm | Hedges g | Keputusan |", "|---|---|---|---|---|---|---|"]
    for _, r in uji[uji["utama"]].iterrows():
        R.append(f"| {r['skenario']} | {r['metrik']} | {r['uji_dipakai']} | {r['p_dipakai']:.3g} | {r['p_holm']:.3g} | "
                 f"{r['hedges_g']:.1f} | {r['keputusan']} |")
    R += ["", "## Gain (software -> DS28E38)", "", "| " + " | ".join(gain.columns) + " |",
          "|" + "---|" * len(gain.columns)]
    for _, r in gain.iterrows():
        R.append("| " + " | ".join(str(v) if isinstance(v, str) else f"{v:.3f}" for v in r.values) + " |")
    R += ["", "## ANOVA dua arah (klasik): porsi variasi dan interaksi", ""]
    for col in UTAMA + ["I_dyn_mA", "E_lyn_mJ"]:
        t = an[(an["metrik"] == col) & (an["galat"] == "klasik")]
        if len(t):
            R.append(f"- {col}: " + "; ".join(f"{r['sumber']} F = {r['F']:.4g}, p = {r['p']:.2g}, eta2p = {r['eta2_parsial']:.3f}"
                                             for _, r in t.iterrows()))
    R += ["", "Catatan: TL dan throughput praktis deterministik (sd ~ 0), dilaporkan deskriptif + validasi model, "
          "tidak diuji. Sebaran sangat kecil membuat p-value amat kecil; tekankan ukuran efek dan persen gain."]
    (out / "ringkasan.md").write_text("\n".join(R), encoding="utf-8")
    print("\n".join(R))
    print(f"\nkeluaran: {out}")


if __name__ == "__main__":
    main()
