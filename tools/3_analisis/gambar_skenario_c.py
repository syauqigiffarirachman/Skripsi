"""Gambar Bab 5 yang memuat skenario C (kapasitas, rho = 1; K5 dan K6) di samping K1-K4 (28 Nov 2026).

Membuat (nama berkas mengikuti nomor gambar di naskah v4):
  5.1  Frame permintaan hilang per run: K4 (barrage) dan K6 (kapasitas)
  5.7  Durasi tidur CPU, enam kelompok
  5.8  Daya dinamis, enam kelompok
  5.9  Energi dinamis per permintaan terlayani, enam kelompok
  5.11 Tingkat layanan terukur vs model, enam kelompok
  5.12 Throughput terukur dan garis kapasitas, enam kelompok
  5.13 BARU  Durasi tidur CPU terhadap utilisasi rho
  5.14 BARU  Arus rata-rata terhadap laju permintaan datang (garis kapasitas)
Gambar 5.2-5.6 dan 5.10 tidak berubah (lihat laporan v5 bagian F14).

Data : data/analisis/kapasitas/runs.csv (keluaran analisis_kapasitas.py; hanya run sah).
Tegangan: ATAS PERMINTAAN peneliti (28 Nov 2026), tegangan USB 27 Nov dianggap SAMA dengan 18 Nov. Daya dan energi K5/K6
  dihitung ulang dengan V = rerata V_avg K1-K4 (arsitektur yang sama) sebagai ganti V_avg 27 Nov (~5,13 V); arus,
  muatan, durasi tidur, latensi, dan tingkat layanan tidak terpengaruh. Lihat tegangan_ke_18nov().
Ketidakpastian: batang galat K5/K6 = akar(u_kal^2 + u_acak^2) dengan u_kal = U_REL_KAL x rerata (komponen blok dan
  IDLE diabaikan; pada K1-K4 selisihnya < 0,1 %).

Pakai:  .venv-analisis/Scripts/python tools/3_analisis/gambar_skenario_c.py [--keluar data/analisis/kapasitas/gambar]
"""
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401
import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402

import kalibrasi_param as kp  # noqa: E402
import gambar_bab4_stop as g4  # noqa: E402   (warna, format koma, gaya seragam dengan gambar lama)

KAP = ROOT / "data" / "analisis" / "kapasitas"
C_SW, C_HW, FMT, koma = g4.C_SW, g4.C_HW, g4.FMT, g4.koma
URUT = ["K1", "K2", "K5", "K6", "K3", "K4"]                  # beban naik: normal, kapasitas, barrage
POS = {"K1": 0, "K2": 1, "K5": 2.6, "K6": 3.6, "K3": 5.2, "K4": 6.2}
WARNA = {"K1": C_SW, "K2": C_HW, "K5": C_SW, "K6": C_HW, "K3": C_SW, "K4": C_HW}
ARSIR = {"K1": "", "K2": "", "K5": "..", "K6": "..", "K3": "//", "K4": "//"}
NAMA = {"K1": "K1\nSoftware\nnormal", "K2": "K2\nDS28E38\nnormal", "K5": "K5\nSoftware\nkapasitas",
        "K6": "K6\nDS28E38\nkapasitas", "K3": "K3\nSoftware\nbarrage", "K4": "K4\nDS28E38\nbarrage"}
ARCH = {"K1": "K1", "K2": "K2", "K3": "K1", "K4": "K2", "K5": "K1", "K6": "K2"}


def muat():
    r = pd.read_csv(KAP / "runs.csv")
    r = r[r["kelompok"].isin(URUT)].copy()
    return tegangan_ke_18nov(r)


def tegangan_ke_18nov(r):
    """K5/K6: daya dan energi dihitung ulang pada tegangan K1-K4 (18 Nov). Mengembalikan salinan dengan kolom diperbarui."""
    r = r.copy()
    sah = r[r["sah"]]
    v18 = {a: sah[sah["kelompok"].isin(["K1", "K2", "K3", "K4"]) & (sah["arch"] == a)]["V_avg"].mean() for a in ("K1", "K2")}
    # pemeriksaan: rumus E_lyn = P_dyn x T / N_lyn harus mengembalikan kolom lama (sebelum koreksi)
    c = r[r["kelompok"].isin(["K5", "K6"]) & r["sah"]]
    e0 = (c["P_dyn_mW"] * c["T_obs_log_s"] / c["N_lyn"]).to_numpy()
    assert np.allclose(e0, c["E_lyn_mJ"].to_numpy(), rtol=1e-6), "rumus E_lyn tidak cocok dengan analisis"
    m = r["kelompok"].isin(["K5", "K6"])
    f = r.loc[m, "arch"].map(v18) / r.loc[m, "V_avg"]
    r.loc[m, "P_avg_mW"] = r.loc[m, "P_avg_mW"] * f
    r.loc[m, "P_dyn_mW"] = r.loc[m, "P_avg_mW"] - r.loc[m, "P_idle_mW"]
    r.loc[m, "E_lyn_mJ"] = r.loc[m, "P_dyn_mW"] * r.loc[m, "T_obs_log_s"] / r.loc[m, "N_lyn"]
    r.loc[m, "E_per_sign_mJ"] = r.loc[m, "E_per_sign_mJ"] * f
    r.attrs["v18"] = v18
    return r


def ringkas(r, kol):
    s = r[r["sah"]]
    out = {}
    for k in URUT:
        x = s.loc[s["kelompok"] == k, kol].dropna()
        out[k] = (float(x.mean()), float(x.std(ddof=1) / np.sqrt(len(x))))
    return out


def sumbu_kelompok(ax):
    ax.set_xticks([POS[k] for k in URUT])
    ax.set_xticklabels([NAMA[k] for k in URUT], fontsize=8.5)
    ax.set_xlim(-0.6, 6.8)
    ax.yaxis.set_major_formatter(FMT)
    for x in (1.8, 4.4):
        ax.axvline(x, color="0.8", lw=0.8, ls=":")


def legenda(ax, atas=1.14):
    ax.legend(handles=[Patch(facecolor=C_SW, label="Software"), Patch(facecolor=C_HW, label="DS28E38"),
                       Patch(facecolor="white", edgecolor="0.3", label="Normal (1/s)"),
                       Patch(facecolor="white", edgecolor="0.3", hatch="..", label="Kapasitas (ρ ≈ 1)"),
                       Patch(facecolor="white", edgecolor="0.3", hatch="//", label="Barrage (10/s)")],
              ncol=5, loc="upper center", bbox_to_anchor=(0.5, atas), frameon=False, fontsize=8)


def batang(ax, nilai, galat=None, fmt=lambda v: f"{v:.2f}".replace(".", ","), ofs=0.0):
    for k in URUT:
        v = nilai[k]
        ax.bar(POS[k], v, 0.8, color=WARNA[k], hatch=ARSIR[k], edgecolor="white")
        if galat is not None:
            ax.errorbar(POS[k], v, yerr=galat[k], color="0.15", capsize=4, lw=1.1, capthick=1.1)
        ax.text(POS[k], v + (galat[k] if galat is not None else 0) + ofs, fmt(v), ha="center", fontsize=9)
    sumbu_kelompok(ax)


def simpan(fig, out, nama):
    fig.tight_layout()
    fig.savefig(out / nama, bbox_inches="tight")
    plt.close(fig)


def galat_gab(r, kol):
    rs = ringkas(r, kol)
    return {k: float(np.hypot(kp.U_REL_KAL * abs(m), se)) for k, (m, se) in rs.items()}


# ------------------------------------------------------------------ gambar
def g51(r, out):
    """Frame hilang per run, K4 dan K6, dengan satu batas yang sama: 1 % dari permintaan yang tiba."""
    fig, axs = plt.subplots(1, 2, figsize=(10.6, 4.2))
    for ax, k, jd in [(axs[0], "K4", "(a) K4 DS28E38, barrage 10/s"), (axs[1], "K6", "(b) K6 DS28E38, kapasitas ρ ≈ 1")]:
        g = r[r["kelompok"] == k]
        dalam = (g["rx_lost"] / g["arrived"] * 100) < 1.0
        x = np.arange(int(g["rx_lost"].max()) + 1)
        a = g[dalam]["rx_lost"].value_counts().reindex(x, fill_value=0)
        b = g[~dalam]["rx_lost"].value_counts().reindex(x, fill_value=0)
        ax.bar(x, a, 0.6, color=C_HW, label="Hilang < 1 %")
        ax.bar(x, b, 0.6, bottom=a, color="white", edgecolor=C_HW, hatch="//", label="Hilang ≥ 1 %")
        for i in x:
            t = a.iloc[i] + b.iloc[i]
            if t:
                ax.text(i, t + 0.4, str(int(t)), ha="center", fontsize=9)
        batas = 0.01 * g["arrived"].mean()
        ax.axvline(batas, color="k", ls="--", lw=1.0)
        ax.text(batas + 0.08, (a + b).max() * 1.05, f"batas 1 %\n(±{koma(round(batas, 1))} frame)", fontsize=8.5, va="top")
        ax.set_xticks(x)
        ax.set_xlabel("Jumlah frame permintaan hilang per run")
        ax.set_ylabel("Jumlah run")
        ax.yaxis.set_major_locator(plt.MaxNLocator(integer=True))
        ax.set_title(jd, loc="left")
        ax.set_ylim(0, (a + b).max() * 1.35 + 1)
        ax.legend(frameon=False, loc="upper center", fontsize=8.5)
    simpan(fig, out, "Gambar_5.1_frame_hilang_K4_K6.png")


def g57(r, out):
    v = {k: m for k, (m, _) in ringkas(r, "S_cpu_log").items()}
    fig, ax = plt.subplots(figsize=(8.4, 4.6))
    batang(ax, v, ofs=1.2)
    ax.set_ylim(0, 112)
    ax.set_ylabel("Durasi tidur dalam CPU (% dari jendela 60 s)")
    legenda(ax)
    simpan(fig, out, "Gambar_5.7_durasi_tidur_cpu.png")


def _galat_fig(r, out, kol, ylabel, nama):
    v = {k: m for k, (m, _) in ringkas(r, kol).items()}
    g = galat_gab(r, kol)
    ymax = max(v[k] + g[k] for k in v) * 1.14
    fig, ax = plt.subplots(figsize=(8.4, 4.6))
    batang(ax, v, galat=g, ofs=0.012 * ymax)
    ax.set_ylim(0, ymax)
    ax.set_ylabel(ylabel)
    legenda(ax)
    ax.text(0.5, -0.33, f"Batang galat = ketidakpastian gabungan (didominasi kalibrasi shunt ±{koma(round(kp.U_REL_KAL * 100, 2))} %). "
            "K5/K6 dihitung pada tegangan 18 Nov.", transform=ax.transAxes, ha="center", fontsize=7.8, color="0.35")
    simpan(fig, out, nama)


def g58(r, out):
    _galat_fig(r, out, "P_dyn_mW", "Daya dinamis (mW)", "Gambar_5.8_daya_dinamis.png")


def g59(r, out):
    _galat_fig(r, out, "E_lyn_mJ", "Energi dinamis per permintaan terlayani (mJ)", "Gambar_5.9_energi_per_permintaan.png")


def g511(r, out):
    ter = {k: m for k, (m, _) in ringkas(r, "TL_pct").items()}
    mod = {k: m for k, (m, _) in ringkas(r, "TL_model_pct").items()}
    fig, ax = plt.subplots(figsize=(9.0, 4.7))
    l = 0.36
    for k in URUT:
        ax.bar(POS[k] - l / 2 - 0.02, ter[k], l, color=WARNA[k], hatch=ARSIR[k], edgecolor="white")
        ax.bar(POS[k] + l / 2 + 0.02, mod[k], l, facecolor="white", edgecolor=WARNA[k], hatch=ARSIR[k], lw=1.4)
        ax.text(POS[k] - l / 2 - 0.02, ter[k] + 1.5, f"{ter[k]:.1f}".replace(".", ","), ha="center", fontsize=8)
        ax.text(POS[k] + l / 2 + 0.02, mod[k] + 1.5, f"{mod[k]:.1f}".replace(".", ","), ha="center", fontsize=8)
    sumbu_kelompok(ax)
    ax.set_ylim(0, 114)
    ax.set_ylabel("Tingkat layanan (% permintaan terlayani)")
    ax.legend(handles=[Patch(facecolor="0.4", label="Terukur"), Patch(facecolor="white", edgecolor="0.4", lw=1.4, label="Model utilisasi"),
                       Patch(facecolor=C_SW, label="Software"), Patch(facecolor=C_HW, label="DS28E38")],
              ncol=4, loc="upper center", bbox_to_anchor=(0.5, 1.14), frameon=False, fontsize=8.5)
    simpan(fig, out, "Gambar_5.11_tingkat_layanan_vs_model.png")


def g512(r, out):
    v = {k: m for k, (m, _) in ringkas(r, "Theta_sps").items()}
    cap = {a: float(r[(r["sah"]) & (r["arch"] == a)]["Theta_max_sps"].mean()) for a in ("K1", "K2")}
    fig, ax = plt.subplots(figsize=(8.6, 4.6))
    batang(ax, v, ofs=0.18)
    ax.hlines(cap["K1"], -0.6, 6.8, colors=C_SW, linestyles="--", lw=1.3)
    ax.hlines(cap["K2"], -0.6, 6.8, colors=C_HW, linestyles="--", lw=1.3)
    ax.text(6.85, cap["K1"], "kapasitas\nSoftware " + f"{cap['K1']:.2f}".replace(".", ","), color=C_SW, fontsize=8.5, va="center")
    ax.text(6.85, cap["K2"], "kapasitas\nDS28E38 " + f"{cap['K2']:.2f}".replace(".", ","), color=C_HW, fontsize=8.5, va="center")
    ax.set_xlim(-0.6, 8.2)
    ax.set_ylim(0, 10.2)
    ax.set_ylabel("Throughput (tanda tangan/s)")
    legenda(ax)
    simpan(fig, out, "Gambar_5.12_throughput_vs_batas.png")


def _rho(r):
    s = r[r["sah"]].copy()
    s["rho"] = s["lambda_eff_rps"] * s["lat_mean_ms_nyata"] / 1000.0
    return s


def g513(r, out):
    s = _rho(r)
    fig, ax = plt.subplots(figsize=(7.8, 4.7))
    for nama, ks, c, mk in [("Software", ["K1", "K5", "K3"], C_SW, "o"), ("DS28E38", ["K2", "K6", "K4"], C_HW, "s")]:
        xs = [s.loc[s.kelompok == k, "rho"].mean() for k in ks]
        ys = [s.loc[s.kelompok == k, "S_cpu_log"].mean() for k in ks]
        ax.plot(xs, ys, "-" + mk, color=c, lw=2, ms=8, label=nama)
        ofs = ([(6, 8, "left"), (-9, 16, "right"), (9, -24, "left")] if nama == "DS28E38"
               else [(6, 8, "left"), (8, 10, "left"), (-8, 10, "right")])
        for x, y, tag, (dx, dy, ha) in zip(xs, ys, ["normal", "kapasitas", "barrage"], ofs):
            ax.annotate(f"{tag}\n{y:.1f}".replace(".", ","), (x, y), textcoords="offset points", xytext=(dx, dy),
                        fontsize=8, color=c, ha=ha)
    ax.axvline(1.0, color="k", ls="--", lw=1.0)
    ax.text(1.03, 60, "ρ = 1\n(tepat di kapasitas)", fontsize=8.5)
    ax.set_xscale("log")
    ax.set_xlim(0.08, 9)
    ax.set_ylim(-3, 108)
    ax.set_xlabel("Utilisasi ρ = laju permintaan terlayani × waktu satu tanda tangan (skala log)")
    ax.set_ylabel("Durasi tidur dalam CPU (%)")
    ax.xaxis.set_major_formatter(FMT)
    ax.yaxis.set_major_formatter(FMT)
    ax.legend(frameon=False, loc="upper right")
    simpan(fig, out, "Gambar_5.13_durasi_tidur_terhadap_utilisasi.png")


def g514(r, out):
    s = r[r["sah"]]
    fig, ax = plt.subplots(figsize=(7.8, 4.7))
    for nama, ks, c, mk, a in [("Software", ["K1", "K5", "K3"], C_SW, "o", "K1"), ("DS28E38", ["K2", "K6", "K4"], C_HW, "s", "K2")]:
        xs = [s.loc[s.kelompok == k, "lambda_eff_rps"].mean() for k in ks]
        ys = [s.loc[s.kelompok == k, "I_avg_mA"].mean() for k in ks]
        ax.plot(xs, ys, "-" + mk, color=c, lw=2, ms=8, label=nama)
        ofs = ([(6, -22, "left"), (-9, -34, "right"), (9, -24, "left")] if nama == "DS28E38"
               else [(6, -22, "left")] * 3)
        for x, y, tag, (dx, dy, ha) in zip(xs, ys, ["normal", "kapasitas", "barrage"], ofs):
            ax.annotate(f"{tag}\n{y:.1f}".replace(".", ","), (x, y), textcoords="offset points", xytext=(dx, dy),
                        fontsize=8, color=c, ha=ha)
        cap = float(s[s["arch"] == a]["Theta_max_sps"].mean())
        ax.axvline(cap, color=c, ls="--", lw=1.1)
        ax.text(cap, 0.3, f"kapasitas {cap:.2f}".replace(".", ","), color=c, fontsize=8, rotation=90, ha="right", va="bottom")
    ax.set_xscale("log")
    ax.set_xlim(0.7, 14)
    ax.set_ylim(0, 20)
    ax.set_xlabel("Laju permintaan datang (permintaan/detik, skala log)")
    ax.set_ylabel("Arus rata-rata papan (mA)")
    ax.xaxis.set_major_formatter(FMT)
    ax.yaxis.set_major_formatter(FMT)
    ax.legend(frameon=False, loc="upper left")
    simpan(fig, out, "Gambar_5.14_arus_terhadap_laju_datang.png")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--keluar", default=str(KAP / "gambar"))
    a = ap.parse_args()
    out = Path(a.keluar)
    out.mkdir(parents=True, exist_ok=True)
    g4.gaya()
    r = muat()
    for f in (g51, g57, g58, g59, g511, g512, g513, g514):
        f(r, out)
    print("V acuan 18 Nov:", {k: round(v, 3) for k, v in r.attrs["v18"].items()})
    for k in ("P_dyn_mW", "E_lyn_mJ"):
        print(k, {a: round(m, 3) for a, (m, _) in ringkas(r, k).items()})
    for p in sorted(out.glob("Gambar_*.png")):
        print(" ", p.name)


if __name__ == "__main__":
    main()
