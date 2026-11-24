# Ringkasan analisis varian Stop (deep sleep)

Dibuat 2026-11-21 14:34; rshunt = 0.0821 Ohm, offset = -2.83 LSB, u_rel kalibrasi = 1.5%; bootstrap 10000 resampel (seed 20261116); keutuhan data: MEMENUHI STANDAR.

## 1. Deskriptif: rerata (sd) [min-maks]

| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi sign (waktu nyata) (ms) | 654.88 (0.09) [654.63-655.03] | 116.23 (0.057) [116.12-116.37] | 654.91 (0.11) [654.76-655.36] | 116.3 (0.087) [116.21-116.46] |
| Daya dinamis (mW) | 48.848 (0.052) [48.763-48.917] | 3.6043 (0.022) [3.5543-3.6557] | 74.515 (0.062) [74.39-74.603] | 31.297 (0.046) [31.191-31.383] |
| Durasi tidur CPU (%) | 35.083 (0.0083) [35.071-35.099] | 98.166 (0.0048) [98.154-98.174] | 0.37654 (0.0013) [0.37395-0.37858] | 83.966 (0.028) [83.899-84.026] |
| Energi per permintaan terlayani (mJ) | 49.976 (0.054) [49.887-50.046] | 3.6656 (0.023) [3.6147-3.7178] | 49.441 (0.042) [49.353-49.511] | 3.6541 (0.0049) [3.6449-3.6629] |
| Arus dinamis (mA) | 9.3652 (0.0042) [9.3568-9.3741] | 0.68926 (0.0039) [0.68275-0.69654] | 14.281 (0.0035) [14.275-14.291] | 6.0003 (0.0086) [5.9844-6.0157] |
| Muatan dinamis per layanan (mC) | 9.5815 (0.0042) [9.5735-9.5898] | 0.70097 (0.0039) [0.69435-0.70837] | 9.4759 (0.0026) [9.4703-9.4845] | 0.70057 (0.00085) [0.69926-0.70211] |
| Daya rata-rata (mW) | 61.644 (0.052) [61.563-61.716] | 16.363 (0.02) [16.32-16.408] | 87.311 (0.062) [87.19-87.402] | 44.055 (0.046) [43.947-44.136] |
| Arus rata-rata (mA) | 11.816 (0.0044) [11.807-11.825] | 3.1364 (0.0037) [3.1278-3.1417] | 16.732 (0.0037) [16.726-16.742] | 8.4473 (0.01) [8.4295-8.4632] |
| Energi jendela (mJ) | 3721 (3.2) [3715.9-3725.3] | 981.85 (1.2) [979.24-984.53] | 5271.8 (3.9) [5263.9-5278.5] | 2647.3 (3.3) [2639.4-2652.9] |
| Tingkat layanan (%) | 100 (0) [100-100] | 100 (0) [100-100] | 15.041 (1.8e-15) [15.041-15.041] | 85.498 (0.072) [85.406-85.691] |
| Throughput (sign/s) | 0.97743 (7e-05) [0.97729-0.97757] | 0.98329 (4.9e-06) [0.98329-0.9833] | 1.5071 (0.00016) [1.5068-1.5075] | 8.5649 (0.0065) [8.553-8.5721] |
| Tegangan bus (V) | 5.2186 (0.0038) [5.2143-5.2229] | 5.218 (0.0043) [5.2063-5.224] | 5.2183 (0.0034) [5.2126-5.2209] | 5.2163 (0.005) [5.204-5.2219] |
| Latensi episode pin (logger) (ms) | 664.16 (0.073) [664.01-664.29] | 119.51 (0.043) [119.45-119.63] | - | - |

## 2. Ketidakpastian (rerata +- U_gab; komponen acak dan sistematis)

| Kelompok | Metrik | Rerata | u_acak | u_kal | u_blok | u_idle | U_gab (rel.) |
|---|---|---|---|---|---|---|---|
| K1 | lat_mean_ms_nyata | 654.88 | 0.033 | 0 | 0.028 | 0 | 0.044 (0.01 %) |
| K1 | P_dyn_mW | 48.848 | 0.019 | 0.73 | 0.0054 | 0.028 | 0.73 (1.50 %) |
| K1 | S_cpu_log | 35.083 | 0.0031 | 0 | 0.0021 | 0 | 0.0037 (0.01 %) |
| K1 | E_lyn_mJ | 49.976 | 0.02 | 0.75 | 0.0083 | 0.029 | 0.75 (1.50 %) |
| K1 | I_dyn_mA | 9.3652 | 0.0016 | 0.14 | 0.0017 | 0.0054 | 0.14 (1.50 %) |
| K1 | Q_lyn_mC | 9.5815 | 0.0016 | 0.14 | 0.0012 | 0.0056 | 0.14 (1.50 %) |
| K1 | P_avg_mW | 61.644 | 0.019 | 0.92 | 0.00093 | 0 | 0.92 (1.50 %) |
| K2 | lat_mean_ms_nyata | 116.23 | 0.021 | 0 | 0.0033 | 0 | 0.022 (0.02 %) |
| K2 | P_dyn_mW | 3.6043 | 0.0083 | 0.054 | 0.015 | 0.028 | 0.063 (1.76 %) |
| K2 | S_cpu_log | 98.166 | 0.0018 | 0 | 0.00074 | 0 | 0.0019 (0.00 %) |
| K2 | E_lyn_mJ | 3.6656 | 0.0084 | 0.055 | 0.015 | 0.029 | 0.065 (1.76 %) |
| K2 | I_dyn_mA | 0.68926 | 0.0014 | 0.01 | 0.0018 | 0.0054 | 0.012 (1.73 %) |
| K2 | Q_lyn_mC | 0.70097 | 0.0015 | 0.011 | 0.0018 | 0.0055 | 0.012 (1.73 %) |
| K2 | P_avg_mW | 16.363 | 0.0074 | 0.25 | 0.0058 | 0 | 0.25 (1.50 %) |
| K3 | lat_mean_ms_nyata | 654.91 | 0.041 | 0 | 0.00064 | 0 | 0.041 (0.01 %) |
| K3 | P_dyn_mW | 74.515 | 0.023 | 1.1 | 0.014 | 0.028 | 1.1 (1.50 %) |
| K3 | S_cpu_log | 0.37654 | 0.00049 | 0 | 0.0008 | 0 | 0.00094 (0.25 %) |
| K3 | E_lyn_mJ | 49.441 | 0.016 | 0.74 | 0.0089 | 0.019 | 0.74 (1.50 %) |
| K3 | I_dyn_mA | 14.281 | 0.0013 | 0.21 | 0.0016 | 0.0054 | 0.21 (1.50 %) |
| K3 | Q_lyn_mC | 9.4759 | 0.00097 | 0.14 | 0.0011 | 0.0036 | 0.14 (1.50 %) |
| K3 | P_avg_mW | 87.311 | 0.023 | 1.3 | 0.0095 | 0 | 1.3 (1.50 %) |
| K4 | lat_mean_ms_nyata | 116.3 | 0.033 | 0 | 0.032 | 0 | 0.046 (0.04 %) |
| K4 | P_dyn_mW | 31.297 | 0.017 | 0.47 | 0.0035 | 0.028 | 0.47 (1.50 %) |
| K4 | S_cpu_log | 83.966 | 0.01 | 0 | 0.024 | 0 | 0.026 (0.03 %) |
| K4 | E_lyn_mJ | 3.6541 | 0.0018 | 0.055 | 0.0012 | 0.0033 | 0.055 (1.50 %) |
| K4 | I_dyn_mA | 6.0003 | 0.0032 | 0.09 | 0.0068 | 0.0054 | 0.09 (1.51 %) |
| K4 | Q_lyn_mC | 0.70057 | 0.00032 | 0.011 | 0.00064 | 0.00063 | 0.011 (1.51 %) |
| K4 | P_avg_mW | 44.055 | 0.017 | 0.66 | 0.0058 | 0 | 0.66 (1.50 %) |
- Drift IDLE awal-akhir blok terbesar: 0.005 mA. u_kal berlaku pada nilai mutlak dan saling menghapus pada gain % antar-arsitektur.

## 3. RM1 dan RM3: software vs DS28E38 (positif = DS28E38 lebih baik)

| Skenario | Metrik | SW | HW | Delta [IK 95 %] | Gain % [IK 95 %] | Rasio HW/SW | Welch p (Holm) | Tumpang tindih | Keputusan |
|---|---|---|---|---|---|---|---|---|---|
| Normal | lat_mean_ms_nyata | 654.88 | 116.23 | 538.7 [538.6, 538.7] ms | 82.25 [82.25, 82.26] | 0.1775 | 8.2e-179 (4.1e-178) | tidak | tolak H0 |
| Normal | P_dyn_mW | 48.848 | 3.6043 | 45.24 [45.22, 45.26] mW | 92.62 [92.61, 92.64] | 0.07379 | 1.2e-113 (2.5e-113) | tidak | tolak H0 |
| Normal | S_cpu_log | 35.083 | 98.166 | 63.08 [63.08, 63.09] poin persentase | - | 2.798 | 7.6e-175 (3e-174) | tidak | tolak H0 |
| Normal | E_lyn_mJ | 49.976 | 3.6656 | 46.31 [46.29, 46.33] mJ | 92.67 [92.65, 92.68] | 0.07335 | 3.7e-112 | tidak |  |
| Normal | I_dyn_mA | 9.3652 | 0.68926 | 8.676 [8.674, 8.678] mA | 92.64 [92.63, 92.66] | 0.0736 | 2.4e-176 | tidak |  |
| Normal | Q_lyn_mC | 9.5815 | 0.70097 | 8.881 [8.879, 8.883] mC | 92.68 [92.67, 92.70] | 0.07316 | 1.4e-177 | tidak |  |
| Normal | P_avg_mW | 61.644 | 16.363 | 45.28 [45.26, 45.3] mW | 73.45 [73.44, 73.47] | 0.2655 | 9.7e-109 | tidak |  |
| Normal | TL_pct | 100 | 100 | 0 [0, 0] poin persentase | - | 1 | - | - |  |
| Normal | Theta_sps | 0.97743 | 0.98329 | 0.005865 [0.005841, 0.00589] sign/s | - | 1.006 | - | - |  |
| Barrage | lat_mean_ms_nyata | 654.91 | 116.3 | 538.6 [538.6, 538.7] ms | 82.24 [82.24, 82.25] | 0.1776 | 4.8e-193 (2.9e-192) | tidak | tolak H0 |
| Barrage | P_dyn_mW | 74.515 | 31.297 | 43.22 [43.19, 43.24] mW | 58.00 [57.97, 58.02] | 0.42 | 1e-141 (3.1e-141) | tidak | tolak H0 |
| Barrage | S_cpu_log | 0.37654 | 83.966 | 83.59 [83.58, 83.6] poin persentase | - | 223 | 6.9e-103 (6.9e-103) | tidak | tolak H0 |
| Barrage | E_lyn_mJ | 49.441 | 3.6541 | 45.79 [45.77, 45.8] mJ | 92.61 [92.61, 92.61] | 0.07391 | 8e-92 | tidak |  |
| Barrage | I_dyn_mA | 14.281 | 6.0003 | 8.281 [8.278, 8.284] mA | 57.99 [57.96, 58.01] | 0.4201 | 5.2e-112 | tidak |  |
| Barrage | Q_lyn_mC | 9.4759 | 0.70057 | 8.775 [8.774, 8.776] mC | 92.61 [92.60, 92.61] | 0.07393 | 3e-123 | tidak |  |
| Barrage | P_avg_mW | 87.311 | 44.055 | 43.26 [43.23, 43.28] mW | 49.54 [49.52, 49.56] | 0.5046 | 2e-142 | tidak |  |
| Barrage | TL_pct | 15.041 | 85.498 | 70.46 [70.43, 70.48] poin persentase | - | 5.684 | - | - |  |
| Barrage | Theta_sps | 1.5071 | 8.5649 | 7.058 [7.055, 7.06] sign/s | - | 5.683 | - | - |  |
- Aturan persen (rerata SW >= 3 x U_gab SW): rasio terkecil 66.6.
- Mann-Whitney U (cek) di banding_sw_hw.csv kolom p_mwu_cek; tanpa tumpang tindih nilainya mentok di 3,0e-11 (minimum untuk 30 vs 30).

## 4. RM2: pengaruh barrage pada tiap arsitektur (barrage - normal)

| Arsitektur | Metrik | Normal | Barrage | Selisih [IK 95 %] | Relatif |
|---|---|---|---|---|---|
| Software | S_cpu_log | 35.083 | 0.37654 | -34.706 [-34.709, -34.703] | - |
| Software | P_dyn_mW | 48.848 | 74.515 | 25.668 [25.638, 25.695] | 52.55 % |
| Software | I_dyn_mA | 9.3652 | 14.281 | 4.9163 [4.9143, 4.9182] | 52.49 % |
| Software | E_lyn_mJ | 49.976 | 49.441 | -0.53421 [-0.55878, -0.51061] | -1.069 % |
| Software | TL_pct | 100 | 15.041 | -84.959 [-84.959, -84.959] | - |
| Software | Theta_sps | 0.97743 | 1.5071 | 0.52971 [0.52965, 0.52978] | 54.19 % |
| Software | lat_mean_ms_nyata | 654.88 | 654.91 | 0.031724 [-0.014486, 0.083774] | 0.004844 % |
| DS28E38 | S_cpu_log | 98.166 | 83.966 | -14.2 [-14.21, -14.19] | - |
| DS28E38 | P_dyn_mW | 3.6043 | 31.297 | 27.692 [27.674, 27.71] | 768.3 % |
| DS28E38 | I_dyn_mA | 0.68926 | 6.0003 | 5.311 [5.3077, 5.3144] | 770.5 % |
| DS28E38 | E_lyn_mJ | 3.6656 | 3.6541 | -0.011489 [-0.01943, -0.0037211] | -0.3134 % |
| DS28E38 | TL_pct | 100 | 85.498 | -14.502 [-14.528, -14.477] | - |
| DS28E38 | Theta_sps | 0.98329 | 8.5649 | 7.5816 [7.5793, 7.5838] | 771 % |
| DS28E38 | lat_mean_ms_nyata | 116.23 | 116.3 | 0.07483 [0.038716, 0.11196] | 0.06438 % |

## 5. RM3: interaksi, DD = Delta_barrage - Delta_normal

| Metrik | Delta normal | Delta barrage | DD [IK 95 %] | Tafsiran |
|---|---|---|---|---|
| lat_mean_ms_nyata | 538.65 | 538.61 | -0.04311 [-0.1007, 0.01981] | manfaat HW tidak terbukti berbeda antar skenario |
| P_dyn_mW | 45.243 | 43.218 | -2.025 [-2.058, -1.992] | manfaat HW lebih kecil saat barrage |
| S_cpu_log | 63.083 | 83.59 | 20.51 [20.5, 20.52] | manfaat HW lebih besar saat barrage |
| E_lyn_mJ | 46.31 | 45.787 | -0.5227 [-0.5478, -0.4972] | manfaat HW lebih kecil saat barrage |
| I_dyn_mA | 8.676 | 8.2812 | -0.3947 [-0.3986, -0.3909] | manfaat HW lebih kecil saat barrage |

## 6. H4: tingkat layanan dan model utilisasi

| Kelompok | N_dtg | N_lyn | N_lwt | N_ggl | TL [min-maks] | TL model | rho | Theta | Theta_maks |
|---|---|---|---|---|---|---|---|---|---|
| K1 | 59-59 | 59-59 | 0-0 | 0 | 100.00 [100.00-100.00] | 100.00 | 0.640 | 0.9774 | 1.527 |
| K2 | 59-59 | 59-59 | 0-0 | 0 | 100.00 [100.00-100.00] | 100.00 | 0.114 | 0.9833 | 8.604 |
| K3 | 605-605 | 91-91 | 514-514 | 0 | 15.04 [15.04-15.04] | 15.24 | 6.562 | 1.5071 | 1.527 |
| K4 | 601-603 | 514-515 | 86-88 | 0 | 85.50 [85.41-85.69] | 85.83 | 1.165 | 8.5649 | 8.598 |

## 7. Kesesuaian instrumen

| Pasangan | n | Bias | Bias relatif | Batas kesesuaian 95 % |
|---|---|---|---|---|
| S_cpu 120 run: pin - firmware (poin %) | 120 | -0.01424 | -0.0262 % | [-0.05477, 0.02628] |
- Episode pin dipasangkan dengan DWT pada 60 dari 60 run normal; pada barrage episode per tanda tangan tidak terpisah (latensi barrage hanya dari DWT).
- Bias positif pin - DWT = kerja CPU di luar jendela DWT dalam satu episode bangun (tetap per tanda tangan).
- Arus INA226 vs multimeter: BELUM (jalankan ulang dengan --multimeter).

Model duty cycle per run (I_model - I_avg):

| Kelompok | I_aktif | I_tidur | Selisih rerata | Selisih maks |
|---|---|---|---|---|
| K1 | 16.843 mA | 2.513 mA | +0.0002 mA | 0.0096 mA |
| K2 | 15.030 mA | 2.915 mA | +0.0004 mA | 0.0084 mA |
| K3 | 16.786 mA | 2.562 mA | +0.0000 mA | 0.0003 mA |
| K4 | 16.610 mA | 6.889 mA | +0.0001 mA | 0.0051 mA |

## 8. Leave-one-block-out (gain % / Delta S_cpu poin)

| Analisis | Skenario | Latensi | P_dyn | E_lyn | I_dyn | Delta S_cpu |
|---|---|---|---|---|---|---|
| semua blok | Normal | 82.25 % | 92.62 % | 92.67 % | 92.64 % | 63.083 |
| semua blok | Barrage | 82.24 % | 58.00 % | 92.61 % | 57.99 % | 83.590 |
| tanpa blok1 | Normal | 82.25 % | 92.60 % | 92.64 % | 92.63 % | 63.083 |
| tanpa blok1 | Barrage | 82.24 % | 58.00 % | 92.61 % | 58.02 % | 83.605 |
| tanpa blok2 | Normal | 82.25 % | 92.62 % | 92.66 % | 92.64 % | 63.085 |
| tanpa blok2 | Barrage | 82.24 % | 57.99 % | 92.61 % | 57.99 % | 83.590 |
| tanpa blok3 | Normal | 82.25 % | 92.62 % | 92.67 % | 92.64 % | 63.082 |
| tanpa blok3 | Barrage | 82.24 % | 58.01 % | 92.61 % | 57.98 % | 83.589 |
| tanpa blok4 | Normal | 82.25 % | 92.64 % | 92.69 % | 92.65 % | 63.084 |
| tanpa blok4 | Barrage | 82.25 % | 58.00 % | 92.61 % | 57.95 % | 83.572 |