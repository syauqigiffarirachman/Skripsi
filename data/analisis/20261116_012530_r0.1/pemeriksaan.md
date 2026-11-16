# Pemeriksaan keutuhan data tahap D

## 1. Run per sesi

| Blok | Sesi | Run | Sah | Gugur | Build logger |
|---|---|---|---|---|---|
| blok1 | 20261115_173019_K1 | 22 | 21 | 1 | 4C364131 |
| blok2 | 20261115_180129_K2 | 42 | 39 | 3 | 4C364131 |
| blok3 | 20261115_185637_K1 | 43 | 37 | 6 | 4C364131 |
| blok3b | 20261115_204018_K1 | 8 | 8 | 0 | 4C364231 |
| blok4 | 20261115_205801_K2 | 25 | 25 | 0 | 4C364231 |

Run sah dengan 'late ringan' (<= 10 cuplikan terlambat, selang < 5000 us): 33

## 2. Jumlah run sah per kelompok (target 30)

| Kelompok | Arsitektur | Skenario | Sah |
|---|---|---|---|
| K1 | Software | Normal | 30 |
| K2 | DS28E38 | Normal | 30 |
| K3 | Software | Barrage | 30 |
| K4 | DS28E38 | Barrage | 30 |

## 3. IDLE pembuka dan penutup tiap blok (drift baseline)

| Blok | I_idle awal | I_idle akhir | Selisih (mA) | V awal | V akhir | Status |
|---|---|---|---|---|---|---|
| blok1 | 11.605 | 11.594 | -0.011 | 5.107 | 5.101 | OK |
| blok2 | 12.125 | 12.092 | -0.032 | 5.162 | 5.140 | OK |
| blok3 | 11.566 | 11.556 | -0.010 | 5.132 | 5.089 | OK |
| blok3b | 11.490 | 11.484 | -0.006 | 5.199 | 5.199 | OK |
| blok4 | 12.001 | 11.976 | -0.024 | 5.219 | 5.221 | OK |

## 4. Konsistensi (run sah)

- Selisih S_cpu logger vs firmware terbesar: 0.0037 poin (batas 0,5)
- Frame rusak/hilang di UART (total): 0
- Galat jam host: -5119 s.d. -4238 ppm (dikoreksi dengan jam logger)
- Selang cuplik INA226 terbesar: 4123 us

## 5. Pencilan (pagar Tukey 3 x IQR, hanya dilaporkan, tidak dibuang)

- K1 Durasi tidur CPU: 35.4088 % (blok3b 004_K1_A.csv; median 35.4642)
- K1 Durasi tidur CPU: 35.4058 % (blok3b 006_K1_A.csv; median 35.4642)
- K3 Durasi tidur CPU: 0.3927 % (blok1 021_K1_B.csv; median 0.3818)
- K4 Durasi tidur CPU: 64.7147 % (blok2 009_K2_B.csv; median 64.7172)

## 6. Run gugur dan sebabnya

- Run gugur di sesi terdaftar: 10 dari 140 run (termasuk IDLE)
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
- Per sebab: cuplikan diterima N != dikirim N: 10; checksum shunt/bus tidak cocok: 10; tepi diterima N != dikirim N: 1
- Sesi gugur utuh (data/dev/gugur/, tidak dianalisis): 4 sesi, 18 run
  - 20261115_195039_K1 (8 run): TAHAP D blok 3b (tambahan K1, pengganti run gugur blok 1 dan 3): K1 konfigurasi 1c, 4A+2B; ST-Link host dicabut; daya host USB-C langsung ke port komputer; logger KEMBALI lewat hub; aturan late ringan berlaku
  - 20261115_201042_K1 (8 run): TAHAP D blok 3b ULANG (pengganti run gugur blok 1 dan 3): K1 konfigurasi 1c, 4A+2B; host baru dicabut-colok daya (POR); ST-Link host dicabut; daya host USB-C langsung ke port komputer; logger lewat hub; aturan late ringan
  - diagnosis_20261115_195951_K1 (1 run): diagnosis pergeseran +0,78 mA setelah blok 3b
  - diagnosis_20261115_200926_K1 (1 run): diagnosis: IDLE setelah host dicabut-colok daya (power-on reset)

## Kesimpulan: MEMENUHI STANDAR
