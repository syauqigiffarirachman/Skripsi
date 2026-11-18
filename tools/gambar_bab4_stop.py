"""Gambar Bab 4 varian Stop (deep sleep) yang belum dibuat oleh analisis_d.py (18 Nov 2026).

Membuat empat gambar sesuai laporan/panduan_grafik_bab4_2026-11-18.md:
  4.1  Jejak arus terhadap waktu (4 panel, satu run contoh per kelompok)
  4.7  Tingkat layanan terukur vs model utilisasi
  4.9  Arus diam IDLE awal/akhir tiap blok (bukti keadaan bersih)
  4.10 Distribusi frame permintaan hilang pada K4 (semua 41 run)

Pakai:  python tools/gambar_bab4_stop.py [--shunt 0.100] [--keluar data/analisis/stop_final/gambar_bab4]
Data:   data/analisis/stop_final/{runs.csv,layanan.csv}, data/runs/<sesi>/<berkas> (jejak arus mentah).
Konvensi: warna menurut arsitektur (Software biru, DS28E38 oranye), Barrage berarsir, desimal koma, 300 dpi.
Nilai mutlak arus memakai shunt nominal 0,100 Ohm; jalankan ulang dengan --shunt setelah kalibrasi.
"""
import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402
from matplotlib.ticker import FuncFormatter  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
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


def baca_jejak(path, shunt):
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
    i_ma = (np.array([b for _, b in s]) + 2) * 2.5e-6 / shunt * 1000.0
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
def gambar_41(out, shunt, i_idle):
    fig, axs = plt.subplots(2, 2, figsize=(11, 6.4), sharex=True, sharey=True)
    pos = {"K1": (0, 0), "K2": (0, 1), "K3": (1, 0), "K4": (1, 1)}
    judul = {"K1": "(a) K1 Software, normal", "K2": "(b) K2 DS28E38, normal",
             "K3": "(c) K3 Software, barrage", "K4": "(d) K4 DS28E38, barrage"}
    A, B = 10.0, 13.0
    for k, rel in CONTOH.items():
        ax = axs[pos[k]]
        t, i, ev = baca_jejak(ROOT / "data" / "runs" / rel, shunt)
        for a, b in selang_aktif(ev, A, B):
            ax.axvspan(a, b, color="#d9d9d9", lw=0)
        m = (t >= A) & (t <= B)
        ax.plot(t[m], i[m], color=WARNA[k], lw=0.9)
        ax.axhline(i_idle, color="k", ls="--", lw=0.8)
        ax.set_title(judul[k], loc="left")
        ax.set_xlim(A, B)
        ax.set_ylim(0, 21)
        ax.xaxis.set_major_formatter(FMT)
        ax.yaxis.set_major_formatter(FMT)
        if False:                                 # inset 0,5 s untuk pulsa pendek
            ins = ax.inset_axes([0.60, 0.42, 0.37, 0.48])
            a2, b2 = 10.0, 10.5
            for a, b in selang_aktif(ev, a2, b2):
                ins.axvspan(a, b, color="#d9d9d9", lw=0)
            mm = (t >= a2) & (t <= b2)
            ins.plot(t[mm], i[mm], color=WARNA[k], lw=0.8)
            ins.axhline(i_idle, color="k", ls="--", lw=0.6)
            ins.set_xlim(a2, b2)
            ins.set_ylim(0, 16)
            ins.tick_params(labelsize=7)
            ins.xaxis.set_major_formatter(FMT)
            ins.yaxis.set_major_formatter(FMT)
            ins.set_title("rinci 0,5 s", fontsize=7, pad=2)
            ax.indicate_inset_zoom(ins, edgecolor="0.4")
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


def gambar_41_rinci(out, shunt, i_idle):
    """Gambar 4.1b: rinci pulsa aktif. (a) K2 satu permintaan (dua pulsa), (b) K4 pulsa berselang-seling 0,5 s."""
    fig, axs = plt.subplots(1, 2, figsize=(11, 3.8), sharey=True)
    for ax, k, (a, b), jd in [(axs[0], "K2", (10.9, 11.3), "(a) K2 DS28E38, normal: satu permintaan"),
                              (axs[1], "K4", (10.0, 10.5), "(b) K4 DS28E38, barrage: 0,5 s")]:
        t, i, ev = baca_jejak(ROOT / "data" / "runs" / CONTOH[k], shunt)
        for x, y in selang_aktif(ev, a, b):
            ax.axvspan(x, y, color="#d9d9d9", lw=0)
        m = (t >= a) & (t <= b)
        ax.plot(t[m], i[m], color=WARNA[k], lw=1.0, marker=".", ms=2.5)
        ax.axhline(i_idle, color="k", ls="--", lw=0.8)
        ax.set_xlim(a, b)
        ax.set_ylim(0, 21)
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
        ax.text(n, 1.935, f"selisih {(f'{v[-1] - v[0]:+.3f}').replace('.', ',')} mA", ha="center", fontsize=8, color="0.3")
    rata = idle["I_avg_mA"].mean()
    ax.axhspan(rata - 0.05, rata + 0.05, color="#e8e8e8", lw=0, zorder=0)
    ax.axhline(rata, color="0.5", lw=0.8, ls=":")
    ax.set_xticks(range(4))
    ax.set_xticklabels([nama_blok[f"blok{i}"] for i in range(1, 5)])
    ax.set_xlim(-0.6, 3.6)
    ax.set_ylim(1.93, 2.05)
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


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--shunt", type=float, default=0.100)
    ap.add_argument("--keluar", default=str(ANA / "gambar_bab4"))
    a = ap.parse_args()
    out = Path(a.keluar)
    out.mkdir(parents=True, exist_ok=True)
    gaya()
    r = pd.read_csv(ANA / "runs.csv")
    i_idle = float(r[r.scen == "IDLE"]["I_avg_mA"].mean())
    gambar_41(out, a.shunt, i_idle)
    gambar_41_rinci(out, a.shunt, i_idle)
    gambar_47(out)
    idle = gambar_49(out)
    k4 = gambar_410(out)
    print("keluar:", out)
    for f in sorted(out.glob("*.png")):
        print(" ", f.name)
    print("IDLE rerata", round(i_idle, 3), "mA; K4 total", len(k4), "run, sah", int(k4.sah.sum()))


if __name__ == "__main__":
    main()
