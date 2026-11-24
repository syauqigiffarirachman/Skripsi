# Dekomposisi energi dinamis per tanda tangan (Normal)

rshunt = 0.0821 Ohm, offset = -2.83 LSB; n = K1: 30 run, K2: 30 run.
Energi dinamis = integral (P - P_idle) dt per fase; dibagi jumlah tanda tangan terlayani. Nilai mutlak mengikuti kalibrasi shunt di atas (u_kal bawaan 1.5 %, kalibrasi 21 Nov 2026).

| Kelompok | Fase | Durasi (ms/sign) | Arus rerata (mA) | E dinamis (mJ/sign) | Porsi (%) |
|---|---|---|---|---|---|
| K1 | CPU aktif | 664.14 | 16.859 | 49.911 (sd 0.051) | 99.9 |
| K1 | IC menghitung (CPU tidur) | 0.00 | nan | 0.000 (sd 0.000) | 0.0 |
| K1 | tidur di luar episode | 358.96 | 2.485 | 0.064 (sd 0.007) | 0.1 |
| K1 | **jumlah** | | | **49.976** | 100 |
|  | selisih terhadap E_lyn utama | | | +0.00 % | |
| K2 | CPU aktif | 18.62 | 15.871 | 1.302 (sd 0.011) | 35.5 |
| K2 | IC menghitung (CPU tidur) | 100.88 | 6.795 | 2.289 (sd 0.004) | 62.4 |
| K2 | tidur di luar episode | 897.49 | 2.461 | 0.075 (sd 0.013) | 2.0 |
| K2 | **jumlah** | | | **3.666** | 100 |
|  | selisih terhadap E_lyn utama | | | +0.00 % | |
