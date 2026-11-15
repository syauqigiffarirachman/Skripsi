# Pemeriksaan standar data tahap D

## 1. Run per sesi

| Blok | Sesi | Run | Sah | Gugur | Build logger |
|---|---|---|---|---|---|
| blok1 | 20261115_173019_K1 | 22 | 21 | 1 | 4C364131 |
| blok2 | 20261115_180129_K2 | 42 | 39 | 3 | 4C364131 |
| blok3 | 20261115_185637_K1 | 43 | 37 | 6 | 4C364131 |
| blok3b | 20261115_204018_K1 | 8 | 8 | 0 | 4C364231 |
| blok4 | 20261115_205801_K2 | 25 | 25 | 0 | 4C364231 |

Run gugur:

- blok1 020_K1_A.csv: cuplikan diterima 30051 != dikirim 30061; checksum shunt/bus tidak cocok
- blok2 022_K2_A.csv: cuplikan diterima 29949 != dikirim 29959; checksum shunt/bus tidak cocok; tepi diterima 239 != dikirim 240
- blok2 035_K2_A.csv: cuplikan diterima 29946 != dikirim 29955; checksum shunt/bus tidak cocok
- blok2 039_K2_B.csv: cuplikan diterima 30039 != dikirim 30049; checksum shunt/bus tidak cocok
- blok3 004_K1_A.csv: cuplikan diterima 30053 != dikirim 30063; checksum shunt/bus tidak cocok
- blok3 022_K1_A.csv: cuplikan diterima 30036 != dikirim 30066; checksum shunt/bus tidak cocok
- blok3 023_K1_B.csv: cuplikan diterima 30036 != dikirim 30046; checksum shunt/bus tidak cocok
- blok3 039_K1_A.csv: cuplikan diterima 30053 != dikirim 30063; checksum shunt/bus tidak cocok
- blok3 040_K1_A.csv: cuplikan diterima 30053 != dikirim 30063; checksum shunt/bus tidak cocok
- blok3 041_K1_B.csv: cuplikan diterima 30035 != dikirim 30056; checksum shunt/bus tidak cocok

Run sah dengan 'late ringan' (<= 10 cuplikan terlambat, selang < 5000 us): 33

## 2. Jumlah run sah per kelompok (target 30)

| Kelompok | Arsitektur | Skenario | Sah | Status |
|---|---|---|---|---|
| K1 | Software | Normal | 30 | OK |
| K2 | DS28E38 | Normal | 30 | OK |
| K3 | Software | Barrage | 30 | OK |
| K4 | DS28E38 | Barrage | 30 | OK |

## 3. IDLE pembuka dan penutup tiap sesi (P_idle untuk koreksi pergeseran)

| Blok | IDLE sah | I_idle awal | I_idle akhir | Selisih | V awal | V akhir | Status |
|---|---|---|---|---|---|---|---|
| blok1 | 2 | 11.605 | 11.594 | -0.011 | 5.107 | 5.101 | OK |
| blok2 | 2 | 12.125 | 12.092 | -0.032 | 5.162 | 5.140 | OK |
| blok3 | 2 | 11.566 | 11.556 | -0.010 | 5.132 | 5.089 | OK |
| blok3b | 2 | 11.490 | 11.484 | -0.006 | 5.199 | 5.199 | OK |
| blok4 | 2 | 12.001 | 11.976 | -0.024 | 5.219 | 5.221 | OK |
Batas: |selisih| <= 0,05 mA = OK, <= 0,10 mA = PERIKSA, lebih = GAGAL.

## 4. Kecocokan model tingkat layanan (barrage)

| Kelompok | TL terukur | TL model | Selisih | Status |
|---|---|---|---|---|
| K3 | 15.042 % | 15.238 % | -0.196 | OK |
| K4 | 49.254 % | 49.443 % | -0.190 | OK |
| K1 | 100.0-100.0 % | 100 % | - | OK |
| K2 | 100.0-100.0 % | 100 % | - | OK |

## 5. Konsistensi pengukuran (run sah)

- Selisih S_cpu logger vs host terbesar: 0.0037 poin (batas 0,5)
- Selisih arus model duty-cycle vs terukur terbesar: 0.0034 mA
- Frame rusak/hilang di UART (total): 0
- Galat jam host: -5119 s.d. -4238 ppm
- Selang cuplik INA226 terbesar: 4123 us

## 6. Pencilan (pagar Tukey 3 x IQR, hanya dilaporkan, tidak dibuang)

- K1 Durasi tidur CPU: 35.4088 % (blok3b 004_K1_A.csv; median 35.4642)
- K1 Durasi tidur CPU: 35.4058 % (blok3b 006_K1_A.csv; median 35.4642)
- K3 Durasi tidur CPU: 0.3927 % (blok1 021_K1_B.csv; median 0.3818)
- K4 Durasi tidur CPU: 64.7147 % (blok2 009_K2_B.csv; median 64.7172)

## 7. Efek blok (kelompok yang diambil di lebih dari satu blok)

| Kelompok | Metrik | Blok (n): rerata | Rentang antar blok / sd dalam blok |
|---|---|---|---|
| K1 | I_avg_mA | blok1 (9): 16.2988; blok3 (17): 16.2428; blok3b (4): 16.1836 | 0.1152 / 0.0209 = 5.5 |
| K1 | I_dyn_mA | blok1 (9): 4.6990; blok3 (17): 4.6822; blok3b (4): 4.6967 | 0.0169 / 0.0209 = 0.8 |
| K1 | P_dyn_mW | blok1 (9): 23.6290; blok3 (17): 22.7963; blok3b (4): 24.3228 | 1.5265 / 0.2217 = 6.9 |
| K1 | E_lyn_mJ | blok1 (9): 24.2183; blok3 (17): 23.3655; blok3b (4): 24.9119 | 1.5464 / 0.2272 = 6.8 |
| K1 | lat_mean_ms_nyata | blok1 (9): 655.4174; blok3 (17): 655.3760; blok3b (4): 655.4498 | 0.0739 / 0.0720 = 1.0 |
| K1 | V_avg | blok1 (9): 5.0872; blok3 (17): 5.0486; blok3b (4): 5.1941 | 0.1455 / 0.0104 = 14.0 |
| K2 | I_avg_mA | blok2 (18): 13.1350; blok4 (12): 12.9966 | 0.1384 / 0.0120 = 11.5 |
| K2 | I_dyn_mA | blok2 (18): 1.0265; blok4 (12): 1.0081 | 0.0184 / 0.0120 = 1.5 |
| K2 | P_dyn_mW | blok2 (18): 5.3167; blok4 (12): 5.2804 | 0.0363 / 0.0899 = 0.4 |
| K2 | E_lyn_mJ | blok2 (18): 5.4304; blok4 (12): 5.3945 | 0.0359 / 0.0919 = 0.4 |
| K2 | lat_mean_ms_nyata | blok2 (18): 202.1333; blok4 (12): 202.1781 | 0.0448 / 0.0134 = 3.3 |
| K2 | V_avg | blok2 (18): 5.1540; blok4 (12): 5.2220 | 0.0679 / 0.0035 = 19.2 |
| K3 | I_avg_mA | blok1 (10): 18.6929; blok3 (18): 18.6532; blok3b (2): 18.5892 | 0.1037 / 0.0061 = 17.0 |
| K3 | I_dyn_mA | blok1 (10): 7.0931; blok3 (18): 7.0926; blok3b (2): 7.1023 | 0.0097 / 0.0061 = 1.6 |
| K3 | P_dyn_mW | blok1 (10): 35.7317; blok3 (18): 34.5841; blok3b (2): 36.7713 | 2.1872 / 0.1825 = 12.0 |
| K3 | E_lyn_mJ | blok1 (10): 23.7298; blok3 (18): 22.9647; blok3b (2): 24.4199 | 1.4552 / 0.1204 = 12.1 |
| K3 | lat_mean_ms_nyata | blok1 (10): 655.5509; blok3 (18): 655.4750; blok3b (2): 655.5517 | 0.0767 / 0.1335 = 0.6 |
| K3 | V_avg | blok1 (10): 5.0790; blok3 (18): 5.0216; blok3b (2): 5.1906 | 0.1690 / 0.0093 = 18.3 |
| K4 | I_avg_mA | blok2 (19): 16.5818; blok4 (11): 16.4388 | 0.1430 / 0.0104 = 13.8 |
| K4 | I_dyn_mA | blok2 (19): 4.4733; blok4 (11): 4.4503 | 0.0229 / 0.0104 = 2.2 |
| K4 | P_dyn_mW | blok2 (19): 22.9717; blok4 (11): 23.1761 | 0.2044 / 0.0959 = 2.1 |
| K4 | E_lyn_mJ | blok2 (19): 4.6618; blok4 (11): 4.7043 | 0.0426 / 0.0196 = 2.2 |
| K4 | lat_mean_ms_nyata | blok2 (19): 202.1576; blok4 (11): 202.2038 | 0.0462 / 0.0143 = 3.2 |
| K4 | V_avg | blok2 (19): 5.1476; blok4 (11): 5.2172 | 0.0696 / 0.0037 = 18.7 |
Rasio > ~3 berarti pergeseran antar blok lebih besar dari sebaran dalam blok; untuk metrik absolut (I_avg, P_avg) ini wajar (tegangan USB), metrik terkoreksi IDLE (I_dyn, P_dyn, E_lyn) seharusnya kecil.

## Kesimpulan pemeriksaan: MEMENUHI STANDAR
