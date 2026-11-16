# Ringkasan analisis tahap D

Dibuat 2026-11-16 01:25; rshunt = 0.1 Ohm, offset = -2.0 LSB, u_rel kalibrasi = 5.0%; bootstrap 10000 resampel (seed 20261116); keutuhan data: MEMENUHI STANDAR.

## 1. Deskriptif: rerata (sd) [min-maks]

| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi sign (waktu nyata) (ms) | 655.4 (0.071) [655.27-655.56] | 202.15 (0.026) [202.11-202.2] | 655.51 (0.089) [655.33-655.7] | 202.17 (0.027) [202.13-202.23] |
| Daya dinamis (mW) | 23.25 (0.62) [22.376-24.448] | 5.3021 (0.096) [5.0219-5.4783] | 35.112 (0.74) [34.123-36.779] | 23.047 (0.14) [22.693-23.25] |
| Durasi tidur CPU (%) | 35.458 (0.02) [35.406-35.478] | 92.991 (5.7e-05) [92.991-92.991] | 0.382 (0.0024) [0.37802-0.39273] | 64.717 (0.00065) [64.715-64.718] |
| Energi per permintaan terlayani (mJ) | 23.828 (0.63) [22.934-25.041] | 5.4161 (0.098) [5.1293-5.5966] | 23.317 (0.49) [22.66-24.42] | 4.6774 (0.03) [4.6053-4.7199] |
| Arus dinamis (mA) | 4.6892 (0.022) [4.6388-4.7289] | 1.0191 (0.015) [0.99387-1.0578] | 7.0934 (0.0068) [7.0757-7.1055] | 4.4649 (0.016) [4.4394-4.4957] |
| Muatan dinamis per layanan (mC) | 4.8057 (0.023) [4.7546-4.847] | 1.041 (0.016) [1.0154-1.0807] | 4.7105 (0.0045) [4.6986-4.7198] | 0.90615 (0.0031) [0.90111-0.91245] |
| Daya rata-rata (mW) | 82.455 (0.81) [81.459-84.166] | 67.759 (0.13) [67.395-67.935] | 94.279 (0.88) [93.206-96.497] | 85.496 (0.23) [85.066-85.833] |
| Arus rata-rata (mA) | 16.252 (0.043) [16.157-16.329] | 13.08 (0.07) [12.982-13.166] | 18.662 (0.028) [18.586-18.703] | 16.529 (0.071) [16.428-16.604] |
| Energi jendela (mJ) | 4985.8 (48) [4925.8-5086.3] | 4083.7 (8.1) [4061.3-4094.5] | 5697.2 (53) [5632.5-5832] | 5154 (15) [5127.1-5175.1] |
| Tingkat layanan (%) | 100 (0) [100-100] | 100 (0) [100-100] | 15.042 (0.0045) [15.041-15.066] | 49.254 (0.00045) [49.254-49.256] |
| Throughput (sign/s) | 0.97574 (0.00025) [0.97555-0.97638] | 0.97896 (0.00013) [0.97875-0.97918] | 1.5059 (0.00021) [1.5055-1.5063] | 4.9273 (0.00067) [4.9259-4.9283] |
| Tegangan bus (V) | 5.0796 (0.05) [5.0299-5.196] | 5.1812 (0.034) [5.1374-5.2225] | 5.052 (0.048) [4.9983-5.1919] | 5.1731 (0.034) [5.134-5.2187] |
| Latensi episode pin (logger) (ms) | 661.46 (0.072) [661.32-661.62] | 202.17 (0.026) [202.13-202.22] | - | - |

## 2. Ketidakpastian (rerata +- U_gab; komponen acak dan sistematis)

| Kelompok | Metrik | Rerata | u_acak | u_kal | u_blok | u_idle | U_gab (rel.) |
|---|---|---|---|---|---|---|---|
| K1 | lat_mean_ms_nyata | 655.4 | 0.027 | 0 | 0.037 | 0 | 0.046 (0.01 %) |
| K1 | P_dyn_mW | 23.25 | 0.23 | 1.2 | 0.76 | 0.16 | 1.4 (6.11 %) |
| K1 | S_cpu_log | 35.458 | 0.0073 | 0 | 0.03 | 0 | 0.031 (0.09 %) |
| K1 | E_lyn_mJ | 23.828 | 0.24 | 1.2 | 0.77 | 0.17 | 1.5 (6.09 %) |
| K1 | I_dyn_mA | 4.6892 | 0.0083 | 0.23 | 0.0091 | 0.032 | 0.24 (5.05 %) |
| K1 | Q_lyn_mC | 4.8057 | 0.0085 | 0.24 | 0.0087 | 0.033 | 0.24 (5.05 %) |
| K1 | P_avg_mW | 82.455 | 0.3 | 4.1 | 1.1 | 0 | 4.3 (5.18 %) |
| K2 | lat_mean_ms_nyata | 202.15 | 0.0097 | 0 | 0.032 | 0 | 0.033 (0.02 %) |
| K2 | P_dyn_mW | 5.3021 | 0.036 | 0.27 | 0.026 | 0.17 | 0.32 (5.97 %) |
| K2 | S_cpu_log | 92.991 | 2.1e-05 | 0 | 1.2e-05 | 0 | 2.5e-05 (0.00 %) |
| K2 | E_lyn_mJ | 5.4161 | 0.037 | 0.27 | 0.025 | 0.17 | 0.32 (5.97 %) |
| K2 | I_dyn_mA | 1.0191 | 0.0057 | 0.051 | 0.013 | 0.032 | 0.062 (6.09 %) |
| K2 | Q_lyn_mC | 1.041 | 0.0058 | 0.052 | 0.013 | 0.033 | 0.063 (6.08 %) |
| K2 | P_avg_mW | 67.759 | 0.048 | 3.4 | 0.12 | 0 | 3.4 (5.00 %) |
| K3 | lat_mean_ms_nyata | 655.51 | 0.033 | 0 | 0.044 | 0 | 0.055 (0.01 %) |
| K3 | P_dyn_mW | 35.112 | 0.28 | 1.8 | 1.1 | 0.16 | 2.1 (5.96 %) |
| K3 | S_cpu_log | 0.382 | 0.00091 | 0 | 0.00077 | 0 | 0.0012 (0.31 %) |
| K3 | E_lyn_mJ | 23.317 | 0.18 | 1.2 | 0.73 | 0.11 | 1.4 (5.97 %) |
| K3 | I_dyn_mA | 7.0934 | 0.0026 | 0.35 | 0.0055 | 0.032 | 0.36 (5.02 %) |
| K3 | Q_lyn_mC | 4.7105 | 0.0017 | 0.24 | 0.0038 | 0.021 | 0.24 (5.02 %) |
| K3 | P_avg_mW | 94.279 | 0.33 | 4.7 | 1.4 | 0 | 4.9 (5.23 %) |
| K4 | lat_mean_ms_nyata | 202.17 | 0.01 | 0 | 0.033 | 0 | 0.034 (0.02 %) |
| K4 | P_dyn_mW | 23.047 | 0.054 | 1.2 | 0.14 | 0.17 | 1.2 (5.10 %) |
| K4 | S_cpu_log | 64.717 | 0.00024 | 0 | 5.9e-05 | 0 | 0.00025 (0.00 %) |
| K4 | E_lyn_mJ | 4.6774 | 0.011 | 0.23 | 0.03 | 0.034 | 0.24 (5.10 %) |
| K4 | I_dyn_mA | 4.4649 | 0.0058 | 0.22 | 0.016 | 0.032 | 0.23 (5.07 %) |
| K4 | Q_lyn_mC | 0.90615 | 0.0012 | 0.045 | 0.0031 | 0.0066 | 0.046 (5.07 %) |
| K4 | P_avg_mW | 85.496 | 0.085 | 4.3 | 0.29 | 0 | 4.3 (5.01 %) |
- Drift IDLE awal-akhir blok terbesar: 0.032 mA. u_kal berlaku pada nilai mutlak dan saling menghapus pada gain % antar-arsitektur.

## 3. RM1 dan RM3: software vs DS28E38 (positif = DS28E38 lebih baik)

| Skenario | Metrik | SW | HW | Delta [IK 95 %] | Gain % [IK 95 %] | Rasio HW/SW | Welch p (Holm) | Tumpang tindih | Keputusan |
|---|---|---|---|---|---|---|---|---|---|
| Normal | lat_mean_ms_nyata | 655.4 | 202.15 | 453.2 [453.2, 453.3] ms | 69.16 [69.15, 69.16] | 0.3084 | 2.4e-138 (1.2e-137) | tidak | tolak H0 |
| Normal | P_dyn_mW | 23.25 | 5.3021 | 17.95 [17.73, 18.17] mW | 77.19 [76.94, 77.46] | 0.2281 | 1.1e-45 (2.2e-45) | tidak | tolak H0 |
| Normal | S_cpu_log | 35.458 | 92.991 | 57.53 [57.53, 57.54] poin persentase | - | 2.623 | 2.6e-102 (7.9e-102) | tidak | tolak H0 |
| Normal | E_lyn_mJ | 23.828 | 5.4161 | 18.41 [18.19, 18.64] mJ | 77.27 [77.01, 77.53] | 0.2273 | 8.4e-46 | tidak |  |
| Normal | I_dyn_mA | 4.6892 | 1.0191 | 3.67 [3.66, 3.679] mA | 78.27 [78.14, 78.38] | 0.2173 | 2.8e-105 | tidak |  |
| Normal | Q_lyn_mC | 4.8057 | 1.041 | 3.765 [3.755, 3.774] mC | 78.34 [78.22, 78.45] | 0.2166 | 2.6e-105 | tidak |  |
| Normal | P_avg_mW | 82.455 | 67.759 | 14.7 [14.42, 14.99] mW | 17.82 [17.54, 18.12] | 0.8218 | 1e-39 | tidak |  |
| Normal | TL_pct | 100 | 100 | 0 [0, 0] poin persentase | - | 1 | - | - |  |
| Normal | Theta_sps | 0.97574 | 0.97896 | 0.00322 [0.003117, 0.003312] sign/s | - | 1.003 | - | - |  |
| Barrage | lat_mean_ms_nyata | 655.51 | 202.17 | 453.3 [453.3, 453.4] ms | 69.16 [69.16, 69.16] | 0.3084 | 8.7e-127 (3.5e-126) | tidak | tolak H0 |
| Barrage | P_dyn_mW | 35.112 | 23.047 | 12.07 [11.81, 12.34] mW | 34.36 [33.86, 34.89] | 0.6564 | 6.8e-39 (6.8e-39) | tidak | tolak H0 |
| Barrage | S_cpu_log | 0.382 | 64.717 | 64.34 [64.33, 64.34] poin persentase | - | 169.4 | 5.6e-147 (3.4e-146) | tidak | tolak H0 |
| Barrage | E_lyn_mJ | 23.317 | 4.6774 | 18.64 [18.47, 18.81] mJ | 79.94 [79.79, 80.10] | 0.2006 | 8.7e-48 | tidak |  |
| Barrage | I_dyn_mA | 7.0934 | 4.4649 | 2.629 [2.622, 2.635] mA | 37.06 [36.98, 37.14] | 0.6294 | 4.5e-86 | tidak |  |
| Barrage | Q_lyn_mC | 4.7105 | 0.90615 | 3.804 [3.802, 3.806] mC | 80.76 [80.74, 80.79] | 0.1924 | 3.6e-143 | tidak |  |
| Barrage | P_avg_mW | 94.279 | 85.496 | 8.783 [8.477, 9.106] mW | 9.32 [9.02, 9.63] | 0.9068 | 2.1e-33 | tidak |  |
| Barrage | TL_pct | 15.042 | 49.254 | 34.21 [34.21, 34.21] poin persentase | - | 3.274 | - | - |  |
| Barrage | Theta_sps | 1.5059 | 4.9273 | 3.421 [3.421, 3.422] sign/s | - | 3.272 | - | - |  |
- Aturan persen (rerata SW >= 3 x U_gab SW): rasio terkecil 16.4.
- Mann-Whitney U (cek) di banding_sw_hw.csv kolom p_mwu_cek; tanpa tumpang tindih nilainya mentok di 3,0e-11 (minimum untuk 30 vs 30).

## 4. RM2: pengaruh barrage pada tiap arsitektur (barrage - normal)

| Arsitektur | Metrik | Normal | Barrage | Selisih [IK 95 %] | Relatif |
|---|---|---|---|---|---|
| Software | S_cpu_log | 35.458 | 0.382 | -35.076 [-35.083, -35.069] | - |
| Software | P_dyn_mW | 23.25 | 35.112 | 11.863 [11.526, 12.212] | 51.02 % |
| Software | I_dyn_mA | 4.6892 | 7.0934 | 2.4043 [2.3961, 2.4127] | 51.27 % |
| Software | E_lyn_mJ | 23.828 | 23.317 | -0.51079 [-0.78846, -0.22694] | -2.144 % |
| Software | TL_pct | 100 | 15.042 | -84.958 [-84.959, -84.956] | - |
| Software | Theta_sps | 0.97574 | 1.5059 | 0.53015 [0.53003, 0.53026] | 54.33 % |
| Software | lat_mean_ms_nyata | 655.4 | 655.51 | 0.10717 [0.066867, 0.14786] | 0.01635 % |
| DS28E38 | S_cpu_log | 92.991 | 64.717 | -28.274 [-28.274, -28.274] | - |
| DS28E38 | P_dyn_mW | 5.3021 | 23.047 | 17.744 [17.682, 17.804] | 334.7 % |
| DS28E38 | I_dyn_mA | 1.0191 | 4.4649 | 3.4457 [3.4381, 3.4536] | 338.1 % |
| DS28E38 | E_lyn_mJ | 5.4161 | 4.6774 | -0.7387 [-0.77402, -0.70255] | -13.64 % |
| DS28E38 | TL_pct | 100 | 49.254 | -50.746 [-50.746, -50.746] | - |
| DS28E38 | Theta_sps | 0.97896 | 4.9273 | 3.9483 [3.9481, 3.9485] | 403.3 % |
| DS28E38 | lat_mean_ms_nyata | 202.15 | 202.17 | 0.023306 [0.010332, 0.03659] | 0.01153 % |

## 5. RM3: interaksi, DD = Delta_barrage - Delta_normal

| Metrik | Delta normal | Delta barrage | DD [IK 95 %] | Tafsiran |
|---|---|---|---|---|
| lat_mean_ms_nyata | 453.25 | 453.33 | 0.08386 [0.04135, 0.1258] | berbeda secara statistik tetapi praktis sama (|DD| < 1 % Delta) |
| P_dyn_mW | 17.948 | 12.066 | -5.882 [-6.219, -5.533] | manfaat HW lebih kecil saat barrage |
| S_cpu_log | 57.533 | 64.335 | 6.803 [6.795, 6.809] | manfaat HW lebih besar saat barrage |
| E_lyn_mJ | 18.411 | 18.639 | 0.2279 [-0.05113, 0.5162] | manfaat HW tidak terbukti berbeda antar skenario |
| I_dyn_mA | 3.67 | 2.6286 | -1.041 [-1.053, -1.03] | manfaat HW lebih kecil saat barrage |

## 6. H4: tingkat layanan dan model utilisasi

| Kelompok | N_dtg | N_lyn | N_lwt | N_ggl | TL [min-maks] | TL model | rho | Theta | Theta_maks |
|---|---|---|---|---|---|---|---|---|---|
| K1 | 59-59 | 59-59 | 0-0 | 0 | 100.00 [100.00-100.00] | 100.00 | 0.640 | 0.9757 | 1.526 |
| K2 | 59-59 | 59-59 | 0-0 | 0 | 100.00 [100.00-100.00] | 100.00 | 0.198 | 0.9790 | 4.947 |
| K3 | 604-605 | 91-91 | 513-514 | 0 | 15.04 [15.04-15.07] | 15.24 | 6.562 | 1.5059 | 1.526 |
| K4 | 603-605 | 297-298 | 306-307 | 0 | 49.25 [49.25-49.26] | 49.44 | 2.023 | 4.9273 | 4.946 |

## 7. Kesesuaian instrumen

| Pasangan | n | Bias | Bias relatif | Batas kesesuaian 95 % |
|---|---|---|---|---|
| latensi per tanda tangan K1 normal: pin - DWT (ms) | 1770 | 6.062 | 0.925 % | [5.964, 6.16] |
| latensi per tanda tangan K2 normal: pin - DWT (ms) | 1770 | 0.02263 | 0.0112 % | [-0.0108, 0.05606] |
| S_cpu 120 run: pin - firmware (poin %) | 120 | -0.001339 | -0.00277 % | [-0.003689, 0.00101] |
- Episode pin dipasangkan dengan DWT pada 60 dari 60 run normal; pada barrage episode per tanda tangan tidak terpisah (latensi barrage hanya dari DWT).
- Bias positif pin - DWT = kerja CPU di luar jendela DWT dalam satu episode bangun (tetap per tanda tangan).
- Arus INA226 vs multimeter: BELUM (jalankan ulang dengan --multimeter).

Model duty cycle per run (I_model - I_avg):

| Kelompok | I_aktif | I_tidur | Selisih rerata | Selisih maks |
|---|---|---|---|---|
| K1 | 18.735 mA | 11.732 mA | -0.0001 mA | 0.0020 mA |
| K2 | 18.921 mA | 12.639 mA | -0.0001 mA | 0.0034 mA |
| K3 | 18.689 mA | 11.752 mA | +0.0000 mA | 0.0001 mA |
| K4 | 19.053 mA | 15.154 mA | +0.0003 mA | 0.0011 mA |

## 8. Leave-one-block-out (gain % / Delta S_cpu poin)

| Analisis | Skenario | Latensi | P_dyn | E_lyn | I_dyn | Delta S_cpu |
|---|---|---|---|---|---|---|
| semua blok | Normal | 69.16 % | 77.19 % | 77.27 % | 78.27 % | 57.533 |
| semua blok | Barrage | 69.16 % | 34.36 % | 79.94 % | 37.06 % | 64.335 |
| tanpa blok1 | Normal | 69.16 % | 77.03 % | 77.11 % | 78.25 % | 57.534 |
| tanpa blok1 | Barrage | 69.16 % | 33.78 % | 79.76 % | 37.06 % | 64.336 |
| tanpa blok2 | Normal | 69.15 % | 77.29 % | 77.36 % | 78.50 % | 57.533 |
| tanpa blok2 | Barrage | 69.15 % | 33.99 % | 79.82 % | 37.26 % | 64.335 |
| tanpa blok3 | Normal | 69.16 % | 77.76 % | 77.83 % | 78.31 % | 57.544 |
| tanpa blok3 | Barrage | 69.16 % | 35.81 % | 80.38 % | 37.07 % | 64.335 |
| tanpa blok3b | Normal | 69.16 % | 77.03 % | 77.11 % | 78.26 % | 57.526 |
| tanpa blok3b | Barrage | 69.16 % | 34.14 % | 79.87 % | 37.05 % | 64.335 |
| tanpa blok4 | Normal | 69.16 % | 77.13 % | 77.21 % | 78.11 % | 57.533 |
| tanpa blok4 | Barrage | 69.16 % | 34.58 % | 80.01 % | 36.94 % | 64.335 |