# Pemeriksaan keutuhan data varian Stop (deep sleep)

## 1. Run per sesi

| Blok | Sesi | Run | Sah | Gugur | Build logger |
|---|---|---|---|---|---|
| blok1 | 20261118_115044_K2S | 35 | 31 | 4 | 4C364231 |
| blok2 | 20261118_124646_K1S | 32 | 32 | 0 | 4C364231 |
| blok3 | 20261118_132320_K1S | 32 | 32 | 0 | 4C364231 |
| blok4 | 20261118_140335_K2S | 40 | 33 | 7 | 4C364231 |

Run sah dengan 'late ringan' (<= 10 cuplikan terlambat, selang < 5000 us): 62

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
| blok1 | 2.449 | 2.450 | +0.001 | 5.207 | 5.217 | OK |
| blok2 | 2.449 | 2.451 | +0.003 | 5.218 | 5.225 | OK |
| blok3 | 2.451 | 2.451 | -0.001 | 5.226 | 5.218 | OK |
| blok4 | 2.448 | 2.442 | -0.005 | 5.214 | 5.217 | OK |

## 4. Konsistensi (run sah)

- Selisih S_cpu logger vs firmware terbesar: 0.0795 poin (batas 0,5)
- Frame rusak/hilang di UART (total): 13
- Galat jam host: -64 s.d. -31 ppm (dikoreksi dengan jam logger)
- Selang cuplik INA226 terbesar: 4091 us
- Varian Stop: frame rusak total 0 (harus 0); frame hilang 13 (tiap run < 0,25 % dari yang tiba)

## 5. Pencilan (pagar Tukey 3 x IQR, hanya dilaporkan, tidak dibuang)

- K3 Latensi sign (waktu nyata): 655.3598 ms (blok2 021_K1S_B.csv; median 654.8944)
- K3 Arus dinamis: 14.2911 mA (blok3 006_K1S_B.csv; median 14.2809)
- K3 Muatan dinamis per layanan: 9.4845 mC (blok3 006_K1S_B.csv; median 9.4758)

## 6. Run gugur dan sebabnya

- Run gugur di sesi terdaftar: 11 dari 139 run (termasuk IDLE)
  - blok1 006_K2S_B.csv: host: bad=0 hilang=2 pin_salah=0 gagal=0 lat_drop=0
  - blok1 008_K2S_B.csv: host: bad=0 hilang=2 pin_salah=0 gagal=0 lat_drop=0
  - blok1 019_K2S_B.csv: host: bad=0 hilang=2 pin_salah=0 gagal=0 lat_drop=0
  - blok1 034_K2S_B.csv: host: bad=0 hilang=3 pin_salah=0 gagal=0 lat_drop=0
  - blok4 013_K2S_B.csv: host: bad=0 hilang=4 pin_salah=0 gagal=0 lat_drop=0
  - blok4 020_K2S_B.csv: host: bad=0 hilang=2 pin_salah=0 gagal=0 lat_drop=0
  - blok4 022_K2S_B.csv: host: bad=0 hilang=2 pin_salah=0 gagal=0 lat_drop=0
  - blok4 024_K2S_B.csv: host: bad=0 hilang=3 pin_salah=0 gagal=0 lat_drop=0
  - blok4 025_K2S_B.csv: host: bad=0 hilang=4 pin_salah=0 gagal=0 lat_drop=0
  - blok4 032_K2S_B.csv: host: bad=0 hilang=2 pin_salah=0 gagal=0 lat_drop=0
  - blok4 035_K2S_B.csv: host: bad=0 hilang=3 pin_salah=0 gagal=0 lat_drop=0
- Per sebab: host: bad=N hilang=N pin_salah=N gagal=N lat_drop=N: 11
- Sesi gugur utuh (data/dev/gugur/, tidak dianalisis): 3 sesi, 3 run
  - stop_salah_arg_150341 (1 run): galat_DS_cabut_saat_jalan
  - stop_salah_arg_150403 (1 run): cek_DS_terpasang
  - stop_salah_arg_150440 (1 run): galat_DS_cabut_saat_jalan

## Kesimpulan: MEMENUHI STANDAR
