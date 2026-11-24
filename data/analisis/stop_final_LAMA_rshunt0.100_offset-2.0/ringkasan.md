# Ringkasan analisis varian Stop (deep sleep)

Dibuat 2026-11-18 15:31; rshunt = 0.1 Ohm, offset = -2.0 LSB, u_rel kalibrasi = 5.0%; bootstrap 10000 resampel (seed 20261116); keutuhan data: MEMENUHI STANDAR.

## 1. Deskriptif: rerata (sd) [min-maks]

| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi sign (waktu nyata) (ms) | 654.88 (0.09) [654.63-655.03] | 116.23 (0.057) [116.12-116.37] | 654.91 (0.11) [654.76-655.36] | 116.3 (0.087) [116.21-116.46] |
| Daya dinamis (mW) | 40.104 (0.042) [40.035-40.161] | 2.9591 (0.018) [2.9182-3.0012] | 61.177 (0.051) [61.075-61.249] | 25.695 (0.038) [25.608-25.765] |
| Durasi tidur CPU (%) | 35.083 (0.0083) [35.071-35.099] | 98.166 (0.0048) [98.154-98.174] | 0.37654 (0.0013) [0.37395-0.37858] | 83.966 (0.028) [83.899-84.026] |
| Energi per permintaan terlayani (mJ) | 41.03 (0.044) [40.957-41.088] | 3.0093 (0.019) [2.9678-3.0522] | 40.591 (0.035) [40.519-40.649] | 3 (0.004) [2.9925-3.0072] |
| Arus dinamis (mA) | 7.6888 (0.0035) [7.682-7.6961] | 0.56588 (0.0032) [0.56054-0.57186] | 11.725 (0.0028) [11.719-11.733] | 4.9262 (0.0071) [4.9132-4.9389] |
| Muatan dinamis per layanan (mC) | 7.8664 (0.0034) [7.8599-7.8732] | 0.57549 (0.0032) [0.57007-0.58157] | 7.7797 (0.0021) [7.7751-7.7868] | 0.57517 (0.00069) [0.57409-0.57643] |
| Daya rata-rata (mW) | 50.501 (0.042) [50.435-50.56] | 13.326 (0.016) [13.291-13.363] | 71.574 (0.051) [71.475-71.649] | 36.061 (0.038) [35.972-36.127] |
| Arus rata-rata (mA) | 9.68 (0.0036) [9.6726-9.6877] | 2.5542 (0.0031) [2.5472-2.5586] | 13.716 (0.003) [13.711-13.725] | 6.9145 (0.0082) [6.8999-6.9275] |
| Energi jendela (mJ) | 3048.4 (2.6) [3044.2-3051.9] | 799.6 (0.97) [797.48-801.8] | 4321.6 (3.2) [4315.1-4327.1] | 2166.9 (2.7) [2160.4-2171.5] |
| Tingkat layanan (%) | 100 (0) [100-100] | 100 (0) [100-100] | 15.041 (1.8e-15) [15.041-15.041] | 85.498 (0.072) [85.406-85.691] |
| Throughput (sign/s) | 0.97743 (7e-05) [0.97729-0.97757] | 0.98329 (4.9e-06) [0.98329-0.9833] | 1.5071 (0.00016) [1.5068-1.5075] | 8.5649 (0.0065) [8.553-8.5721] |
| Tegangan bus (V) | 5.2186 (0.0038) [5.2143-5.2229] | 5.218 (0.0043) [5.2063-5.224] | 5.2183 (0.0034) [5.2126-5.2209] | 5.2163 (0.005) [5.204-5.2219] |
| Latensi episode pin (logger) (ms) | 664.16 (0.073) [664.01-664.29] | 119.51 (0.043) [119.45-119.63] | - | - |

## 2. Ketidakpastian (rerata +- U_gab; komponen acak dan sistematis)

| Kelompok | Metrik | Rerata | u_acak | u_kal | u_blok | u_idle | U_gab (rel.) |
|---|---|---|---|---|---|---|---|
| K1 | lat_mean_ms_nyata | 654.88 | 0.033 | 0 | 0.028 | 0 | 0.044 (0.01 %) |
| K1 | P_dyn_mW | 40.104 | 0.016 | 2 | 0.0044 | 0.023 | 2 (5.00 %) |
| K1 | S_cpu_log | 35.083 | 0.0031 | 0 | 0.0021 | 0 | 0.0037 (0.01 %) |
| K1 | E_lyn_mJ | 41.03 | 0.016 | 2.1 | 0.0068 | 0.024 | 2.1 (5.00 %) |
| K1 | I_dyn_mA | 7.6888 | 0.0013 | 0.38 | 0.0014 | 0.0045 | 0.38 (5.00 %) |
| K1 | Q_lyn_mC | 7.8664 | 0.0013 | 0.39 | 0.001 | 0.0046 | 0.39 (5.00 %) |
| K1 | P_avg_mW | 50.501 | 0.016 | 2.5 | 0.00074 | 0 | 2.5 (5.00 %) |
| K2 | lat_mean_ms_nyata | 116.23 | 0.021 | 0 | 0.0033 | 0 | 0.022 (0.02 %) |
| K2 | P_dyn_mW | 2.9591 | 0.0068 | 0.15 | 0.012 | 0.023 | 0.15 (5.08 %) |
| K2 | S_cpu_log | 98.166 | 0.0018 | 0 | 0.00074 | 0 | 0.0019 (0.00 %) |
| K2 | E_lyn_mJ | 3.0093 | 0.0069 | 0.15 | 0.013 | 0.024 | 0.15 (5.08 %) |
| K2 | I_dyn_mA | 0.56588 | 0.0012 | 0.028 | 0.0015 | 0.0045 | 0.029 (5.07 %) |
| K2 | Q_lyn_mC | 0.57549 | 0.0012 | 0.029 | 0.0015 | 0.0045 | 0.029 (5.07 %) |
| K2 | P_avg_mW | 13.326 | 0.0061 | 0.67 | 0.0047 | 0 | 0.67 (5.00 %) |
| K3 | lat_mean_ms_nyata | 654.91 | 0.041 | 0 | 0.00064 | 0 | 0.041 (0.01 %) |
| K3 | P_dyn_mW | 61.177 | 0.019 | 3.1 | 0.011 | 0.023 | 3.1 (5.00 %) |
| K3 | S_cpu_log | 0.37654 | 0.00049 | 0 | 0.0008 | 0 | 0.00094 (0.25 %) |
| K3 | E_lyn_mJ | 40.591 | 0.013 | 2 | 0.0073 | 0.015 | 2 (5.00 %) |
| K3 | I_dyn_mA | 11.725 | 0.0011 | 0.59 | 0.0013 | 0.0045 | 0.59 (5.00 %) |
| K3 | Q_lyn_mC | 7.7797 | 0.0008 | 0.39 | 0.0009 | 0.003 | 0.39 (5.00 %) |
| K3 | P_avg_mW | 71.574 | 0.019 | 3.6 | 0.0077 | 0 | 3.6 (5.00 %) |
| K4 | lat_mean_ms_nyata | 116.3 | 0.033 | 0 | 0.032 | 0 | 0.046 (0.04 %) |
| K4 | P_dyn_mW | 25.695 | 0.014 | 1.3 | 0.0028 | 0.023 | 1.3 (5.00 %) |
| K4 | S_cpu_log | 83.966 | 0.01 | 0 | 0.024 | 0 | 0.026 (0.03 %) |
| K4 | E_lyn_mJ | 3 | 0.0015 | 0.15 | 0.001 | 0.0027 | 0.15 (5.00 %) |
| K4 | I_dyn_mA | 4.9262 | 0.0026 | 0.25 | 0.0056 | 0.0045 | 0.25 (5.00 %) |
| K4 | Q_lyn_mC | 0.57517 | 0.00026 | 0.029 | 0.00052 | 0.00052 | 0.029 (5.00 %) |
| K4 | P_avg_mW | 36.061 | 0.014 | 1.8 | 0.0049 | 0 | 1.8 (5.00 %) |
- Drift IDLE awal-akhir blok terbesar: 0.004 mA. u_kal berlaku pada nilai mutlak dan saling menghapus pada gain % antar-arsitektur.

## 3. RM1 dan RM3: software vs DS28E38 (positif = DS28E38 lebih baik)

| Skenario | Metrik | SW | HW | Delta [IK 95 %] | Gain % [IK 95 %] | Rasio HW/SW | Welch p (Holm) | Tumpang tindih | Keputusan |
|---|---|---|---|---|---|---|---|---|---|
| Normal | lat_mean_ms_nyata | 654.88 | 116.23 | 538.7 [538.6, 538.7] ms | 82.25 [82.25, 82.26] | 0.1775 | 8.2e-179 (4.1e-178) | tidak | tolak H0 |
| Normal | P_dyn_mW | 40.104 | 2.9591 | 37.14 [37.13, 37.16] mW | 92.62 [92.61, 92.64] | 0.07379 | 1.1e-113 (2.3e-113) | tidak | tolak H0 |
| Normal | S_cpu_log | 35.083 | 98.166 | 63.08 [63.08, 63.09] poin persentase | - | 2.798 | 7.6e-175 (3e-174) | tidak | tolak H0 |
| Normal | E_lyn_mJ | 41.03 | 3.0093 | 38.02 [38, 38.04] mJ | 92.67 [92.65, 92.68] | 0.07334 | 3.4e-112 | tidak |  |
| Normal | I_dyn_mA | 7.6888 | 0.56588 | 7.123 [7.121, 7.125] mA | 92.64 [92.63, 92.66] | 0.0736 | 2.4e-176 | tidak |  |
| Normal | Q_lyn_mC | 7.8664 | 0.57549 | 7.291 [7.289, 7.293] mC | 92.68 [92.67, 92.70] | 0.07316 | 1.4e-177 | tidak |  |
| Normal | P_avg_mW | 50.501 | 13.326 | 37.18 [37.16, 37.19] mW | 73.61 [73.60, 73.63] | 0.2639 | 9.4e-109 | tidak |  |
| Normal | TL_pct | 100 | 100 | 0 [0, 0] poin persentase | - | 1 | - | - |  |
| Normal | Theta_sps | 0.97743 | 0.98329 | 0.005865 [0.005841, 0.00589] sign/s | - | 1.006 | - | - |  |
| Barrage | lat_mean_ms_nyata | 654.91 | 116.3 | 538.6 [538.6, 538.7] ms | 82.24 [82.24, 82.25] | 0.1776 | 4.8e-193 (2.9e-192) | tidak | tolak H0 |
| Barrage | P_dyn_mW | 61.177 | 25.695 | 35.48 [35.46, 35.5] mW | 58.00 [57.97, 58.02] | 0.42 | 9.3e-142 (2.8e-141) | tidak | tolak H0 |
| Barrage | S_cpu_log | 0.37654 | 83.966 | 83.59 [83.58, 83.6] poin persentase | - | 223 | 6.9e-103 (6.9e-103) | tidak | tolak H0 |
| Barrage | E_lyn_mJ | 40.591 | 3 | 37.59 [37.58, 37.6] mJ | 92.61 [92.61, 92.61] | 0.07391 | 7.8e-92 | tidak |  |
| Barrage | I_dyn_mA | 11.725 | 4.9262 | 6.799 [6.796, 6.802] mA | 57.99 [57.96, 58.01] | 0.4201 | 5.2e-112 | tidak |  |
| Barrage | Q_lyn_mC | 7.7797 | 0.57517 | 7.205 [7.204, 7.205] mC | 92.61 [92.60, 92.61] | 0.07393 | 3e-123 | tidak |  |
| Barrage | P_avg_mW | 71.574 | 36.061 | 35.51 [35.49, 35.54] mW | 49.62 [49.60, 49.64] | 0.5038 | 1.7e-142 | tidak |  |
| Barrage | TL_pct | 15.041 | 85.498 | 70.46 [70.43, 70.48] poin persentase | - | 5.684 | - | - |  |
| Barrage | Theta_sps | 1.5071 | 8.5649 | 7.058 [7.055, 7.06] sign/s | - | 5.683 | - | - |  |
- Aturan persen (rerata SW >= 3 x U_gab SW): rasio terkecil 20.0.
- Mann-Whitney U (cek) di banding_sw_hw.csv kolom p_mwu_cek; tanpa tumpang tindih nilainya mentok di 3,0e-11 (minimum untuk 30 vs 30).

## 4. RM2: pengaruh barrage pada tiap arsitektur (barrage - normal)

| Arsitektur | Metrik | Normal | Barrage | Selisih [IK 95 %] | Relatif |
|---|---|---|---|---|---|
| Software | S_cpu_log | 35.083 | 0.37654 | -34.706 [-34.709, -34.703] | - |
| Software | P_dyn_mW | 40.104 | 61.177 | 21.073 [21.049, 21.096] | 52.55 % |
| Software | I_dyn_mA | 7.6888 | 11.725 | 4.0363 [4.0346, 4.0378] | 52.49 % |
| Software | E_lyn_mJ | 41.03 | 40.591 | -0.43861 [-0.45874, -0.41927] | -1.069 % |
| Software | TL_pct | 100 | 15.041 | -84.959 [-84.959, -84.959] | - |
| Software | Theta_sps | 0.97743 | 1.5071 | 0.52971 [0.52965, 0.52978] | 54.19 % |
| Software | lat_mean_ms_nyata | 654.88 | 654.91 | 0.031724 [-0.014486, 0.083774] | 0.004844 % |
| DS28E38 | S_cpu_log | 98.166 | 83.966 | -14.2 [-14.21, -14.19] | - |
| DS28E38 | P_dyn_mW | 2.9591 | 25.695 | 22.735 [22.72, 22.75] | 768.3 % |
| DS28E38 | I_dyn_mA | 0.56588 | 4.9262 | 4.3603 [4.3576, 4.3631] | 770.5 % |
| DS28E38 | E_lyn_mJ | 3.0093 | 3 | -0.0093508 [-0.015858, -0.0029753] | -0.3107 % |
| DS28E38 | TL_pct | 100 | 85.498 | -14.502 [-14.528, -14.477] | - |
| DS28E38 | Theta_sps | 0.98329 | 8.5649 | 7.5816 [7.5793, 7.5838] | 771 % |
| DS28E38 | lat_mean_ms_nyata | 116.23 | 116.3 | 0.07483 [0.038716, 0.11196] | 0.06438 % |

## 5. RM3: interaksi, DD = Delta_barrage - Delta_normal

| Metrik | Delta normal | Delta barrage | DD [IK 95 %] | Tafsiran |
|---|---|---|---|---|
| lat_mean_ms_nyata | 538.65 | 538.61 | -0.04311 [-0.1007, 0.01981] | manfaat HW tidak terbukti berbeda antar skenario |
| P_dyn_mW | 37.145 | 35.482 | -1.662 [-1.69, -1.636] | manfaat HW lebih kecil saat barrage |
| S_cpu_log | 63.083 | 83.59 | 20.51 [20.5, 20.52] | manfaat HW lebih besar saat barrage |
| E_lyn_mJ | 38.021 | 37.591 | -0.4293 [-0.4498, -0.4083] | manfaat HW lebih kecil saat barrage |
| I_dyn_mA | 7.123 | 6.7989 | -0.3241 [-0.3272, -0.3209] | manfaat HW lebih kecil saat barrage |

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
| K1 | 13.808 mA | 2.043 mA | +0.0002 mA | 0.0079 mA |
| K2 | 12.319 mA | 2.372 mA | +0.0003 mA | 0.0069 mA |
| K3 | 13.760 mA | 2.083 mA | +0.0000 mA | 0.0002 mA |
| K4 | 13.616 mA | 5.635 mA | +0.0001 mA | 0.0042 mA |

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