# Pemeriksaan keutuhan data tahap D

## 1. Run per sesi

| Blok | Sesi | Run | Sah | Gugur | Build logger |
|---|---|---|---|---|---|
| blok1 | 20261118_094801_K1K2 | 16 | 13 | 3 | 4C364231 |

Run sah dengan 'late ringan' (<= 10 cuplikan terlambat, selang < 5000 us): 4

## 2. Jumlah run sah per kelompok (target 30)

| Kelompok | Arsitektur | Skenario | Sah |
|---|---|---|---|
| K1 | Software | Normal | 3 |
| K2 | DS28E38 | Normal | 3 |
| K3 | Software | Barrage | 3 |
| K4 | DS28E38 | Barrage | 0 |

## 3. IDLE pembuka dan penutup tiap blok (drift baseline)

| Blok | I_idle awal | I_idle akhir | Selisih (mA) | V awal | V akhir | Status |
|---|---|---|---|---|---|---|
| blok1 | 2.566 | 2.566 | -0.000 | 5.212 | 5.215 | OK |

## 4. Konsistensi (run sah)

- Selisih S_cpu logger vs firmware terbesar: 0.0124 poin (batas 0,5)
- Frame rusak/hilang di UART (total): 0
- Galat jam host: -48 s.d. -47 ppm (dikoreksi dengan jam logger)
- Selang cuplik INA226 terbesar: 4067 us

## 5. Pencilan (pagar Tukey 3 x IQR, hanya dilaporkan, tidak dibuang)

- tidak ada

## 6. Run gugur dan sebabnya

- Run gugur di sesi terdaftar: 3 dari 16 run (termasuk IDLE)
  - blok1 006_K2S_B.csv: host: bad=1 hilang=1 pin_salah=0 gagal=0 lat_drop=0
  - blok1 010_K2S_B.csv: host: bad=1 hilang=1 pin_salah=0 gagal=0 lat_drop=0
  - blok1 014_K2S_B.csv: host: bad=1 hilang=1 pin_salah=0 gagal=0 lat_drop=0
- Per sebab: host: bad=N hilang=N pin_salah=N gagal=N lat_drop=N: 3
- Sesi gugur utuh (data/dev/gugur/, tidak dianalisis): 4 sesi, 18 run
  - 20261115_195039_K1 (8 run): TAHAP D blok 3b (tambahan K1, pengganti run gugur blok 1 dan 3): K1 konfigurasi 1c, 4A+2B; ST-Link host dicabut; daya host USB-C langsung ke port komputer; logger KEMBALI lewat hub; aturan late ringan berlaku
  - 20261115_201042_K1 (8 run): TAHAP D blok 3b ULANG (pengganti run gugur blok 1 dan 3): K1 konfigurasi 1c, 4A+2B; host baru dicabut-colok daya (POR); ST-Link host dicabut; daya host USB-C langsung ke port komputer; logger lewat hub; aturan late ringan
  - diagnosis_20261115_195951_K1 (1 run): diagnosis pergeseran +0,78 mA setelah blok 3b
  - diagnosis_20261115_200926_K1 (1 run): diagnosis: IDLE setelah host dicabut-colok daya (power-on reset)

## Kesimpulan: BELUM MEMENUHI STANDAR
