# Skripsi: ECDSA P-256 software vs hardware offloading (DS28E38) di STM32F103

Bangku uji dengan tiga board:

| Board | MCU | Peran | Env pengukuran | Sumber |
|---|---|---|---|---|
| **Host** | STM32F103C8 (Blue Pill), HSI 48 MHz, ST-Link `$` | Menandatangani permintaan: K1/K3 software (Mbed TLS), K2/K4 hardware (DS28E38, 1-Wire PA15) | `k1_sw`, `k2_hw` | `src/host/k1_software/`, `src/host/k2_hardware/` + `src/host_common/` |
| **Logger** | STM32F103C8 klon, HSE 72 MHz, ST-Link `X` | INA226 (arus/daya), tangkap tepi pin status host, kendali run ke host, CSV ke PC | `logger` | `src/logger/f103/` + `src/logger_common/` |
| **Attacker** | STM32F030C8 | Pembangkit permintaan: frame 36 B ke host (A = 1/s, B = 10/s) | `attacker` | `src/attacker/` |

Pinout dan arsitektur bangku: `docs/pinout_arsitektur_pengujian.md`.

## Struktur kode

```
src/
  host/k1_software/      K1 software (Mbed TLS lokal di lib/mbedtls, konfigurasi include/mbedtls_k1_config.h)
  host/k2_hardware/      K2 hardware (DS28E38)
  host/eksperimen/       firmware uji awal, bukan pengukuran: k1_rx, monolit, sederhana,
                         softwaretes (micro-ecc), softwaretes_mbedtls
  host_common/           kode bersama K1/K2, header-only (waktu, RAM, jalur logger, penerima attacker)
  logger/f103/           firmware logger
  logger_common/         kode bersama logger (driver INA226, cuplik L3, statistik tepi, laporan)
  attacker/              firmware attacker
lib/
  bench_proto/           protokol antar-board, header-only: frame attacker->host, baris host<->logger
  mbedtls/               Mbed TLS 3.6.7
include/                 konfigurasi Mbed TLS
boards/                  definisi board kustom STM32F030C8
tools/                   skrip PC (lihat di bawah)
```

`platformio.ini` dikelompokkan per board: bagian dasar `[board_host]`, `[board_logger]`, `[board_attacker]`, lalu env firmware `extends` bagian itu.

## Build dan upload

```
pio run -e k1_sw                 # build
pio run -e k1_sw -t upload       # upload host lewat ST-Link '$'
pio run -e logger -t upload      # upload logger lewat ST-Link 'X'
pio run -e attacker -t upload    # ST-Link tanpa nomor seri: cabut ST-Link host dan logger dulu
```

Upload host dan logger dikunci ke nomor seri ST-Link, jadi tidak bisa menimpa board lain. Setelah flash lewat OpenOCD, DBG_SLEEP tetap menyala sampai power-on reset: sebelum mengukur arus jalankan `python tools/run_bench.py nodebug` atau cabut-pasang daya host.

## Alat PC (`tools/`)

| Skrip | Fungsi |
|---|---|
| `run_bench.py` | Pengendali run tahap D (logger, host, attacker) |
| `analisis_d.py` | Analisis statistik data tahap D (Bab 3) |
| `cek_biner.py` | Gerbang reproduksi biner (lihat di bawah) |
| `k1_check.py`, `k2_check.py` | Uji K1/K2 lewat USB: verifikasi setiap tanda tangan di PC |
| `uji_galat.py` | Uji jalur galat protokol logger<->host lewat OpenOCD |
| `a3_clock.py` | Galat jam HSI host terhadap kristal logger |
| `p256.py`, `rfc6979_ref.py` | Referensi ECDSA P-256 murni Python |
| `ecdsa_bench.py`, `bench_message.py`, `window_test.py` | Benchmark firmware eksperimen awal |

## Aturan saat mengubah kode

Data tahap D diambil dengan biner tertentu (`k1_sw` `c55d8ca31cc0`, `k2_hw` `4df67c4f9975`, `logger` `7ac6b702300d`). Perapian atau refaktor yang tidak dimaksudkan mengubah perilaku harus menghasilkan `firmware.bin` identik:

```
python tools/cek_biner.py banding          # semua env harus IDENTIK
```

Bila firmware sengaja diubah: `python tools/cek_biner.py simpan <env>` dan catat alasannya di logbook.

Hal yang diketahui menggeser tata letak Flash walau kodenya sama:
- kode bersama yang punya `.cpp` ditaruh di `lib/` (objek pustaka ditautkan dalam grup terpisah); header-only di `lib/` aman;
- `lib_archive = no` (ikut mengubah penautan `Wire`);
- memindah berkas di `src/` sehingga urutan path berubah (objek `src/` ditautkan urut nama path).

Catatan harian ada di `logbook/`.
