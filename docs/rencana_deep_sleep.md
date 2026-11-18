# Rencana varian Deep Sleep (Stop mode) host

Disusun 17 Nov 2026. Status tiap langkah dicatat di logbook harian; tabel ini diperbarui saat langkah lulus.

## Keputusan (langkah 1, 17 Nov)

- Stop mode **ditambahkan** sebagai varian. Data tahap D (Sleep/WFI @48 MHz, 120 run) **tetap** dipakai, tidak dihapus.
- 17 Nov (lanjutan): lantai Stop ~2 mA DITERIMA user; LED PWR Blue Pill tetap terpasang sebagai beban tetap papan
  (sama di data tahap D, jadi perbandingan Sleep vs Stop adil). Instrumen uA tidak diperlukan.
- Titik ukur tetap di **input 5 V USB-C** (sama dengan tahap D). Lantai tidur papan terukur ~2 mA (LED PWR
  ~1,9 mA + MCU Stop ~14 uA + LDO XC6219B332MR ~25 uA typ), bukan 1-20 uA tingkat chip.
- Firmware Stop dibuat di env baru; `k1_sw`/`k2_hw` dan `tools/biner_acuan.json` tidak disentuh.

## Fakta papan host (17 Nov)

| Butir | Nilai |
|---|---|
| Catu | modul USB-C 5 V -> pin 5V Blue Pill |
| LDO 3V3 | XC6219B332MR (marking LB2K), Iq 25 uA typ / 50 uA maks, CE aktif tinggi |
| LED power modul/papan | resistor dilepas (17 Nov) |
| LED PC13 Blue Pill | masih terpasang (tidak menarik arus bila PC13 analog/tinggi) |
| Kristal LSE | Epson MC-306 32,768 kHz, ESR maks 50 kOhm, CL standar 12,5 pF, +-20 ppm. Margin start AN2867 ~3,3 (CL 12,5 pF, ESR maks) -> wajib diuji |
| PC14/PC15 | bebas (tanpa kabel) |
| DS28E38 | hanya IO+GND (selalu parasit); jumper VCC/CEXT DS9121AQ+ kemungkinan tak berpengaruh (verifikasi) |

## Langkah

| # | Fase | Langkah | Kriteria lulus | Status |
|---|---|---|---|---|
| 1 | 0 | Keputusan desain | tercatat | LULUS 17 Nov |
| 2 | H | Finalisasi papan (LED, jumper DS9121, CE LDO, PC14/15) | IDLE Sleep diukur ulang, foto + tanggal di logbook | 17 Nov: LED PWR Blue Pill (merah) SENGAJA dipertahankan (keputusan user); R seri 10 kOhm di logger->PA10 dan attacker->PA3 |
| 3 | H | Instrumen uA tervalidasi (PPK2 / shunt) | galat <= 2-5 % pada ~33 uA dan ~3,3 mA | TIDAK DIPERLUKAN: lantai diterima ~2 mA (= ~80 LSB INA226) |
| 4 | F | S0a uji LSE (`host_s0a_lse`, `_dingin`) | LSE start tiap start dingin (>= 10x), frekuensi 32768 Hz +- 50 ppm vs logger | LULUS 17 Nov (start dingin 3,5-4,1 s; -47 ppm stabil) |
| 5 | F | S0 Stop minimal -> lantai papan (`host_s0_stop`, `tools/s0_stop.py`) | lantai stabil dan terjelaskan | LULUS 17 Nov: 1,99 mA (POR bersih, tanpa ST-Link), didominasi LED PWR |
| 6 | F | S1 Stop + alarm RTC + pulih 48 MHz | T_sleep RTC = duty PB1 logger (< 1 ms/60 s) | LULUS 17 Nov: selisih 9-20 us per jendela 28,5 s (5 jendela), bangun palsu 0, arus tidur 1,988 mA |
| 7 | F | S2 bangun dari attacker (EXTI PA3 + sinkron bit byte pertama, attacker TIDAK diubah) | 60 s @1 Hz dan 10 Hz: bad = hilang = 0 | LULUS 17 Nov dan DIULANG 18 Nov (setelah pemulihan host, kode final): 1 Hz 59/59, 10 Hz 602/602, bad = hilang = palsu = 0 |
| 8 | F | S3 integrasi K1 (`host_k1_stop`, `_auto`) | run A/B 60 s lolos semua pemeriksaan run_bench | LULUS 18 Nov: A 59/59 (S_cpu logger 32,39 % = host 32,40 %, lat 683 ms @48 MHz), B jenuh seperti K1 baseline (604 tiba, 87 dilayani), semua pemeriksaan lolos |
| 9 | F | S4 integrasi K2 (`host_k2_stop`, `_auto`): overdrive + polling, tunggu chip ~100 ms di Stop | sign valid, A/B 60 s lolos | A LULUS 18 Nov (59/59, lat 116,2 ms, S_cpu 98,19 % = logger, spurious 0). B: 515/600 dilayani, S_cpu cocok, TAPI 1-2 frame rusak/hilang per 600 (kejadian frame bertepatan dengan transisi bangun saat tunggu chip) = temuan; setelah cek idle ketat + penerima dini di jalur alarm (18 Nov malam): bad = palsu = 0, hilang ~0,9 per 600 (2 dari 8 run B bersih penuh) |
| 10 | V | Ketahanan, jalur galat, uji daya balik | lantai semua papan tersambung = lantai host sendiri | LULUS 18 Nov: lantai bersih 1,99 mA (POR penuh), ketahanan 0 crash, jalur galat G1-G7 lulus (attacker berhenti/restart/bolak-balik; DS28E38 dicabut saat jalan = gagal tercatat tanpa crash, pulih tanpa reboot; absen saat boot = siap 0 dan run ditolak) |
| 11 | V | Pilot N=3 per kelompok + alat analisis | metrik lengkap, N=30 cukup | LULUS 18 Nov: pilot K1S/K2S, `analisis_d.py --stop`, toleransi hilang < 0,25 % (keputusan user) |
| 12 | D | Data final 4 x 30 + IDLE (ABBA) | manifest + pemeriksaan otomatis lolos | LULUS 18 Nov: `data/tahap_stop.txt`, 4 blok K2S/K1S/K1S/K2S, 30 sah per kelompok, pemeriksaan MEMENUHI STANDAR, analisis di `data/analisis/stop_final/` |

## Aturan keselamatan firmware Stop

- Jeda >= 3 s setelah boot sebelum Stop pertama (SWD tak bisa tersambung selama Stop). Pemulihan cadangan:
  BOOT0 = 1 (bootloader) atau connect-under-reset.
- Setelah flash: `run_bench.py nodebug` atau cabut-colok daya (DBGMCU_CR 0x307 menyalakan DBG_STOP).
- Jangan probe langsung PC14/PC15 (kapasitansi probe menghentikan LSE).
- Jangan catu 3V3 eksternal bersamaan dengan 5 V.

## Tambahan 18 Nov: DS28E38 overdrive (eksperimen terpisah)

`src/host/eksperimen/ds_overdrive` + `tools/ds_od.py`. Overdrive ternyata bisa (percobaan 11 Nov gagal karena library
OneWire tak mendukungnya). Titik baca sah 2,0-3,0 us; dipakai 2,5 us. 269/269 tanda tangan valid di PC.

| Mode | Total per sign |
|---|---|
| standar + tunggu tetap 130 ms (seperti K2 sekarang) | 192,5 ms |
| standar + polling | 169,6 ms |
| overdrive + tunggu tetap 130 ms | 138,7 ms |
| **overdrive + polling** | **115,6 ms** (kirim 3,4 + chip 106,6 + baca 5,6) |

Chip selalu butuh 105,9-107,3 ms (sd 0,22 ms): itu batas bawah. Tunggu 130 ms bisa jadi ~100 ms tidur + polling.

## Catatan keandalan Stop (belum tuntas)

Dengan ST-Link menempel, beberapa susunan kode membuat inti "terlempar" (PC = 0) tepat setelah bangun dari Stop.
`stop_common.h` dikembalikan ke konfigurasi yang terakhir lulus S2. Uji penentu: ST-Link dicabut + POR bersih.
