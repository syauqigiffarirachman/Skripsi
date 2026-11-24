"""Analisis statistik data tahap D sesuai Bab 3 paper (revisi v5 atas SEMPRO v4, 16 Nov 2026).

Membaca CSV mentah semua sesi tahap D yang terdaftar di manifest, menganalisis ulang tiap run dengan
run_bench.analyse() (kalibrasi --rshunt/--offset bisa diganti tanpa mengambil data lagi), lalu:
  0. pemeriksaan keutuhan + tabel run gugur per sebab (termasuk sesi gugur di data/dev/gugur/);
  1. deskriptif per kelompok: rerata, sd, CV, median, min, maks, IK 95 %;
  2. ketidakpastian: acak (IK bootstrap) dan sistematis (kalibrasi shunt, antar-blok, drift IDLE);
  3. RM1/RM3 - SW vs HW per skenario: selisih + IK 95 % bootstrap, gain %, rasio HW/SW; uji Welch t
     (utama) dengan koreksi Holm atas 6 perbandingan (H1-H3 x 2 skenario), Mann-Whitney U sebagai cek;
  4. RM2 - pengaruh barrage per arsitektur (K1->K3, K2->K4): S_cpu, P_dyn, I_dyn, E_lyn, TL + IK;
  5. RM3 - interaksi: selisih-dari-selisih (Delta_barrage - Delta_normal) + IK bootstrap;
  6. H4 - tingkat layanan: nilai per kelompok + rentang + model utilisasi (tidak diuji);
  7. kesesuaian instrumen (Bland-Altman): latensi DWT vs episode pin (skenario normal), S_cpu firmware
     vs pin, arus INA226 vs multimeter (--multimeter); pemeriksaan model duty cycle per run;
  8. drift: IDLE awal-akhir blok, metrik vs urutan run, leave-one-block-out;
  9. gambar: box plot, interaction plot, drift, Bland-Altman.

Pakai:
  python tools/3_analisis/analisis_d.py                               manifest bawaan data/tahap_d.txt; kalibrasi bawaan = hasil 21 Nov 2026
                                                           (rshunt 0.0821, offset -2.83, u_rel 0.015; tools/2_kalibrasi/kalibrasi_param.py)
  python tools/3_analisis/analisis_d.py --manifest data/tahap_stop.txt --stop --keluar data/analisis/stop_final     data skripsi (Stop)
  python tools/3_analisis/analisis_d.py --rshunt 0.100 --offset -2.0 --u-rel 0.05   mereproduksi hasil sebelum kalibrasi (nominal)
  python tools/3_analisis/analisis_d.py --multimeter data/kalibrasi/multimeter.csv   (kolom: file,I_mm_mA)

Keluaran: data/analisis/<tanggal_jam>_r<rshunt>/.
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import math
import re
import sys
import warnings
from datetime import datetime
from pathlib import Path

import numpy as np
import pandas as pd
from scipy import stats
from statsmodels.stats.multitest import multipletests

import run_bench as rb  # noqa: E402
import kalibrasi_param as kp  # noqa: E402   # default --rshunt/--offset/--u-rel = kalibrasi 21 Nov 2026

STOP_MODE = False
KELOMPOK = {("K1", "A"): "K1", ("K2", "A"): "K2", ("K1", "B"): "K3", ("K2", "B"): "K4"}
INFO_KEL = {"K1": ("Software", "Normal"), "K2": ("DS28E38", "Normal"),
            "K3": ("Software", "Barrage"), "K4": ("DS28E38", "Barrage")}
KK = ("K1", "K2", "K3", "K4")
SKEN = (("Normal", ("K1", "K2")), ("Barrage", ("K3", "K4")))
ARSI = (("Software", ("K1", "K3")), ("DS28E38", ("K2", "K4")))
N_TARGET = 30
N_BOOT = 10_000
SEED = 20261116
EPISODE_GAP_S = 0.200      # jeda rendah < 200 ms = masih satu tanda tangan (K2: tunggu IC ~130,6 ms)
GLITCH_S = 0.001           # pulsa tinggi < 1 ms = bangun singkat bukan tanda tangan (dibuang)

METRIK = [
    ("lat_mean_ms_nyata", "Latensi sign (waktu nyata)", "ms"),
    ("P_dyn_mW", "Daya dinamis", "mW"),
    ("S_cpu_log", "Durasi tidur CPU", "%"),
    ("E_lyn_mJ", "Energi per permintaan terlayani", "mJ"),
    ("I_dyn_mA", "Arus dinamis", "mA"),
    ("Q_lyn_mC", "Muatan dinamis per layanan", "mC"),
    ("P_avg_mW", "Daya rata-rata", "mW"),
    ("I_avg_mA", "Arus rata-rata", "mA"),
    ("E_mJ", "Energi jendela", "mJ"),
    ("TL_pct", "Tingkat layanan", "%"),
    ("Theta_sps", "Throughput", "sign/s"),
    ("V_avg", "Tegangan bus", "V"),
]
LABEL = {c: (l, s) for c, l, s in METRIK}
UTAMA = ["lat_mean_ms_nyata", "P_dyn_mW", "S_cpu_log"]          # H1-H3, keluarga Holm (6 perbandingan)
PELENGKAP = ["E_lyn_mJ", "I_dyn_mA", "Q_lyn_mC", "P_avg_mW"]
ENERGI = ["P_dyn_mW", "E_lyn_mJ", "I_dyn_mA", "Q_lyn_mC", "P_avg_mW", "I_avg_mA"]   # kena kalibrasi shunt
# arah "HW lebih baik": +1 bila nilai HW lebih besar lebih baik, -1 bila lebih kecil lebih baik
ARAH = {"lat_mean_ms_nyata": -1, "P_dyn_mW": -1, "S_cpu_log": +1, "E_lyn_mJ": -1, "I_dyn_mA": -1,
        "Q_lyn_mC": -1, "P_avg_mW": -1, "I_avg_mA": -1, "TL_pct": +1, "Theta_sps": +1}


# --------------------------------------------------------------------------- muat data
def baca_manifest(path):
    sesi = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            blok, nama = line.split()
            sesi.append((blok, ROOT / "data" / "runs" / nama))
    return sesi


def episode_pin(r, res):
    """Durasi episode tanda tangan dari tepi pin status (detik kristal logger) dan latensi DWT nyata.

    Episode = gabungan pulsa tinggi yang dipisah jeda rendah < EPISODE_GAP_S. Dipakai hanya bila jumlah
    episode = jumlah latensi DWT (dipasangkan berurutan). Mengembalikan (pin_s[], dwt_s[]) atau None.
    """
    if not r["lat"] or r["end"] is None or r["t_start"] is None:
        return None
    T = rb.TICK_HZ
    slack = T // 500
    ed = [(t, lv) for t, lv in r["e"] if r["t_start"] - slack <= t <= r["t_endline"]]
    pulsa, naik = [], None
    for t, lv in ed:
        if lv == 1 and naik is None:
            naik = t
        elif lv == 0 and naik is not None:
            if (t - naik) / T >= GLITCH_S:
                pulsa.append([naik, t])
            naik = None
    if not pulsa:
        return None
    ep = [pulsa[0][:]]
    for a, b in pulsa[1:]:
        if (a - ep[-1][1]) / T < EPISODE_GAP_S:
            ep[-1][1] = b
        else:
            ep.append([a, b])
    if len(ep) != len(r["lat"]):
        return None
    k = res["T_obs_log_s"] / res["T_obs_host_s"] / r["end"]["clk_hz"]
    return [(b - a) / T for a, b in ep], [c * k for c in r["lat"]]


def muat(sesi, rshunt, offset, stop=False):
    rows, pins = [], []
    for blok, d in sesi:
        if not d.is_dir():
            raise SystemExit(f"sesi tidak ditemukan: {d}")
        rs = []
        for i, f in enumerate(sorted(d.glob("[0-9][0-9][0-9]_*.csv")), 1):
            text = f.read_text(errors="replace")
            res, bad = rb.analyse(text, rshunt, offset)
            r = rb.parse(text)
            begin = r["begin"] or []
            res.update(scen=f.stem.split("_", 2)[2], file=f.name, urut=i, masalah="; ".join(bad),
                       sesi=d.name, blok=blok, logger_build=begin[4] if len(begin) > 4 else "")
            if not bad and res["scen"] == "A":
                ep = episode_pin(r, res)
                res["pin_episode_cocok"] = ep is not None
                if ep:
                    res["lat_pin_ms"] = float(np.mean(ep[0])) * 1e3
                    for j, (p, w) in enumerate(zip(*ep)):
                        pins.append({"sesi": d.name, "file": f.name, "arch": res["arch"], "i": j,
                                     "pin_ms": p * 1e3, "dwt_ms": w * 1e3})
            rs.append(res)
        rb.energi_sesi(rs)
        rows += rs
    df = pd.DataFrame(rows)
    if stop:                                            # varian Stop (K1S/K2S) dianalisis dengan kerangka K1/K2 yang sama
        df["arch"] = df["arch"].str.replace(r"S$", "", regex=True)
    df["kelompok"] = [KELOMPOK.get((a, s), f"{a}-{s}") for a, s in zip(df["arch"], df["scen"])]
    df["sah"] = df["masalah"].fillna("") == ""
    df["urut_global"] = np.arange(len(df)) + 1          # urutan kronologis (sesi menurut manifest)
    return df, pd.DataFrame(pins)


def muat_gugur():
    """Sesi gugur utuh (data/dev/gugur/): hanya untuk tabel jumlah run gugur dan sebabnya."""
    out = []
    base = ROOT / "data" / "dev" / "gugur"
    for d in sorted(base.glob("stop_*" if STOP_MODE else "*")) if base.is_dir() else []:   # varian Stop: hanya sesi gugurnya sendiri
        cat = ""
        st = d / "sesi.txt"
        if st.exists():
            for line in st.read_text(errors="replace").splitlines():
                if line.startswith("catatan="):
                    cat = line[8:]
        out.append({"sesi": d.name, "run": len(list(d.glob("[0-9][0-9][0-9]_*.csv"))), "catatan": cat})
    return out


# --------------------------------------------------------------------------- statistik dasar
def deskriptif(x):
    x = np.asarray(x, dtype=float)
    x = x[~np.isnan(x)]
    n = len(x)
    if n == 0:
        return {}
    m, sd = x.mean(), x.std(ddof=1) if n > 1 else 0.0
    h = stats.t.ppf(0.975, n - 1) * sd / math.sqrt(n) if n > 1 else float("nan")
    return {"n": n, "rerata": m, "sd": sd, "CV_pct": sd / m * 100 if m else float("nan"),
            "median": np.median(x), "IK95_bawah": m - h, "IK95_atas": m + h, "min": x.min(), "maks": x.max()}


def kel(sah, k, col):
    return sah.loc[sah["kelompok"] == k, col].dropna().to_numpy(float)


def boot_mean(rng, x):
    x = np.asarray(x, float)
    return x[rng.integers(0, len(x), size=(N_BOOT, len(x)))].mean(axis=1)


def ik(v):
    return float(np.percentile(v, 2.5)), float(np.percentile(v, 97.5))


def delta_hw_baik(sw, hw, col):
    """Selisih antar-arsitektur (positif = HW lebih baik), dalam satuan metrik."""
    return (hw - sw) if ARAH[col] > 0 else (sw - hw)


def bland_altman(x, y):
    """x = metode acuan, y = metode pembanding. Selisih = y - x."""
    x, y = np.asarray(x, float), np.asarray(y, float)
    d = y - x
    m, s = d.mean(), d.std(ddof=1)
    return {"n": len(d), "bias": m, "sd_selisih": s, "LoA_bawah": m - 1.96 * s, "LoA_atas": m + 1.96 * s,
            "bias_rel_pct": m / x.mean() * 100 if x.mean() else float("nan")}


# --------------------------------------------------------------------------- pemeriksaan keutuhan
def pemeriksaan(df, sah, gugur):
    L = [f"# Pemeriksaan keutuhan data {'varian Stop (deep sleep)' if STOP_MODE else 'tahap D'}", ""]
    lulus = True
    L += ["## 1. Run per sesi", "", "| Blok | Sesi | Run | Sah | Gugur | Build logger |", "|---|---|---|---|---|---|"]
    for (blok, s), g in df.groupby(["blok", "sesi"], sort=False):
        L.append(f"| {blok} | {s} | {len(g)} | {int(g['sah'].sum())} | {int((~g['sah']).sum())} | "
                 f"{', '.join(sorted(set(g['logger_build'])))} |")
    gug = df[~df["sah"]]
    lr = df[df["sah"] & df["late_ringan"].fillna(0).gt(0)] if "late_ringan" in df else df.iloc[0:0]
    L += ["", f"Run sah dengan 'late ringan' (<= {rb.LATE_RINGAN_MAX} cuplikan terlambat, selang < "
          f"{rb.LATE_RINGAN_US} us): {len(lr)}"]

    L += ["", "## 2. Jumlah run sah per kelompok (target 30)", "", "| Kelompok | Arsitektur | Skenario | Sah |",
          "|---|---|---|---|"]
    for k in KK:
        n = int((sah["kelompok"] == k).sum())
        lulus &= n >= N_TARGET
        L.append(f"| {k} | {INFO_KEL[k][0]} | {INFO_KEL[k][1]} | {n} |")

    L += ["", "## 3. IDLE pembuka dan penutup tiap blok (drift baseline)", "",
          "| Blok | I_idle awal | I_idle akhir | Selisih (mA) | V awal | V akhir | Status |", "|---|---|---|---|---|---|---|"]
    for (blok, s), g in df.groupby(["blok", "sesi"], sort=False):
        idl = g[(g["scen"] == "IDLE")].sort_values("urut")
        if (idl["sah"]).sum() < 1:
            lulus = False
            L.append(f"| {blok} | - | - | - | - | - | GAGAL: tidak ada IDLE sah |")
            continue
        a, z = idl.iloc[0], idl.iloc[-1]
        d = z["I_avg_mA"] - a["I_avg_mA"]
        st = "OK" if abs(d) <= 0.05 else ("PERIKSA" if abs(d) <= 0.10 else "GAGAL")
        lulus &= st != "GAGAL"
        L.append(f"| {blok} | {a['I_avg_mA']:.3f} | {z['I_avg_mA']:.3f} | {d:+.3f} | {a['V_avg']:.3f} | {z['V_avg']:.3f} | {st} |")

    L += ["", "## 4. Konsistensi (run sah)", ""]
    d_s = (sah["S_cpu_log"] - sah["S_cpu_host"]).abs().max()
    nr = int(sah["N_rusak"].fillna(0).sum())
    L += [f"- Selisih S_cpu logger vs firmware terbesar: {d_s:.4f} poin (batas 0,5)",
          f"- Frame rusak/hilang di UART (total): {nr}",
          f"- Galat jam host: {sah['clk_ppm'].min():.0f} s.d. {sah['clk_ppm'].max():.0f} ppm (dikoreksi dengan jam logger)",
          f"- Selang cuplik INA226 terbesar: {sah['dt_max_us'].max():.0f} us"]
    if STOP_MODE:   # varian Stop: frame RUSAK harus 0; frame hilang sudah dibatasi < 0,25 % per run oleh run_bench (toleransi 18 Nov)
        nb = int(sah["rx_bad"].fillna(0).sum())
        L.append(f"- Varian Stop: frame rusak total {nb} (harus 0); frame hilang {nr} (tiap run < 0,25 % dari yang tiba)")
        lulus &= d_s <= 0.5 and nb == 0
    else:
        lulus &= d_s <= 0.5 and nr == 0

    L += ["", "## 5. Pencilan (pagar Tukey 3 x IQR, hanya dilaporkan, tidak dibuang)", ""]
    ada = False
    for col, lab, sat in METRIK:
        if col in ("TL_pct", "Theta_sps") or col not in sah:
            continue
        for k, g in sah.groupby("kelompok"):
            x = g[col].dropna()
            q1, q3 = np.percentile(x, [25, 75])
            for _, r in g[(g[col] < q1 - 3 * (q3 - q1)) | (g[col] > q3 + 3 * (q3 - q1))].iterrows():
                ada = True
                L.append(f"- {k} {lab}: {r[col]:.4f} {sat} ({r['blok']} {r['file']}; median {np.median(x):.4f})")
    if not ada:
        L.append("- tidak ada")

    L += ["", "## 6. Run gugur dan sebabnya", ""]
    sebab = {}
    for _, r in gug.iterrows():
        for m in str(r["masalah"]).split("; "):
            key = re.sub(r"-?\d+(\.\d+)?", "N", m.split("(")[0]).strip()
            sebab[key] = sebab.get(key, 0) + 1
    L.append(f"- Run gugur di sesi terdaftar: {len(gug)} dari {len(df)} run (termasuk IDLE)")
    for _, r in gug.iterrows():
        L.append(f"  - {r['blok']} {r['file']}: {r['masalah']}")
    L.append("- Per sebab: " + "; ".join(f"{k}: {v}" for k, v in sorted(sebab.items(), key=lambda x: -x[1])))
    if gugur:
        L += [f"- Sesi gugur utuh (data/dev/gugur/, tidak dianalisis): {len(gugur)} sesi, {sum(g['run'] for g in gugur)} run"]
        for g in gugur:
            L.append(f"  - {g['sesi']} ({g['run']} run): {g['catatan']}")
    L += ["", f"## Kesimpulan: {'MEMENUHI STANDAR' if lulus else 'BELUM MEMENUHI STANDAR'}", ""]
    return lulus, "\n".join(L)


# --------------------------------------------------------------------------- ketidakpastian
def ketidakpastian(df, sah, u_rel):
    """Komponen ketidakpastian per kelompok dan metrik.

    u_acak : setengah lebar IK 95 % rerata (t), variasi antar-run.
    u_kal  : u_rel x nilai (galat penguatan kalibrasi shunt berlaku proporsional pada arus/daya/energi,
             termasuk komponen dinamis; offset terkoreksi oleh pengurangan IDLE). Hanya metrik energi.
    u_blok : sd rerata antar-blok (kelompok yang diambil di >1 blok), pergeseran antar sesi yang tersisa.
    u_idle : drift IDLE awal-akhir blok terbesar, dikonversi ke satuan metrik (mempengaruhi P_idle).
    U_sis  : akar jumlah kuadrat u_kal, u_blok, u_idle (sistematis); U_gab = gabungan dengan u_acak.
    Untuk gain % antar-arsitektur u_kal saling menghapus (penguatan sama), jadi tidak dimasukkan.
    """
    d_idle = 0.0
    for _, g in df.groupby("sesi", sort=False):
        idl = g[g["scen"] == "IDLE"].sort_values("urut")
        if len(idl) >= 2:
            d_idle = max(d_idle, abs(idl.iloc[-1]["I_avg_mA"] - idl.iloc[0]["I_avg_mA"]))
    out = []
    for k in KK:
        g = sah[sah["kelompok"] == k]
        V, T, N = g["V_avg"].mean(), g["T_obs_log_s"].mean(), g["N_lyn"].mean()
        for col in UTAMA + PELENGKAP:
            x = g[col].dropna()
            d = deskriptif(x)
            u_acak = d["IK95_atas"] - d["rerata"]
            u_kal = u_rel * abs(d["rerata"]) if col in ENERGI else 0.0
            bm = g.groupby("blok")[col].mean()
            u_blok = float(bm.std(ddof=1)) if len(bm) > 1 else 0.0
            u_idle = {"P_dyn_mW": d_idle * V, "I_dyn_mA": d_idle, "E_lyn_mJ": d_idle * V * T / N,
                      "Q_lyn_mC": d_idle * T / N}.get(col, 0.0)
            u_sis = math.sqrt(u_kal ** 2 + u_blok ** 2 + u_idle ** 2)
            out.append({"kelompok": k, "metrik": col, "rerata": d["rerata"], "u_acak": u_acak, "u_kal": u_kal,
                        "u_blok": u_blok, "u_idle": u_idle, "U_sis": u_sis,
                        "U_gab": math.sqrt(u_sis ** 2 + u_acak ** 2),
                        "U_gab_rel_pct": math.sqrt(u_sis ** 2 + u_acak ** 2) / abs(d["rerata"]) * 100})
    return pd.DataFrame(out), d_idle


# --------------------------------------------------------------------------- RM1/RM3: SW vs HW
def banding_sw_hw(sah, rng, unc):
    out = []
    for sk, (ksw, khw) in SKEN:
        for col in UTAMA + PELENGKAP + ["TL_pct", "Theta_sps"]:
            a, b = kel(sah, ksw, col), kel(sah, khw, col)
            ma, mb = boot_mean(rng, a), boot_mean(rng, b)
            d = delta_hw_baik(a.mean(), b.mean(), col)
            db = delta_hw_baik(ma, mb, col)
            r = {"skenario": sk, "pembanding": f"{ksw} vs {khw}", "metrik": col, "n_sw": len(a), "n_hw": len(b),
                 "rerata_sw": a.mean(), "rerata_hw": b.mean(), "delta": d,
                 "delta_IK95_bawah": ik(db)[0], "delta_IK95_atas": ik(db)[1],
                 "rasio_hw_sw": b.mean() / a.mean() if a.mean() else np.nan}
            if col in ("S_cpu_log", "TL_pct"):
                r["satuan_delta"] = "poin persentase"
            else:
                r["satuan_delta"] = LABEL[col][1]
                if col != "Theta_sps":
                    gb = db / ma * 100
                    r.update(G_pct=d / a.mean() * 100, G_IK95_bawah=ik(gb)[0], G_IK95_atas=ik(gb)[1])
                    # aturan pelaporan persen: rerata SW >= 3 x ketidakpastian sistematis+acak SW
                    u = unc[(unc["kelompok"] == ksw) & (unc["metrik"] == col)]
                    if len(u):
                        r["rerata_sw_per_U"] = a.mean() / u["U_gab"].iloc[0]
                        r["persen_layak"] = bool(r["rerata_sw_per_U"] >= 3)
                    # selisih antar-blok pada delta (sistematis untuk gain): sd dari leave-one-block-out
            if col in UTAMA + PELENGKAP and a.std() > 0 and b.std() > 0:
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore")
                    t = stats.ttest_ind(a, b, equal_var=False)
                    u_ = stats.mannwhitneyu(a, b, alternative="two-sided")
                r.update(welch_t=t.statistic, welch_df=t.df, p_welch=t.pvalue, p_mwu_cek=u_.pvalue,
                         tumpang_tindih=bool(not (a.min() > b.max() or b.min() > a.max())))
            out.append(r)
    t = pd.DataFrame(out)
    t["utama"] = t["metrik"].isin(UTAMA)
    t["p_holm"] = np.nan
    m = t["utama"]
    t.loc[m, "p_holm"] = multipletests(t.loc[m, "p_welch"], method="holm")[1]
    t["keputusan"] = np.where(t["utama"], np.where(t["p_holm"] < 0.05, "tolak H0", "gagal menolak H0"), "")
    return t


# --------------------------------------------------------------------------- RM2: barrage per arsitektur
def pengaruh_barrage(sah, rng):
    out = []
    for arsi, (kn, kb) in ARSI:
        for col in ("S_cpu_log", "P_dyn_mW", "I_dyn_mA", "E_lyn_mJ", "TL_pct", "Theta_sps", "lat_mean_ms_nyata"):
            a, b = kel(sah, kn, col), kel(sah, kb, col)
            bs = boot_mean(rng, b) - boot_mean(rng, a)
            out.append({"arsitektur": arsi, "pembanding": f"{kb} - {kn}", "metrik": col,
                        "normal": a.mean(), "barrage": b.mean(), "selisih": b.mean() - a.mean(),
                        "IK95_bawah": ik(bs)[0], "IK95_atas": ik(bs)[1],
                        "relatif_pct": (b.mean() - a.mean()) / a.mean() * 100 if col not in ("S_cpu_log", "TL_pct") else np.nan})
    return pd.DataFrame(out)


def interaksi(sah, rng):
    """Selisih-dari-selisih: DD = Delta_barrage - Delta_normal (Delta: positif = HW lebih baik)."""
    out = []
    for col in UTAMA + ["E_lyn_mJ", "I_dyn_mA"]:
        x = {k: kel(sah, k, col) for k in KK}
        dn = delta_hw_baik(x["K1"].mean(), x["K2"].mean(), col)
        db = delta_hw_baik(x["K3"].mean(), x["K4"].mean(), col)
        bm = {k: boot_mean(rng, x[k]) for k in KK}
        dd = delta_hw_baik(bm["K3"], bm["K4"], col) - delta_hw_baik(bm["K1"], bm["K2"], col)
        lo, hi = ik(dd)
        if lo <= 0 <= hi:
            taf = "manfaat HW tidak terbukti berbeda antar skenario"
        elif abs(db - dn) < 0.01 * max(abs(dn), abs(db)):
            taf = "berbeda secara statistik tetapi praktis sama (|DD| < 1 % Delta)"
        else:
            taf = "manfaat HW lebih besar saat barrage" if db - dn > 0 else "manfaat HW lebih kecil saat barrage"
        out.append({"metrik": col, "delta_normal": dn, "delta_barrage": db, "DD": db - dn,
                    "IK95_bawah": lo, "IK95_atas": hi, "tafsiran": taf})
    return pd.DataFrame(out)


def tabel_layanan(sah):
    out = []
    for k in KK:
        g = sah[sah["kelompok"] == k]
        out.append({"kelompok": k, "N_dtg": f"{g['N_dtg'].min():.0f}-{g['N_dtg'].max():.0f}",
                    "N_lyn": f"{g['N_lyn'].min():.0f}-{g['N_lyn'].max():.0f}",
                    "N_lwt": f"{g['N_lwt'].min():.0f}-{g['N_lwt'].max():.0f}", "N_ggl": int(g["N_ggl"].sum()),
                    "TL_rerata": g["TL_pct"].mean(), "TL_min": g["TL_pct"].min(), "TL_maks": g["TL_pct"].max(),
                    "TL_model": g["TL_model_pct"].mean(),
                    "rho": (g["lambda_eff_rps"] * g["lat_mean_ms_nyata"] / 1e3).mean(),
                    "Theta": g["Theta_sps"].mean(), "Theta_maks_model": g["Theta_max_sps"].mean()})
    return pd.DataFrame(out)


def model_duty_per_run(sah):
    """Pemeriksaan konsistensi I_avg = I_aktif D + I_tidur (1 - D) per run (keadaan dari pin status)."""
    d = (sah["I_model_mA"] - sah["I_avg_mA"])
    return pd.DataFrame([{"kelompok": k, "n": int((sah["kelompok"] == k).sum()),
                          "I_aktif_mA": sah.loc[sah["kelompok"] == k, "I_aktif_mA"].mean(),
                          "I_tidur_mA": sah.loc[sah["kelompok"] == k, "I_tidur_mA"].mean(),
                          "selisih_rerata_mA": d[sah["kelompok"] == k].mean(),
                          "selisih_maks_abs_mA": d[sah["kelompok"] == k].abs().max()} for k in KK])


def leave_one_block_out(sah):
    out = []
    for nama, s in [("semua blok", sah)] + [(f"tanpa {b}", sah[sah["blok"] != b]) for b in sah["blok"].unique()]:
        for sk, (ksw, khw) in SKEN:
            for col in UTAMA + ["E_lyn_mJ", "I_dyn_mA"]:
                a, b = kel(s, ksw, col), kel(s, khw, col)
                d = delta_hw_baik(a.mean(), b.mean(), col)
                out.append({"analisis": nama, "skenario": sk, "metrik": col, "n_sw": len(a), "n_hw": len(b),
                            "delta": d, "G_pct": d / a.mean() * 100 if col != "S_cpu_log" else np.nan})
    return pd.DataFrame(out)


# --------------------------------------------------------------------------- gambar
def gambar(out, df, sah, pins, ba_mm):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    gdir = out / "gambar"
    gdir.mkdir(exist_ok=True)
    xt = ["K1\nSW normal", "K2\nHW normal", "K3\nSW barrage", "K4\nHW barrage"]
    for col, lab, sat in METRIK:
        if col == "V_avg":
            continue
        data = [kel(sah, k, col) for k in KK]
        fig, ax = plt.subplots(figsize=(6, 4))
        ax.boxplot(data, showfliers=True)
        ax.set_xticks([1, 2, 3, 4], xt)
        ax.set_ylabel(f"{lab} ({sat})")
        ax.set_title(f"{lab} (n = {', '.join(str(len(x)) for x in data)})")
        ax.grid(axis="y", alpha=0.3)
        fig.tight_layout()
        fig.savefig(gdir / f"box_{col}.png", dpi=150)
        plt.close(fig)
    for col in UTAMA + ["E_lyn_mJ", "I_dyn_mA", "TL_pct"]:
        fig, ax = plt.subplots(figsize=(5, 4))
        for arsi, (kn, kb), mk in (("Software", ("K1", "K3"), "o-"), ("DS28E38", ("K2", "K4"), "s--")):
            m = [kel(sah, kn, col).mean(), kel(sah, kb, col).mean()]
            e = [deskriptif(kel(sah, k, col))["IK95_atas"] - mm for k, mm in zip((kn, kb), m)]
            ax.errorbar([0, 1], m, yerr=e, fmt=mk, capsize=4, label=arsi)
        ax.set_xticks([0, 1], ["Normal (1/s)", "Barrage (10/s)"])
        ax.set_ylabel(f"{LABEL[col][0]} ({LABEL[col][1]})")
        ax.set_title(f"Interaksi arsitektur x skenario: {LABEL[col][0]}", fontsize=10)
        ax.legend()
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(gdir / f"interaksi_{col}.png", dpi=150)
        plt.close(fig)
    ok = df[df["sah"]]
    for col in ("I_avg_mA", "I_dyn_mA", "P_dyn_mW", "S_cpu_log", "lat_mean_ms_nyata", "V_avg"):
        fig, ax = plt.subplots(figsize=(9, 3.5))
        for k, mk in zip(list(KK) + ["K1-IDLE", "K2-IDLE"], ["o", "s", "^", "v", "x", "+"]):
            g = ok[ok["kelompok"] == k]
            if len(g) and g[col].notna().any():
                ax.plot(g["urut_global"], g[col], mk, ms=4, label=k)
        for _, g in df.groupby("blok", sort=False):
            ax.axvline(g["urut_global"].min() - 0.5, color="0.7", lw=0.8)
        ax.set_xlabel("urutan run (kronologis)")
        ax.set_ylabel(col)
        ax.set_title(f"Drift: {col} terhadap urutan run (garis = batas blok)")
        ax.legend(fontsize=7, ncol=6)
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(gdir / f"drift_{col}.png", dpi=150)
        plt.close(fig)

    def ba_plot(x, y, nama, satuan, fn):
        x, y = np.asarray(x, float), np.asarray(y, float)
        b = bland_altman(x, y)
        fig, ax = plt.subplots(figsize=(5.5, 4))
        ax.plot((x + y) / 2, y - x, ".", ms=4)
        for v, ls in ((b["bias"], "-"), (b["LoA_bawah"], "--"), (b["LoA_atas"], "--")):
            ax.axhline(v, ls=ls, color="k", lw=0.8)
        ax.set_xlabel(f"rerata kedua metode ({satuan})")
        ax.set_ylabel(f"selisih ({satuan})")
        ax.set_title(f"Bland-Altman: {nama}\nbias {b['bias']:.4g}, LoA [{b['LoA_bawah']:.4g}, {b['LoA_atas']:.4g}]", fontsize=9)
        ax.grid(alpha=0.3)
        fig.tight_layout()
        fig.savefig(gdir / fn, dpi=150)
        plt.close(fig)
    for a in ("K1", "K2"):
        p = pins[pins["arch"] == a] if len(pins) else pins
        if len(p):
            ba_plot(p["dwt_ms"], p["pin_ms"], f"latensi {a} per tanda tangan (pin - DWT)", "ms", f"ba_latensi_{a}.png")
    ba_plot(sah["S_cpu_host"], sah["S_cpu_log"], "S_cpu (pin - firmware), 120 run", "poin %", "ba_S_cpu.png")
    if ba_mm is not None:
        ba_plot(ba_mm["I_mm_mA"], ba_mm["I_avg_mA"], "arus (INA226 - multimeter)", "mA", "ba_arus_multimeter.png")


# --------------------------------------------------------------------------- utama
def f(v, n=4):
    if v is None or (isinstance(v, float) and math.isnan(v)):
        return "-"
    return f"{v:.{n}g}"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--manifest", default=str(ROOT / "data" / "tahap_d.txt"))
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF,
                    help="hambatan shunt efektif (Ohm); bawaan = kalibrasi 21 Nov 2026 (nominal modul 0,100)")
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB, help="offset nol (satuan 2,5 uV)")
    ap.add_argument("--u-rel", type=float, default=kp.U_REL_KAL,
                    help="ketidakpastian relatif kalibrasi shunt (bawaan 1,5 %%; sebelum kalibrasi 5 %%)")
    ap.add_argument("--multimeter", default=None, help="CSV kolom file,I_mm_mA (Bland-Altman INA226 vs multimeter)")
    ap.add_argument("--keluar", default=None)
    ap.add_argument("--stop", action="store_true", help="manifest berisi sesi varian Stop (arsitektur K1S/K2S)")
    ap.add_argument("--tanpa-gambar", action="store_true")
    args = ap.parse_args()
    rng = np.random.default_rng(SEED)

    global STOP_MODE
    STOP_MODE = args.stop
    df, pins = muat(baca_manifest(Path(args.manifest)), args.rshunt, args.offset, args.stop)
    out = Path(args.keluar) if args.keluar else ROOT / "data" / "analisis" / f"{datetime.now():%Y%m%d_%H%M%S}_r{args.rshunt:g}"
    out.mkdir(parents=True, exist_ok=True)
    df.to_csv(out / "runs.csv", index=False)
    pins.to_csv(out / "latensi_pin_vs_dwt.csv", index=False)
    sah = df[df["sah"] & df["kelompok"].isin(KK)].copy()

    lulus, teks = pemeriksaan(df, sah, muat_gugur())
    (out / "pemeriksaan.md").write_text(teks, encoding="utf-8")

    des = []
    for k in KK:
        g = sah[sah["kelompok"] == k]
        for col, lab, sat in METRIK + [("lat_pin_ms", "Latensi episode pin (logger)", "ms")]:
            if col in g and g[col].notna().any():
                des.append({"kelompok": k, "metrik": col, "label": lab, "satuan": sat, **deskriptif(g[col])})
    for a in ("K1", "K2"):
        g = df[df["sah"] & (df["scen"] == "IDLE") & (df["arch"] == a)]
        for col in ("I_avg_mA", "P_avg_mW", "V_avg"):
            des.append({"kelompok": f"{a}-IDLE", "metrik": col, "label": col, "satuan": "", **deskriptif(g[col])})
    des = pd.DataFrame(des)
    des.to_csv(out / "deskriptif.csv", index=False)

    unc, d_idle = ketidakpastian(df, sah, args.u_rel)
    unc.to_csv(out / "ketidakpastian.csv", index=False)
    cmp_ = banding_sw_hw(sah, rng, unc)
    cmp_.to_csv(out / "banding_sw_hw.csv", index=False)
    rm2 = pengaruh_barrage(sah, rng)
    rm2.to_csv(out / "pengaruh_barrage.csv", index=False)
    dd = interaksi(sah, rng)
    dd.to_csv(out / "interaksi_dd.csv", index=False)
    lay = tabel_layanan(sah)
    lay.to_csv(out / "layanan.csv", index=False)
    duty = model_duty_per_run(sah)
    duty.to_csv(out / "model_duty_per_run.csv", index=False)
    lobo = leave_one_block_out(sah)
    lobo.to_csv(out / "leave_one_block_out.csv", index=False)

    ba = []
    for a in ("K1", "K2"):
        p = pins[pins["arch"] == a] if len(pins) else pins
        if len(p):
            ba.append({"pasangan": f"latensi per tanda tangan {a} normal: pin - DWT (ms)", **bland_altman(p["dwt_ms"], p["pin_ms"])})
    ba.append({"pasangan": "S_cpu 120 run: pin - firmware (poin %)", **bland_altman(sah["S_cpu_host"], sah["S_cpu_log"])})
    ba_mm = None
    if args.multimeter:
        ba_mm = pd.read_csv(args.multimeter).merge(df[["file", "I_avg_mA"]], on="file", how="left")
        ba.append({"pasangan": "arus rata-rata: INA226 - multimeter (mA)", **bland_altman(ba_mm["I_mm_mA"], ba_mm["I_avg_mA"])})
    ba = pd.DataFrame(ba)
    ba.to_csv(out / "bland_altman.csv", index=False)
    n_cocok = int(sah["pin_episode_cocok"].fillna(False).astype(bool).sum()) if "pin_episode_cocok" in sah else 0

    if not args.tanpa_gambar:
        gambar(out, df, sah, pins, ba_mm)

    # ----------------------------------------------------------------- ringkasan
    R = [f"# Ringkasan analisis {'varian Stop (deep sleep)' if STOP_MODE else 'tahap D'}", "",
         f"Dibuat {datetime.now():%Y-%m-%d %H:%M}; rshunt = {args.rshunt} Ohm, offset = {args.offset} LSB, "
         f"u_rel kalibrasi = {args.u_rel:.1%}; bootstrap {N_BOOT} resampel (seed {SEED}); "
         f"keutuhan data: {'MEMENUHI STANDAR' if lulus else 'BELUM MEMENUHI STANDAR'}.", "",
         "## 1. Deskriptif: rerata (sd) [min-maks]", "",
         "| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |", "|---|---|---|---|---|"]
    for col, lab, sat in METRIK + [("lat_pin_ms", "Latensi episode pin (logger)", "ms")]:
        cells = []
        for k in KK:
            r = des[(des["kelompok"] == k) & (des["metrik"] == col)]
            cells.append(f"{r['rerata'].iloc[0]:.5g} ({r['sd'].iloc[0]:.2g}) [{r['min'].iloc[0]:.5g}-{r['maks'].iloc[0]:.5g}]"
                         if len(r) else "-")
        R.append(f"| {lab} ({sat}) | " + " | ".join(cells) + " |")

    R += ["", "## 2. Ketidakpastian (rerata +- U_gab; komponen acak dan sistematis)", "",
          "| Kelompok | Metrik | Rerata | u_acak | u_kal | u_blok | u_idle | U_gab (rel.) |", "|---|---|---|---|---|---|---|---|"]
    for _, r in unc.iterrows():
        R.append(f"| {r['kelompok']} | {r['metrik']} | {r['rerata']:.5g} | {r['u_acak']:.2g} | {r['u_kal']:.2g} | "
                 f"{r['u_blok']:.2g} | {r['u_idle']:.2g} | {r['U_gab']:.2g} ({r['U_gab_rel_pct']:.2f} %) |")
    R.append(f"- Drift IDLE awal-akhir blok terbesar: {d_idle:.3f} mA. u_kal berlaku pada nilai mutlak dan saling "
             "menghapus pada gain % antar-arsitektur.")

    R += ["", "## 3. RM1 dan RM3: software vs DS28E38 (positif = DS28E38 lebih baik)", "",
          "| Skenario | Metrik | SW | HW | Delta [IK 95 %] | Gain % [IK 95 %] | Rasio HW/SW | Welch p (Holm) | Tumpang tindih | Keputusan |",
          "|---|---|---|---|---|---|---|---|---|---|"]
    for _, r in cmp_.iterrows():
        g = f"{r['G_pct']:.2f} [{r['G_IK95_bawah']:.2f}, {r['G_IK95_atas']:.2f}]" if not pd.isna(r.get("G_pct")) else "-"
        if not pd.isna(r.get("persen_layak")) and r.get("persen_layak") is False:
            g += " (persen tidak layak)"
        p = (f"{r['p_welch']:.2g} ({r['p_holm']:.2g})" if r["utama"] else f"{r['p_welch']:.2g}") if not pd.isna(r.get("p_welch")) else "-"
        tt = "-" if pd.isna(r.get("tumpang_tindih")) else ("ya" if r["tumpang_tindih"] else "tidak")
        R.append(f"| {r['skenario']} | {r['metrik']} | {r['rerata_sw']:.5g} | {r['rerata_hw']:.5g} | {r['delta']:.4g} "
                 f"[{r['delta_IK95_bawah']:.4g}, {r['delta_IK95_atas']:.4g}] {r['satuan_delta']} | {g} | {r['rasio_hw_sw']:.4g} | "
                 f"{p} | {tt} | {r['keputusan']} |")
    pl = cmp_[cmp_["persen_layak"].notna()] if "persen_layak" in cmp_ else cmp_.iloc[0:0]
    R.append(f"- Aturan persen (rerata SW >= 3 x U_gab SW): rasio terkecil {pl['rerata_sw_per_U'].min():.1f}." if len(pl) else "")
    R.append("- Mann-Whitney U (cek) di banding_sw_hw.csv kolom p_mwu_cek; tanpa tumpang tindih nilainya mentok di "
             "3,0e-11 (minimum untuk 30 vs 30).")

    R += ["", "## 4. RM2: pengaruh barrage pada tiap arsitektur (barrage - normal)", "",
          "| Arsitektur | Metrik | Normal | Barrage | Selisih [IK 95 %] | Relatif |", "|---|---|---|---|---|---|"]
    for _, r in rm2.iterrows():
        R.append(f"| {r['arsitektur']} | {r['metrik']} | {r['normal']:.5g} | {r['barrage']:.5g} | {r['selisih']:.5g} "
                 f"[{r['IK95_bawah']:.5g}, {r['IK95_atas']:.5g}] | {f(r['relatif_pct'], 4)}{'' if pd.isna(r['relatif_pct']) else ' %'} |")

    R += ["", "## 5. RM3: interaksi, DD = Delta_barrage - Delta_normal", "",
          "| Metrik | Delta normal | Delta barrage | DD [IK 95 %] | Tafsiran |", "|---|---|---|---|---|"]
    for _, r in dd.iterrows():
        R.append(f"| {r['metrik']} | {r['delta_normal']:.5g} | {r['delta_barrage']:.5g} | {r['DD']:.4g} "
                 f"[{r['IK95_bawah']:.4g}, {r['IK95_atas']:.4g}] | {r['tafsiran']} |")

    R += ["", "## 6. H4: tingkat layanan dan model utilisasi", "",
          "| Kelompok | N_dtg | N_lyn | N_lwt | N_ggl | TL [min-maks] | TL model | rho | Theta | Theta_maks |",
          "|---|---|---|---|---|---|---|---|---|---|"]
    for _, r in lay.iterrows():
        R.append(f"| {r['kelompok']} | {r['N_dtg']} | {r['N_lyn']} | {r['N_lwt']} | {r['N_ggl']} | {r['TL_rerata']:.2f} "
                 f"[{r['TL_min']:.2f}-{r['TL_maks']:.2f}] | {r['TL_model']:.2f} | {r['rho']:.3f} | {r['Theta']:.4f} | {r['Theta_maks_model']:.3f} |")

    R += ["", "## 7. Kesesuaian instrumen", "", "| Pasangan | n | Bias | Bias relatif | Batas kesesuaian 95 % |", "|---|---|---|---|---|"]
    for _, r in ba.iterrows():
        R.append(f"| {r['pasangan']} | {r['n']} | {r['bias']:.4g} | {r['bias_rel_pct']:.3g} % | [{r['LoA_bawah']:.4g}, {r['LoA_atas']:.4g}] |")
    R += [f"- Episode pin dipasangkan dengan DWT pada {n_cocok} dari {int((sah['scen'] == 'A').sum())} run normal; pada "
          "barrage episode per tanda tangan tidak terpisah (latensi barrage hanya dari DWT).",
          "- Bias positif pin - DWT = kerja CPU di luar jendela DWT dalam satu episode bangun (tetap per tanda tangan)."]
    if ba_mm is None:
        R.append("- Arus INA226 vs multimeter: BELUM (jalankan ulang dengan --multimeter).")
    R += ["", "Model duty cycle per run (I_model - I_avg):", "", "| Kelompok | I_aktif | I_tidur | Selisih rerata | Selisih maks |",
          "|---|---|---|---|---|"]
    for _, r in duty.iterrows():
        R.append(f"| {r['kelompok']} | {r['I_aktif_mA']:.3f} mA | {r['I_tidur_mA']:.3f} mA | {r['selisih_rerata_mA']:+.4f} mA | {r['selisih_maks_abs_mA']:.4f} mA |")

    R += ["", "## 8. Leave-one-block-out (gain % / Delta S_cpu poin)", "",
          "| Analisis | Skenario | Latensi | P_dyn | E_lyn | I_dyn | Delta S_cpu |", "|---|---|---|---|---|---|---|"]
    for (nm, sk), g in lobo.groupby(["analisis", "skenario"], sort=False):
        v = {r["metrik"]: r for _, r in g.iterrows()}
        R.append(f"| {nm} | {sk} | " + " | ".join(f"{v[c]['G_pct']:.2f} %" for c in
                                                     ("lat_mean_ms_nyata", "P_dyn_mW", "E_lyn_mJ", "I_dyn_mA"))
                 + f" | {v['S_cpu_log']['delta']:.3f} |")
    (out / "ringkasan.md").write_text("\n".join(R), encoding="utf-8")
    print(teks)
    print("\n".join(R))
    print(f"\nkeluaran: {out}")


if __name__ == "__main__":
    main()
