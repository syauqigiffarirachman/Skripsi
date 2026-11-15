# Ringkasan analisis tahap D

Dibuat 2026-11-15 21:31; rshunt = 0.1 Ohm, offset = -2.0 LSB; manifest tahap_d.txt; pemeriksaan: MEMENUHI STANDAR.

## Rerata (sd) per kelompok

| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi sign (waktu nyata) (ms) | 655.4 (0.071) | 202.2 (0.026) | 655.5 (0.089) | 202.2 (0.027) |
| Daya dinamis (mW) | 23.25 (0.62) | 5.302 (0.096) | 35.11 (0.74) | 23.05 (0.14) |
| Durasi tidur CPU (%) | 35.46 (0.02) | 92.99 (5.7e-05) | 0.382 (0.0024) | 64.72 (0.00065) |
| Arus dinamis (mA) | 4.689 (0.022) | 1.019 (0.015) | 7.093 (0.0068) | 4.465 (0.016) |
| Daya rata-rata (mW) | 82.45 (0.81) | 67.76 (0.13) | 94.28 (0.88) | 85.5 (0.23) |
| Arus rata-rata (mA) | 16.25 (0.043) | 13.08 (0.07) | 18.66 (0.028) | 16.53 (0.071) |
| Energi jendela (mJ) | 4986 (48) | 4084 (8.1) | 5697 (53) | 5154 (15) |
| Energi dinamis per layanan (mJ) | 23.83 (0.63) | 5.416 (0.098) | 23.32 (0.49) | 4.677 (0.03) |
| Muatan dinamis per layanan (mC) | 4.806 (0.023) | 1.041 (0.016) | 4.71 (0.0045) | 0.9062 (0.0031) |
| Tingkat layanan (%) | 100 (0) | 100 (0) | 15.04 (0.0045) | 49.25 (0.00045) |
| Throughput (sign/s) | 0.9757 (0.00025) | 0.979 (0.00013) | 1.506 (0.00021) | 4.927 (0.00067) |
| Tegangan bus (V) | 5.08 (0.05) | 5.181 (0.034) | 5.052 (0.048) | 5.173 (0.034) |

## Uji beda (metrik utama, koreksi Holm per skenario)

| Skenario | Metrik | Uji | p | p Holm | Hedges g | Keputusan |
|---|---|---|---|---|---|---|
| Normal | lat_mean_ms_nyata | Welch t | 2.37e-138 | 7.12e-138 | 8350.8 | tolak H0 |
| Normal | P_dyn_mW | Mann-Whitney U | 3.02e-11 | 6.04e-11 | 39.7 | tolak H0 |
| Normal | S_cpu_log | Mann-Whitney U | 3.02e-11 | 6.04e-11 | -4087.2 | tolak H0 |
| Barrage | lat_mean_ms_nyata | Welch t | 8.7e-127 | 2.61e-126 | 6796.6 | tolak H0 |
| Barrage | P_dyn_mW | Mann-Whitney U | 3.02e-11 | 6.04e-11 | 22.3 | tolak H0 |
| Barrage | S_cpu_log | Mann-Whitney U | 3.02e-11 | 6.04e-11 | -35644.0 | tolak H0 |

## Gain (software -> DS28E38)

| skenario | G_latensi_pct | G_Pdyn_pct | G_Idyn_pct | G_Pavg_pct | G_Elyn_pct | G_Qlyn_pct | dS_cpu_poin | dTL_poin | rasio_Theta |
|---|---|---|---|---|---|---|---|---|---|
| Normal | 69.156 | 77.195 | 78.266 | 17.823 | 77.270 | 78.338 | 57.533 | 0.000 | 1.003 |
| Barrage | 69.157 | 34.363 | 37.056 | 9.316 | 79.940 | 80.763 | 64.335 | 34.212 | 3.272 |

## ANOVA dua arah (klasik): porsi variasi dan interaksi

- lat_mean_ms_nyata: arsitektur F = 1.711e+09, p = 0, eta2p = 1.000; skenario F = 35.45, p = 2.9e-08, eta2p = 0.234; arsitektur:skenario F = 14.64, p = 0.00021, eta2p = 0.112
- P_dyn_mW: arsitektur F = 2.791e+04, p = 4.4e-140, eta2p = 0.996; skenario F = 2.716e+04, p = 2.1e-139, eta2p = 0.996; arsitektur:skenario F = 1072, p = 2e-60, eta2p = 0.902
- S_cpu_log: arsitektur F = 1.135e+09, p = 0, eta2p = 1.000; skenario F = 3.068e+08, p = 0, eta2p = 1.000; arsitektur:skenario F = 3.538e+06, p = 6e-262, eta2p = 1.000
- I_dyn_mA: arsitektur F = 1.161e+06, p = 7e-234, eta2p = 1.000; skenario F = 1.001e+06, p = 3.7e-230, eta2p = 1.000; arsitektur:skenario F = 3.174e+04, p = 2.7e-143, eta2p = 0.996
- E_lyn_mJ: arsitektur F = 6.276e+04, p = 2e-160, eta2p = 0.998; skenario F = 71.38, p = 9.8e-14, eta2p = 0.381; arsitektur:skenario F = 2.375, p = 0.13, eta2p = 0.020

Catatan: TL dan throughput praktis deterministik (sd ~ 0), dilaporkan deskriptif + validasi model, tidak diuji. Sebaran sangat kecil membuat p-value amat kecil; tekankan ukuran efek dan persen gain.