# Dekomposisi energi dinamis per tanda tangan (Normal)

rshunt = 0.1 Ohm, offset = -2.0 LSB; n = K1: 30 run, K2: 30 run.
Energi dinamis = integral (P - P_idle) dt per fase; dibagi jumlah tanda tangan terlayani. Nilai mutlak mengikuti kalibrasi shunt (u_kal 5 %).

| Kelompok | Fase | Durasi (ms/sign) | Arus rerata (mA) | E dinamis (mJ/sign) | Porsi (%) |
|---|---|---|---|---|---|
| K1 | CPU aktif | 664.14 | 13.821 | 40.977 (sd 0.042) | 99.9 |
| K1 | IC menghitung (CPU tidur) | 0.00 | nan | 0.000 (sd 0.000) | 0.0 |
| K1 | tidur di luar episode | 358.96 | 2.019 | 0.053 (sd 0.005) | 0.1 |
| K1 | **jumlah** | | | **41.030** | 100 |
|  | selisih terhadap E_lyn utama | | | +0.00 % | |
| K2 | CPU aktif | 18.62 | 13.010 | 1.069 (sd 0.009) | 35.5 |
| K2 | IC menghitung (CPU tidur) | 100.88 | 5.558 | 1.879 (sd 0.003) | 62.4 |
| K2 | tidur di luar episode | 897.49 | 2.000 | 0.061 (sd 0.011) | 2.0 |
| K2 | **jumlah** | | | **3.009** | 100 |
|  | selisih terhadap E_lyn utama | | | +0.00 % | |
