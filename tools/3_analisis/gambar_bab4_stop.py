"""Gambar Bab 4 varian Stop (deep sleep) yang belum dibuat oleh analisis_d.py (18 Nov 2026).

Membuat SEMUA gambar Bab 4 (4.1, 4.1b, 4.2 sampai 4.11) dengan gaya seragam sesuai laporan/2_hasil_dan_pembahasan/panduan_grafik_bab4_2026-11-18.md:
  4.1  Jejak arus terhadap waktu (4 panel, satu run contoh per kelompok)
  4.2  Latensi tanda tangan (batang bernilai)
  4.7  Tingkat layanan terukur vs model utilisasi
  4.8  Throughput terukur dan garis batas kapasitas
  4.9  Arus diam IDLE awal/akhir tiap blok (bukti keadaan bersih)
  4.10 Distribusi frame permintaan hilang pada K4 (semua 41 run)

Pakai:  python tools/3_analisis/gambar_bab4_stop.py [--shunt 0.0821] [--keluar data/analisis/stop_final/gambar_bab4]
Data:   data/analisis/stop_final/{runs.csv,layanan.csv}, data/runs/<sesi>/<berkas> (jejak arus mentah).
Konvensi: warna menurut arsitektur (Software biru, DS28E38 oranye), Barrage berarsir, desimal koma, 300 dpi.
Nilai mutlak arus pada jejak (Gambar 4.1/4.1b) memakai --shunt dan --offset; bawaan = hasil kalibrasi 21 Nov 2026
(0,0821 Ohm, offset -2,83; tools/2_kalibrasi/kalibrasi_param.py). Gambar lain mengikuti data/analisis/stop_final/ (jalankan
analisis_d.py dengan kalibrasi yang sama dulu). Sumbu Gambar 4.4/4.5 mengikuti data (tidak tetap).
"""
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import sys
from functools import lru_cache
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402
from matplotlib.ticker import FuncFormatter  # noqa: E402

import kalibrasi_param as kp  # noqa: E402   # default --shunt/--offset = kalibrasi 21 Nov 2026
ANA = ROOT / "data" / "analisis" / "stop_final"
TICK_HZ = 72_000_000
C_SW, C_HW = "#1f5fa8", "#e07b00"          # Software, DS28E38
NAMA = {"K1": "K1\nSoftware\nnormal", "K2": "K2\nDS28E38\nnormal", "K3": "K3\nSoftware\nbarrage", "K4": "K4\nDS28E38\nbarrage"}
WARNA = {"K1": C_SW, "K2": C_HW, "K3": C_SW, "K4": C_HW}
ARSIR = {"K1": "", "K2": "", "K3": "//", "K4": "//"}

# run contoh Gambar 4.1: paling dekat median arus rata-rata kelompoknya (lihat panduan)
CONTOH = {
    "K1": "20261118_124646_K1S/031_K1S_A.csv",
    "K2": "20261118_115044_K2S/033_K2S_A.csv",
    "K3": "20261118_124646_K1S/007_K1S_B.csv",
    "K4": "20261118_115044_K2S/024_K2S_B.csv",
}


def koma(x, _pos=None):
    s = f"{x:.10g}"
    return s.replace(".", ",")


FMT = FuncFormatter(koma)


def gaya():
    plt.rcParams.update({"font.size": 10, "axes.titlesize": 10, "axes.labelsize": 10, "legend.fontsize": 9,
                         "figure.dpi": 100, "savefig.dpi": 300, "axes.spines.top": False,
                         "axes.spines.right": False, "font.family": "DejaVu Sans"})


@lru_cache(maxsize=None)     # Gambar 4.1 dan 4.1b (serta pencarian puncak) memakai berkas yang sama: baca sekali
def baca_jejak(path, shunt, offset=kp.OFFSET_LSB):
    """Jejak satu run: (waktu detik, arus mA, daftar tepi pin status). Hasil di-cache; jangan diubah di tempat."""
    s, e, t0 = [], [], None
    for ln in Path(path).read_text(errors="replace").splitlines():
        if ln.startswith("s,"):
            p = ln.split(",")
            if len(p) == 4:
                s.append((int(p[1]), int(p[2])))
        elif ln.startswith("e,"):
            p = ln.split(",")
            e.append((int(p[1]), int(p[2])))
        elif ln.startswith("h,") and ">START" in ln and t0 is None:
            t0 = int(ln.split(",")[1])
    t = (np.array([a for a, _ in s]) - t0) / TICK_HZ
    i_ma = (np.array([b for _, b in s]) - offset) * kp.SHUNT_LSB_V / shunt * 1000.0     # sama dengan run_bench.analyse
    ev = [((a - t0) / TICK_HZ, lv) for a, lv in e]
    return t, i_ma, ev


def selang_aktif(ev, a, b):
    """Selang [mulai, akhir] pin status = 1 yang beririsan dengan [a, b]."""
    lv, t_prev = 0, a
    for t, l in ev:                                  # level pada waktu a = level tepi terakhir sebelum a
        if t <= a:
            lv = l
    sp = []
    mulai = a if lv == 1 else None
    for t, l in ev:
        if t <= a or t >= b:
            continue
        if l == 1 and mulai is None:
            mulai = t
        elif l == 0 and mulai is not None:
            sp.append((mulai, t))
            mulai = None
    if mulai is not None:
        sp.append((mulai, b))
    return sp


# --------------------------------------------------------------------------- 4.1
def _ymax_jejak(shunt, offset):
    """Batas atas sumbu arus: puncak terbesar run contoh + 8 %, dibulatkan ke atas kelipatan 2 mA (bukan 21 mA tetap
    seperti pada skala shunt nominal; puncak K2/K4 terkalibrasi ~23,8 mA)."""
    puncak = max(float(baca_jejak(ROOT / "data" / "runs" / rel, shunt, offset)[1].max()) for rel in CONTOH.values())
    return float(np.ceil(puncak * 1.08 / 2.0) * 2.0)


def gambar_41(out, shunt, i_idle, offset=kp.OFFSET_LSB):
    fig, axs = plt.subplots(2, 2, figsize=(11, 6.4), sharex=True, sharey=True)
    pos = {"K1": (0, 0), "K2": (0, 1), "K3": (1, 0), "K4": (1, 1)}
    judul = {"K1": "(a) K1 Software, normal", "K2": "(b) K2 DS28E38, normal",
             "K3": "(c) K3 Software, barrage", "K4": "(d) K4 DS28E38, barrage"}
    A, B = 10.0, 13.0
    ymax_j = _ymax_jejak(shunt, offset)
    for k, rel in CONTOH.items():
        ax = axs[pos[k]]
        t, i, ev = baca_jejak(ROOT / "data" / "runs" / rel, shunt, offset)
        for a, b in selang_aktif(ev, A, B):
            ax.axvspan(a, b, color="#d9d9d9", lw=0)
        m = (t >= A) & (t <= B)
        ax.plot(t[m], i[m], color=WARNA[k], lw=0.9)
        ax.axhline(i_idle, color="k", ls="--", lw=0.8)
        ax.set_title(judul[k], loc="left")
        ax.set_xlim(A, B)
        ax.set_ylim(0, ymax_j)
        ax.xaxis.set_major_formatter(FMT)
        ax.yaxis.set_major_formatter(FMT)
    for ax in axs[1]:
        ax.set_xlabel("Waktu sejak awal jendela (s)")
    for ax in axs[:, 0]:
        ax.set_ylabel("Arus host (mA)")
    fig.legend(handles=[Patch(facecolor="#d9d9d9", label="CPU aktif (pin status = 1)"),
                        plt.Line2D([0], [0], color="k", ls="--", lw=0.8, label=f"arus diam ({koma(round(i_idle, 2))} mA)")],
               loc="lower center", ncol=2, frameon=False, bbox_to_anchor=(0.5, -0.01))
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(out / "gambar_4_1_jejak_arus.png", bbox_inches="tight")
    plt.close(fig)


def gambar_41_rinci(out, shunt, i_idle, offset=kp.OFFSET_LSB):
    """Gambar 4.1b: rinci pulsa aktif. (a) K2 satu permintaan (dua pulsa), (b) K4 pulsa berselang-seling 0,5 s."""
    fig, axs = plt.subplots(1, 2, figsize=(11, 3.8), sharey=True)
    ymax_j = _ymax_jejak(shunt, offset)
    for ax, k, (a, b), jd in [(axs[0], "K2", (10.9, 11.3), "(a) K2 DS28E38, normal: satu permintaan"),
                              (axs[1], "K4", (10.0, 10.5), "(b) K4 DS28E38, barrage: 0,5 s")]:
        t, i, ev = baca_jejak(ROOT / "data" / "runs" / CONTOH[k], shunt, offset)
        for x, y in selang_aktif(ev, a, b):
            ax.axvspan(x, y, color="#d9d9d9", lw=0)
        m = (t >= a) & (t <= b)
        ax.plot(t[m], i[m], color=WARNA[k], lw=1.0, marker=".", ms=2.5)
        ax.axhline(i_idle, color="k", ls="--", lw=0.8)
        ax.set_xlim(a, b)
        ax.set_ylim(0, ymax_j)
        ax.set_title(jd, loc="left")
        ax.set_xlabel("Waktu sejak awal jendela (s)")
        ax.xaxis.set_major_formatter(FMT)
        ax.yaxis.set_major_formatter(FMT)
    axs[0].set_ylabel("Arus host (mA)")
    fig.legend(handles=[Patch(facecolor="#d9d9d9", label="CPU aktif (pin status = 1)"),
                        plt.Line2D([0], [0], color="k", ls="--", lw=0.8, label=f"arus diam ({koma(round(i_idle, 2))} mA)")],
               loc="lower center", ncol=2, frameon=False, bbox_to_anchor=(0.5, -0.02))
    fig.tight_layout(rect=(0, 0.06, 1, 1))
    fig.savefig(out / "gambar_4_1b_jejak_arus_rinci.png", bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------- 4.7
def gambar_47(out):
    lay = pd.read_csv(ANA / "layanan.csv").set_index("kelompok")
    fig, ax = plt.subplots(figsize=(7.4, 4.4))
    lebar = 0.36
    for n, k in enumerate(["K1", "K2", "K3", "K4"]):
        ter, mod = lay.loc[k, "TL_rerata"], lay.loc[k, "TL_model"]
        ax.bar(n - lebar / 2, ter, lebar, color=WARNA[k], hatch=ARSIR[k], edgecolor="white" if not ARSIR[k] else "white",
               label=None)
        ax.bar(n + lebar / 2, mod, lebar, facecolor="white", edgecolor=WARNA[k], hatch=ARSIR[k], lw=1.4)
        ax.text(n - lebar / 2, ter + 1.5, f"{ter:.2f}".replace(".", ","), ha="center", fontsize=8.5)
        ax.text(n + lebar / 2, mod + 1.5, f"{mod:.2f}".replace(".", ","), ha="center", fontsize=8.5)
    ax.set_xticks(range(4))
    ax.set_xticklabels([NAMA[k] for k in ["K1", "K2", "K3", "K4"]])
    ax.set_ylim(0, 112)
    ax.set_ylabel("Tingkat layanan (% permintaan terlayani)")
    ax.yaxis.set_major_formatter(FMT)
    ax.legend(handles=[Patch(facecolor="0.4", label="Terukur"),
                       Patch(facecolor="white", edgecolor="0.4", lw=1.4, label="Model utilisasi"),
                       Patch(facecolor=C_SW, label="Software"), Patch(facecolor=C_HW, label="DS28E38")],
              ncol=4, loc="upper center", bbox_to_anchor=(0.5, 1.14), frameon=False, fontsize=8.5)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_7_tingkat_layanan_vs_model.png", bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------- 4.9
def gambar_49(out):
    r = pd.read_csv(ANA / "runs.csv")
    idle = r[r.scen == "IDLE"].sort_values(["blok", "urut"])
    nama_blok = {"blok1": "Blok 1\nK2S", "blok2": "Blok 2\nK1S", "blok3": "Blok 3\nK1S", "blok4": "Blok 4\nK2S"}
    rata = idle["I_avg_mA"].mean()                    # jendela sumbu mengikuti rerata IDLE (bukan nilai tetap 1,99 mA lama)
    fig, ax = plt.subplots(figsize=(7.4, 4.2))
    for n, (b, g) in enumerate(idle.groupby("blok")):
        g = g.sort_values("urut")
        v = g["I_avg_mA"].to_numpy()
        warna = C_HW if g["arch"].iloc[0] == "K2" else C_SW
        ax.plot([n - 0.18, n + 0.18], v, "-", color=warna, lw=1.2)
        ax.plot(n - 0.18, v[0], "o", color=warna, ms=7, mfc="white")
        ax.plot(n + 0.18, v[-1], "o", color=warna, ms=7)
        ax.text(n - 0.18, v[0] + 0.004, f"{v[0]:.3f}".replace(".", ","), ha="center", fontsize=8)
        ax.text(n + 0.18, v[-1] - 0.009, f"{v[-1]:.3f}".replace(".", ","), ha="center", fontsize=8)
        ax.text(n, rata - 0.055, f"selisih {(f'{v[-1] - v[0]:+.3f}').replace('.', ',')} mA", ha="center", fontsize=8, color="0.3")
    ax.axhspan(rata - 0.05, rata + 0.05, color="#e8e8e8", lw=0, zorder=0)
    ax.axhline(rata, color="0.5", lw=0.8, ls=":")
    ax.set_xticks(range(4))
    ax.set_xticklabels([nama_blok[f"blok{i}"] for i in range(1, 5)])
    ax.set_xlim(-0.6, 3.6)
    ax.set_ylim(rata - 0.06, rata + 0.06)
    ax.set_ylabel("Arus diam (mA)  [sumbu dipersempit]")
    ax.yaxis.set_major_formatter(FMT)
    ax.legend(handles=[plt.Line2D([0], [0], marker="o", mfc="white", mec="0.3", color="0.3", ls="", label="IDLE awal blok"),
                       plt.Line2D([0], [0], marker="o", color="0.3", ls="", label="IDLE akhir blok"),
                       Patch(facecolor="#e8e8e8", label="batas keabsahan ±0,05 mA"),
                       Patch(facecolor=C_SW, label="K1S Software"), Patch(facecolor=C_HW, label="K2S DS28E38")],
              ncol=3, loc="upper center", bbox_to_anchor=(0.5, 1.2), frameon=False, fontsize=8.5)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_9_arus_diam_antar_blok.png", bbox_inches="tight")
    plt.close(fig)
    return idle


# --------------------------------------------------------------------------- 4.10
def gambar_410(out):
    r = pd.read_csv(ANA / "runs.csv")
    k4 = r[r.kelompok == "K4"].copy()
    n_ok = k4[k4.sah].rx_lost.value_counts().reindex(range(5), fill_value=0)
    n_gg = k4[~k4.sah].rx_lost.value_counts().reindex(range(5), fill_value=0)
    fig, ax = plt.subplots(figsize=(7.0, 4.2))
    x = np.arange(5)
    ax.bar(x, n_ok, 0.6, color=C_HW, label="Run sah (dianalisis)")
    ax.bar(x, n_gg, 0.6, bottom=n_ok, color="white", edgecolor=C_HW, hatch="//", label="Run gugur (melampaui batas)")
    for i in range(5):
        tot = n_ok.iloc[i] + n_gg.iloc[i]
        if tot:
            ax.text(i, tot + 0.4, str(int(tot)), ha="center", fontsize=9)
    rata_dtg = k4[k4.sah].arrived.mean()
    batas = 0.0025 * rata_dtg
    ax.axvline(batas, color="k", ls="--", lw=1.0)
    ax.text(batas + 0.05, ax.get_ylim()[1] * 0.92, f"batas 0,25 %\n(±{koma(round(batas, 1))} frame)", fontsize=8.5, va="top")
    ax.set_xticks(x)
    ax.set_xlabel("Jumlah frame permintaan hilang per run")
    ax.set_ylabel("Jumlah run")
    ax.set_ylim(0, max(n_ok.max() + n_gg.max(), 20) + 3)
    ax.legend(frameon=False, loc="upper right", fontsize=8.5)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_10_frame_hilang_K4.png", bbox_inches="tight")
    plt.close(fig)
    return k4


def _legenda_arsitektur(ax, atas=1.14):
    ax.legend(handles=[Patch(facecolor=C_SW, label="Software"), Patch(facecolor=C_HW, label="DS28E38"),
                       Patch(facecolor="white", edgecolor="0.3", label="Normal"),
                       Patch(facecolor="white", edgecolor="0.3", hatch="//", label="Barrage")],
              ncol=4, loc="upper center", bbox_to_anchor=(0.5, atas), frameon=False, fontsize=8.5)


# --------------------------------------------------------------------------- 4.2
def gambar_42(out):
    d = pd.read_csv(ANA / "deskriptif.csv")
    d = d[d.metrik == "lat_mean_ms_nyata"].set_index("kelompok")
    fig, ax = plt.subplots(figsize=(7.2, 4.4))
    for n, k in enumerate(["K1", "K2", "K3", "K4"]):
        v = d.loc[k, "rerata"]
        ax.bar(n, v, 0.6, color=WARNA[k], hatch=ARSIR[k], edgecolor="white")
        ax.text(n, v + 12, f"{v:.2f}".replace(".", ","), ha="center", fontsize=9.5)
    ax.set_xticks(range(4))
    ax.set_xticklabels([NAMA[k] for k in ["K1", "K2", "K3", "K4"]])
    ax.set_ylim(0, 760)
    ax.set_ylabel("Latensi tanda tangan (ms)")
    ax.yaxis.set_major_formatter(FMT)
    _legenda_arsitektur(ax)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_2_latensi.png", bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------- 4.8
def gambar_48(out):
    lay = pd.read_csv(ANA / "layanan.csv").set_index("kelompok")
    fig, ax = plt.subplots(figsize=(7.2, 4.4))
    for n, k in enumerate(["K1", "K2", "K3", "K4"]):
        v = lay.loc[k, "Theta"]
        ax.bar(n, v, 0.6, color=WARNA[k], hatch=ARSIR[k], edgecolor="white")
        ax.text(n, v + 0.18, f"{v:.2f}".replace(".", ","), ha="center", fontsize=9.5)
    cap_sw = float(lay.loc["K1", "Theta_maks_model"])
    cap_hw = float(lay.loc["K2", "Theta_maks_model"])
    ax.hlines(cap_sw, -0.55, 3.55, colors=C_SW, linestyles="--", lw=1.3)
    ax.hlines(cap_hw, -0.55, 3.55, colors=C_HW, linestyles="--", lw=1.3)
    ax.text(3.6, cap_sw, "kapasitas Software " + f"{cap_sw:.2f}".replace(".", ","), color=C_SW, fontsize=8.5, va="center")
    ax.text(3.6, cap_hw, "kapasitas DS28E38 " + f"{cap_hw:.2f}".replace(".", ","), color=C_HW, fontsize=8.5, va="center")
    ax.set_xlim(-0.6, 5.3)
    ax.set_xticks(range(4))
    ax.set_xticklabels([NAMA[k] for k in ["K1", "K2", "K3", "K4"]])
    ax.set_ylim(0, 10.2)
    ax.set_ylabel("Throughput (tanda tangan/s)")
    ax.yaxis.set_major_formatter(FMT)
    _legenda_arsitektur(ax)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_8_throughput_vs_batas.png", bbox_inches="tight")
    plt.close(fig)


def _dua_desimal(v):
    return f"{v:.2f}".replace(".", ",")


def _batang(ax, nilai, galat=None, fmt=_dua_desimal, ofs=0.0):
    for n, k in enumerate(["K1", "K2", "K3", "K4"]):
        v = nilai[k]
        ax.bar(n, v, 0.6, color=WARNA[k], hatch=ARSIR[k], edgecolor="white")
        if galat is not None:
            ax.errorbar(n, v, yerr=galat[k], color="0.15", capsize=4, lw=1.1, capthick=1.1)
        y_teks = v + (galat[k] if galat is not None else 0) + ofs
        ax.text(n, y_teks, fmt(v), ha="center", fontsize=9.5)
    ax.set_xticks(range(4))
    ax.set_xticklabels([NAMA[k] for k in ["K1", "K2", "K3", "K4"]])
    ax.yaxis.set_major_formatter(FMT)


def _ambil(metrik, kolom="rerata"):
    d = pd.read_csv(ANA / "deskriptif.csv")
    d = d[d.metrik == metrik].set_index("kelompok")
    return {k: float(d.loc[k, kolom]) for k in ["K1", "K2", "K3", "K4"]}


def _ugab(metrik):
    u = pd.read_csv(ANA / "ketidakpastian.csv")
    u = u[u.metrik == metrik].set_index("kelompok")
    return {k: float(u.loc[k, "U_gab"]) for k in ["K1", "K2", "K3", "K4"]}


# --------------------------------------------------------------------------- 4.3
def gambar_43(out):
    v = _ambil("S_cpu_log")
    fig, ax = plt.subplots(figsize=(7.2, 4.4))
    _batang(ax, v, ofs=1.2)
    ax.set_ylim(0, 112)
    ax.set_ylabel("Durasi tidur dalam CPU (% dari jendela 60 s)")
    _legenda_arsitektur(ax)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_3_durasi_tidur_cpu.png", bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------- 4.4 dan 4.5
def _gambar_dengan_galat(out, metrik, label_y, ymax, berkas, ofs):
    v = _ambil(metrik)
    g = _ugab(metrik)
    if ymax is None:                                  # sumbu mengikuti data (batang + galat + ruang label), bukan nilai tetap
        ymax = max(v[k] + g[k] for k in v) * 1.14
    if ofs is None:
        ofs = 0.012 * ymax
    fig, ax = plt.subplots(figsize=(7.2, 4.4))
    _batang(ax, v, galat=g, ofs=ofs)
    ax.set_ylim(0, ymax)
    ax.set_ylabel(label_y)
    _legenda_arsitektur(ax)
    ax.text(0.5, -0.30, f"Batang galat = ketidakpastian gabungan (didominasi kalibrasi shunt ±{koma(round(kp.U_REL_KAL * 100, 2))} %)",
            transform=ax.transAxes, ha="center", fontsize=8, color="0.35")
    fig.tight_layout()
    fig.savefig(out / berkas, bbox_inches="tight")
    plt.close(fig)


def gambar_44(out):
    _gambar_dengan_galat(out, "P_dyn_mW", "Daya dinamis (mW)", None, "gambar_4_4_daya_dinamis.png", None)


def gambar_45(out):
    _gambar_dengan_galat(out, "E_lyn_mJ", "Energi dinamis per permintaan terlayani (mJ)", None,
                         "gambar_4_5_energi_per_permintaan.png", None)


# --------------------------------------------------------------------------- 4.6
def gambar_46(out):
    dd = pd.read_csv(ANA / "interaksi_dd.csv").set_index("metrik")
    fig, axs = plt.subplots(1, 2, figsize=(10.4, 4.3))
    spes = [("S_cpu_log", "(a) Durasi tidur dalam CPU", "Durasi tidur dalam CPU (%)", 112, "poin"),
            ("P_dyn_mW", "(b) Daya dinamis", "Daya dinamis (mW)", None, "mW")]
    for ax, (m, jd, yl, ymax, sat) in zip(axs, spes):
        v = _ambil(m)
        if ymax is None:                              # mengikuti data (daya dinamis tergantung kalibrasi shunt)
            ymax = max(v.values()) * 1.15
        for arsi, (kn, kb), c, mk in [("Software", ("K1", "K3"), C_SW, "o"), ("DS28E38", ("K2", "K4"), C_HW, "s")]:
            ax.plot([0, 1], [v[kn], v[kb]], "-" + mk, color=c, lw=2, ms=8, label=arsi)
            ax.text(-0.05, v[kn], _dua_desimal(v[kn]), color=c, fontsize=9.5, ha="right", va="center")
            ax.text(1.05, v[kb] + (ymax * 0.03 if v[kb] < 5 else 0), _dua_desimal(v[kb]), color=c, fontsize=9.5, ha="left",
                    va="center")
        d = dd.loc[m]
        ax.text(0.98, 0.97 if m == "S_cpu_log" else 0.04, "interaksi DD = " + f"{d.DD:+.2f}".replace(".", ",") + " " + sat +
                "  [" + f"{d.IK95_bawah:+.2f}".replace(".", ",") + "; " + f"{d.IK95_atas:+.2f}".replace(".", ",") + "]",
                transform=ax.transAxes, ha="right", va="top" if m == "S_cpu_log" else "bottom", fontsize=8.5, color="0.25")
        ax.set_xticks([0, 1])
        ax.set_xticklabels(["Normal", "Barrage"])
        ax.set_xlim(-0.4, 1.4)
        ax.set_ylim(0, ymax)
        ax.set_title(jd, loc="left")
        ax.set_ylabel(yl)
        ax.yaxis.set_major_formatter(FMT)
        ax.legend(frameon=False, loc="upper left" if m == "P_dyn_mW" else "center", fontsize=9)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_6_interaksi.png", bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------- 4.11
def gambar_411(out):
    r = pd.read_csv(ANA / "runs.csv")
    s = r[r.sah & r.kelompok.isin(["K1", "K2", "K3", "K4"])]
    ba = pd.read_csv(ANA / "bland_altman.csv").iloc[0]
    fig, ax = plt.subplots(figsize=(7.4, 4.4))
    rng = np.random.default_rng(1)
    for k in ["K1", "K2", "K3", "K4"]:
        g = s[s.kelompok == k]
        x = (g.S_cpu_log + g.S_cpu_host) / 2
        y = g.S_cpu_log - g.S_cpu_host
        ax.scatter(x + rng.normal(0, 0.0, len(x)), y, s=22, color=WARNA[k], marker="o" if not ARSIR[k] else "^",
                   alpha=0.85, label=NAMA[k].replace("\n", " "), edgecolor="white", lw=0.4)
    ax.axhline(ba.bias, color="k", lw=1.2)
    ax.axhline(ba.LoA_bawah, color="k", lw=1.0, ls="--")
    ax.axhline(ba.LoA_atas, color="k", lw=1.0, ls="--")
    x1 = ax.get_xlim()[1]
    for yv, tx in [(ba.bias, "bias " + f"{ba.bias:+.3f}".replace(".", ",")),
                   (ba.LoA_atas, "+1,96 sd " + f"{ba.LoA_atas:+.3f}".replace(".", ",")),
                   (ba.LoA_bawah, "-1,96 sd " + f"{ba.LoA_bawah:+.3f}".replace(".", ","))]:
        ax.text(x1, yv, " " + tx, fontsize=8.5, va="center")
    ax.set_xlim(-2, 104)
    ax.set_ylim(-0.1, 0.1)
    ax.set_xlabel("Rerata kedua cara (% tidur CPU)")
    ax.set_ylabel("Selisih pin − firmware (poin persentase)")
    ax.yaxis.set_major_formatter(FMT)
    ax.legend(ncol=4, loc="upper center", bbox_to_anchor=(0.5, 1.16), frameon=False, fontsize=7.8, handletextpad=0.2,
              columnspacing=0.8)
    fig.tight_layout()
    fig.savefig(out / "gambar_4_11_bland_altman_scpu.png", bbox_inches="tight")
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--shunt", type=float, default=kp.RSHUNT_EFEKTIF,
                    help="hambatan shunt efektif (Ohm); bawaan = kalibrasi 21 Nov 2026")
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB, help="offset nol (satuan 2,5 uV)")
    ap.add_argument("--keluar", default=str(ANA / "gambar_bab4"))
    a = ap.parse_args()
    out = Path(a.keluar)
    out.mkdir(parents=True, exist_ok=True)
    gaya()
    r = pd.read_csv(ANA / "runs.csv")
    i_idle = float(r[r.scen == "IDLE"]["I_avg_mA"].mean())
    gambar_41(out, a.shunt, i_idle, a.offset)
    gambar_41_rinci(out, a.shunt, i_idle, a.offset)
    gambar_42(out)
    gambar_43(out)
    gambar_44(out)
    gambar_45(out)
    gambar_46(out)
    gambar_411(out)
    gambar_47(out)
    gambar_48(out)
    idle = gambar_49(out)
    k4 = gambar_410(out)
    print("keluar:", out)
    for f in sorted(out.glob("*.png")):
        print(" ", f.name)
    print("IDLE rerata", round(i_idle, 3), "mA; K4 total", len(k4), "run, sah", int(k4.sah.sum()))


if __name__ == "__main__":
    main()
