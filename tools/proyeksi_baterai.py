"""C7: proyeksi pemakaian baterai dari data final Stop (ilustrasi, bukan umur baterai terukur).

Dua ukuran yang tidak bergantung pada papan uji:
 (a) tanda tangan per baterai (hanya energi dinamis): N = E_batere / E_lyn
 (b) umur pada beban kerja Normal (1 permintaan/s) = kapasitas / (I_idle + I_dyn)  [dengan lantai papan ~2,45 mA terkalibrasi]
     dan versi 'tanpa lantai' = kapasitas / I_dyn (batas atas; lantai LED papan ini bukan sifat chip).
Asumsi ilustratif: baterai 1000 mAh x 3,7 V = 13,32 kJ; konversi regulator diabaikan; energi diukur di input 5,2 V.
Membaca data/analisis/stop_final/runs.csv, jadi nilainya mengikuti kalibrasi yang dipakai analisis_d.py saat runs.csv dibuat
(21 Nov 2026: rshunt 0,0821 Ohm, offset -2,83). Jalankan analisis_d.py dulu bila kalibrasi berubah, baru skrip ini.
"""
from pathlib import Path
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
D = ROOT / "data" / "analisis" / "stop_final"
CAP_MAH, V_BAT = 1000.0, 3.7
E_BAT_J = CAP_MAH * 1e-3 * 3600 * V_BAT

r = pd.read_csv(D / "runs.csv")
r = r[r.sah & r.kelompok.isin(["K1", "K2", "K3", "K4"])]
g = r.groupby("kelompok").agg(E_lyn=("E_lyn_mJ", "mean"), I_idle=("I_idle_mA", "mean"), I_dyn=("I_dyn_mA", "mean"),
                              I_avg=("I_avg_mA", "mean"), TL=("TL_pct", "mean"), Theta=("Theta_sps", "mean"))
g["tanda_tangan_per_baterai"] = E_BAT_J / (g.E_lyn * 1e-3)
g["umur_jam_dgn_lantai"] = CAP_MAH / g.I_avg
g["umur_jam_tanpa_lantai"] = CAP_MAH / g.I_dyn
g["umur_hari_dgn_lantai"] = g.umur_jam_dgn_lantai / 24
g.to_csv(D / "proyeksi_baterai.csv")
n = {"K1": "K1 Software, normal", "K2": "K2 DS28E38, normal", "K3": "K3 Software, barrage", "K4": "K4 DS28E38, barrage"}
L = ["# Proyeksi pemakaian baterai (ilustrasi)\n",
     f"Asumsi: baterai {CAP_MAH:.0f} mAh x {V_BAT} V = {E_BAT_J / 1e3:.2f} kJ; regulator diabaikan; arus diukur di input 5,2 V. "
     "Lantai papan (LED daya dkk.) ~2,45 mA terkalibrasi (1,99 mA pada shunt nominal 0,100 Ohm) ikut pada kolom 'dengan lantai'; kolom 'tanpa lantai' hanya arus dinamis (batas atas, "
     "karena lantai chip sebenarnya 1-20 uA). Ini proyeksi dari data pengukuran, bukan umur baterai terukur.\n",
     "| Kelompok | E per layanan (mJ) | Tanda tangan per baterai (ribu) | Arus rerata (mA) | Umur dengan lantai (jam) | Umur tanpa lantai (jam) | Tingkat layanan (%) |",
     "|---|---|---|---|---|---|---|"]
for k, x in g.iterrows():
    L.append(f"| {n[k]} | {x.E_lyn:.2f} | {x.tanda_tangan_per_baterai / 1e3:.0f} | {x.I_avg:.2f} | {x.umur_jam_dgn_lantai:.0f} | "
             f"{x.umur_jam_tanpa_lantai:.0f} | {x.TL:.1f} |")
L.append("")
L.append("Catatan: pada barrage kedua arsitektur melayani jumlah permintaan berbeda (K3 15 %, K4 85,5 %), sehingga umur per baterai K3 vs K4 "
         "tidak setara pekerjaan; bandingkan lewat kolom 'tanda tangan per baterai'.")
(D / "proyeksi_baterai.md").write_text("\n".join(L) + "\n", encoding="utf-8")
print("\n".join(L))
