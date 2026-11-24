# Peta alur: dari firmware sampai kesimpulan

Dokumen ini menjawab satu pertanyaan: **kode mana yang menghasilkan angka di skripsi, dan dalam urutan apa ia dibaca.**
Yang dimaksud "final" adalah kode yang dipakai pada pengambilan data varian Stop pada **18 Nov 2026**
(4 blok ABBA: K2S, K1S, K1S, K2S; manifes `data/tahap_stop.txt`). Semua yang lain adalah pengembangan.

## 1. Bangku ukur dalam satu gambar

```
                        PC (tools/)
        ┌───────────────┬──────────────┬───────────────────┐
        │ konsol CH340  │ USB CDC      │ ST-Link '$'       │
        │ a / b / x     │ CSV + perintah lewat nilai baud  │ (flash saja, dicabut saat ukur)
        ▼               ▲              ▼
   ┌──────────┐    ┌──────────┐    ┌────────────────────┐
   │ ATTACKER │    │  LOGGER  │    │ HOST (K1S / K2S)   │
   │ F030C8   │    │ F103C8   │    │ F103C8             │
   │          │    │ 72 MHz   │    │ 48 MHz, Stop mode  │
   └────┬─────┘    └──┬──┬──┬─┘    └───┬────┬────┬──────┘
        │ frame 36 B  │  │  │          │    │    │
        └────────────────────────────► PA3 (USART2 RX host)
                      │  │  └─ PA0 ◄──── PB1 (pin status CPU: 1 = aktif)
                      │  └──── PA9/PA10 ◄──► PA10/PA9 (jalur kendali run, 115200)
                      └─ I2C1 ─► INA226 ─ shunt ─ 5 V ─► catu host (arus yang diukur)
```
Pinout lengkap: `docs/pinout_arsitektur_pengujian.md`.

## 2. Satu run, langkah demi langkah

Dikendalikan `tools/1_akuisisi/run_bench.py` (lihat docstring-nya):

1. PC menghentikan attacker (`x`), lalu memberi perintah ke logger (nilai baud = 1.000.000 + perintah x 1000 + argumen).
2. Logger mulai merekam INA226 dan tepi PB1, lalu meneruskan `S<detik>` ke host lewat USART1.
3. Host membalas `>START` dan membuka jendela pengamatan; PC menyalakan attacker (`a` = 1 Hz, `b` = 10 Hz).
4. Selama jendela, host tidur di **Stop mode** dan bangun oleh frame attacker; ia menandatangani lalu tidur lagi.
5. Host menutup dengan `>END ... >L ... >DONE`; logger mengalirkan CSV (`s` = cuplikan arus, `e` = tepi pin, `h` = baris host).
6. PC menyimpan CSV mentah di `data/runs/<sesi>/` lalu `analyse()` menghitung metrik per run.

## 3. Firmware: urutan membaca per board

### Attacker (`src/attacker/`)
1. `src/attacker/main.cpp`: pewaktu TIM3 mengirim frame 36 B (mode A 1/s, B 10/s); konsol `a b x s f`.
2. Format frame: `lib/bench_proto/proto_attacker_host.h` (dipakai juga oleh host).

### Logger (`src/logger/`) — satu folder
1. `src/logger/f103/main.cpp`: **alur utama**. Jam 72 MHz dari kristal, mesin status run, perintah via baud, keluaran CSV.
2. `src/logger/modul/ina226.*`: driver INA226 (I2C).
3. `src/logger/modul/l3_capture.*`: cuplik arus kontinu berstempel waktu.
4. `src/logger/modul/edge_stats.*`: statistik tepi pin status (HIGH, LOW, periode).
5. `src/logger/modul/ina_report.*`: laporan teks INA226.
6. Protokol baris ke host: `lib/bench_proto/proto_host_logger.h`.

### Host K1S (`src/host/final/k1_stop/`) dan K2S (`src/host/final/k2_stop/`)
Baca dalam urutan ini (K1S dan K2S memakai kerangka yang sama; beda hanya cara menandatangani):
1. `src/host/final/bersama/stop_common.h`: RTC dari LSE, masuk/keluar Stop, pemulihan jam 48 MHz, penerima frame attacker.
2. `src/host/final/bersama/stop_link.h`: kendali run lewat USART1 (`?`, `S`, `>START`, `>END`, `>L`, `>DONE`).
3. `src/host/final/bersama/stop_window.h`: mesin jendela pengamatan (alarm RTC, `serve_one`, T_obs/T_sleep).
4. **K1S** `k1_stop/main.cpp`: ECDSA P-256 software (Mbed TLS, konfigurasi `include/mbedtls_k1_config.h`).
   **K2S** `k2_stop/main.cpp` + `bersama/ds28e38_od.h`: DS28E38 overdrive, menunggu chip di Stop.
5. `src/host/bersama/host_util.h`: pencacah siklus DWT dan pengurai angka (dipakai final dan dev).

Flag `DIAG_SIG`, `DIAG_LONG`, `K1S_AUTO`, `K2S_AUTO`, `DBG_RX` adalah `#ifdef` di berkas yang sama. **Semuanya mati** di
env final; menyalakannya menghasilkan env "pendukung" (`platformio.ini`, KELOMPOK 2).

## 4. Skrip PC: urutan dari data ke kesimpulan

| Tahap | Folder | Skrip utama | Menghasilkan |
|---|---|---|---|
| 1 Akuisisi | `tools/1_akuisisi/` | `run_bench.py plan ...`, `flash_stop.py` | `data/runs/<sesi>/*.csv` mentah |
| 2 Kalibrasi | `tools/2_kalibrasi/` | `kalibrasi_shunt.py`, `kalibrasi_param.py` | R_eff 0,0821 Ohm, offset -2,83, u 1,5 % |
| 3 Analisis | `tools/3_analisis/` | `analisis_d.py --stop` lalu `dekomposisi_energi.py`, `analisis_sensitivitas.py`, `proyeksi_baterai.py`, `gambar_bab4_stop.py`, `ekspor_gambar_naskah.py` | `data/analisis/stop_final/` |
| 4 Verifikasi | `tools/4_verifikasi/` | `cek_biner.py`, `manifes_firmware.py`, `b1_verifikasi.py`, `diag_long.py`, `uji_galat_stop.py` | bukti kebenaran |

Satu perintah untuk seluruh alur analisis: `python tools/3_analisis/jalankan_analisis.py` (`--kering` mencetak urutan).
Isinya, dari akar repo:
```
python tools/3_analisis/analisis_d.py --manifest data/tahap_stop.txt --stop --keluar data/analisis/stop_final
python tools/3_analisis/dekomposisi_energi.py
python tools/3_analisis/analisis_sensitivitas.py
python tools/3_analisis/proyeksi_baterai.py
python tools/3_analisis/gambar_bab4_stop.py
python tools/3_analisis/ekspor_gambar_naskah.py
```
Data mentah menyimpan nilai register INA226; kalibrasi diterapkan saat analisis, jadi hasil dapat dihitung ulang tanpa
mengambil data lagi. Default kalibrasi ada di satu tempat: `tools/2_kalibrasi/kalibrasi_param.py`.

## 5. Bukti bahwa ini kode yang dipakai pada data

Build BERSIH dari sumber ini menghasilkan `firmware.bin` dengan sha256 yang sama dengan manifes data 18 Nov
(`data/manifes_firmware_stop.md`). Keterbatasan yang jujur: manifes mencatat bahwa bukti bahwa biner itulah yang ter-flash saat sesi
bersifat tidak langsung (sumber tidak lebih baru dari biner, dan build-id logger `4C364231` di tiap `#RUN,BEGIN`); tidak ada sha256
yang dicatat saat sesi. Yang dibuktikan di sini: sumber ini membangun biner yang sama dengan yang dicatat manifes.

| Env | sha256 firmware.bin (awal) |
|---|---|
| `host_k1_stop` | `2ca6e8d0d861...` |
| `host_k2_stop` | `5b4e017827e3...` |
| `logger` | `7ac6b702300d...` |
| `attacker` | `faf092a1fff6...` |

Acuan di `tools/4_verifikasi/biner_acuan.json` kini mencakup SEMUA 29 env (termasuk dev dan pendukung; dibuktikan identik
dengan build struktur sebelum pemindahan).

Periksa sendiri (membangun dari bersih, beberapa menit):
`python tools/4_verifikasi/cek_biner.py banding host_k1_stop host_k2_stop logger attacker`

Analisis: `python tools/4_verifikasi/cek_reproduksi_analisis.py` menjalankan `analisis_d.py --stop` ke folder sementara dan
membandingkannya dengan `data/analisis/stop_final/` (CSV, MD, gambar identik; stempel waktu `ringkasan.md` diabaikan).
Pemisahan: `python tools/4_verifikasi/cek_pemisahan.py` memastikan kode final tidak meng-`#include`/mengimpor apa pun dari `dev/`.

## 6. Yang BUKAN bagian data akhir (pengembangan)

| Kelompok | Lokasi | Isi |
|---|---|---|
| Tangga menuju Stop | `src/host/dev/tahap_stop/`, `tools/dev/tahap_stop/` | S0a LSE, S0 lantai Stop, S1 alarm RTC, S2 terima frame, overdrive DS28E38 (`ds_od`), pengendali otomatis `k1s_run.py` |
| Firmware tahap D (tanpa Stop) | `src/host/dev/tahap_d/`, `tools/dev/tahap_d/` | `k1_sw`, `k2_hw` (WFI), uji PC `k1_check`, `k2_check`, emulator siklus |
| Eksperimen awal | `src/host/dev/eksperimen/` | monolit, sederhana, softwaretes (micro-ecc dan Mbed TLS heavy), `k1_rx` |
| Varian pendukung | `platformio.ini` KELOMPOK 2 | `*_auto`, `*_diag_auto`, `*_long_auto`, `*_comb*`, `*_dbg`, `logger_selftest`. Firmwarenya pendukung; skrip PC-nya (`b1_verifikasi`, `diag_long`, `uji_galat_stop`) ada di `tools/4_verifikasi/` karena memverifikasi firmware final. |
| Demo sidang | `tools/demo/` | `demo_sidang.py` |

Kode pendukung pun tidak mengubah angka utama; ia hanya bukti tambahan (B1 verifikasi tanda tangan, B2 sensitivitas
konfigurasi software, diagnosis frame hilang, uji galat).

## 7. Peta path lama ke baru
Lihat `docs/PEMETAAN_PATH.md`. Hanya `logbook/`, `laporan/`, dan catatan harian `docs/belajar/2026-11-*.md` yang sengaja tidak ditulis ulang (riwayat bertanggal); path di dalamnya
merujuk struktur lama. Dokumen hidup dan komentar kode sudah diperbarui.
