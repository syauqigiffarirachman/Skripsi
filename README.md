# Skripsi: ECDSA P-256 software vs hardware offloading (DS28E38) di STM32F103

Terakhir diperbarui: 2026-11-18 11:35 WIB (UTC+7)

## Status

| Bagian | Status |
|---|---|
| Tahap D (K1-K4, tidur Sleep/WFI @ 48 MHz, 120 run) | selesai; data dan analisis di `data/` dan `tools/analisis_d.py` |
| Varian deep sleep (Stop mode, K1S/K2S) | **dalam pengerjaan**: langkah 4-9 lulus, pilot dan perbaikan keandalan berjalan, data final (langkah 12) belum diambil. Rincian: `docs/rencana_deep_sleep.md` |
| Laporan, revisi Bab 3, panduan grafik | `laporan/` |
| Review kode (18 Nov) | `docs/review/laporan_review_kode_2026-11-18.md` |

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
                         softwaretes (micro-ecc), softwaretes_mbedtls, ds_overdrive (DS28E38 overdrive)
  host/deepsleep/        varian Stop mode: stop_common.h (RTC/LSE, jam, penerima frame, tidur Stop),
                         stop_window.h (jendela pengamatan), stop_link.h (kendali run), ds28e38_od.h,
                         k1_stop, k2_stop (K1S/K2S) dan uji bertahap s0a_lse, s0_stop, s1_rtc, s2_rx
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
tests/                   uji unit alat PC (unittest, tanpa perangkat keras)
ds9481_comm/             pustaka Python dan C untuk bicara ke DS28E38/DS2476 lewat adaptor DS9481P-300
docs/                    pinout, rencana deep sleep, bahan belajar (docs/belajar), review kode (docs/review)
laporan/                 laporan progres, revisi Bab 3, panduan grafik Bab 4
logbook/                 catatan harian
requirements-analisis.txt  versi paket Python untuk analisis (venv .venv-analisis)
extra_script_provenance.py hook pasca-build pencatat asal-usul biner (belum didaftarkan di platformio.ini)
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

## Varian deep sleep (Stop mode)

Tahap D memakai tidur Sleep (WFI), IDLE ~11,6 mA. Varian Stop mengganti jendela pengamatan dengan Stop + alarm RTC
(LSE 32,768 kHz) dan bangun oleh frame attacker; lantai papan ~2 mA (didominasi LED PWR, diterima). Data tahap D
**tidak** diganti; Stop ditambahkan sebagai varian.

| Env | Fungsi |
|---|---|
| `host_s0a_lse`, `host_s0a_lse_dingin` | uji kristal LSE dan start dingin |
| `host_s0_stop` | lantai arus papan di Stop |
| `host_s1_rtc` | Stop + bangun alarm RTC |
| `host_s2_rx`, `host_s2_trace` | bangun oleh frame attacker, terima utuh (trace = jejak diagnosis) |
| `host_k1_stop`, `host_k1_stop_auto` | K1S: Mbed TLS + Stop; `_auto` = jendela berulang tanpa perintah logger |
| `host_k2_stop`, `host_k2_stop_auto`, `host_k2_stop_dbg` | K2S: DS28E38 overdrive + polling + Stop; `_dbg` = diagnostik frame lewat SWD (`g_d_*`) |
| `host_ds_od`, `host_ds_od_stress` | eksperimen DS28E38 overdrive (uji ketahanan 100 tanda tangan) |

Aturan khusus saat host di Stop:
- **SWD tidak bisa tersambung selama Stop.** Flash lewat `tools/flash_stop.py` (mencoba berulang sampai inti kebetulan
  bangun), bukan `pio run -t upload`:

```powershell
python tools/flash_stop.py .pio/build/host_k1_stop/firmware.elf
```

- Setelah flash, DBGMCU_CR = 0x307 menahan Stop sampai firmware menolkannya (jendela aman 5 s setelah boot) atau power-on reset.
- Host terkunci (SWD mati dan flash_stop gagal): pasang jumper BOOT0 = 1, cabut-pasang daya, lalu
  `python tools/pulihkan_host.py [firmware.elf]`. Setelah selesai BOOT0 = 0 dan cabut-pasang daya lagi.
- Jangan probe langsung PC14/PC15 (kapasitansi probe menghentikan LSE) dan jangan beri 3V3 eksternal bersamaan dengan 5 V.
- Ukur arus Stop **tanpa ST-Link** dan setelah POR bersih: ST-Link menyumbang ~0,58 mA.
- Resistor seri jalur sinyal antar-board adalah **10 kOhm** sejak 17 Nov (logger PA9 -> host PA10, attacker PA9 -> host PA3);
  tahap D memakai ~1 kOhm. Pinout lengkap (diperbarui 18 Nov, termasuk bagian 2b host Stop): `docs/pinout_arsitektur_pengujian.md`.
- Status keandalan bangun-dari-Stop (frame rusak/hilang pada K2S barrage) dan keputusan terbuka: `docs/review/laporan_review_kode_2026-11-18.md` (H1, H2).

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
| `provenance.py` | Catatan asal-usul biner (hash `firmware.bin`, git HEAD, status kotor) ditulis di luar firmware |
| `flash_stop.py`, `pulihkan_host.py` | Flash/baca host yang sedang di Stop; pemulihan host terkunci lewat BOOT0 |
| `k1s_run.py` | Satu run K1S/K2S dengan host mode otomatis (`*_auto`), lalu analisis dengan `run_bench.analyse` |
| `s0a_lse.py`, `s0_stop.py`, `s1_rtc.py`, `s2_rx.py`, `s2_dump.sh` | Uji bertahap deep sleep (LSE, lantai arus Stop, alarm RTC, bangun oleh frame) |
| `ds_od.py` | Uji DS28E38 overdrive + polling; memverifikasi tiap tanda tangan di PC |

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

## Uji alat PC dan lingkungan analisis

Lingkungan: `python -m venv .venv-analisis`, lalu `.venv-analisis/Scripts/python -m pip install -r requirements-analisis.txt`
(versi disalin dari venv yang terpasang 18 Nov, bukan hasil `pip freeze`; pin versi tidak memverifikasi integritas unduhan).

```powershell
.venv-analisis/Scripts/python -B -m unittest discover -s tests -v
```

Uji memakai data sintetis dan tidak membuka port serial. Cakupan: `parse`, `layanan` (TL, model kejenuhan),
`energi_sesi` (P_dyn, E_lyn), `analyse` (aturan sah/gugur), dan `provenance`. Belum ada uji untuk `analisis_d.py`.

## Provenance biner (asal-usul data)

Catatan hash dan commit **tidak** ditanam ke firmware (itu mengubah biner dan merusak gerbang `cek_biner.py`), melainkan
ditulis di samping `firmware.bin`: `tools/provenance.py` (logika, teruji) dan `extra_script_provenance.py` (hook SCons).
Hook **belum didaftarkan** di `platformio.ini` dan belum pernah dijalankan oleh PlatformIO; petunjuk pendaftaran ada di
kepala berkasnya. Perlu diingat: mengubah `platformio.ini` membuat PlatformIO menghapus semua folder build.
Sampai hook aktif, `sesi.txt` hanya mencatat hash ELF untuk sesi campuran K1/K2, jadi catat hash `firmware.bin` biner data final
di logbook (lihat temuan H1 di laporan review).

Catatan harian ada di `logbook/`. Bahan belajar: `docs/belajar/`. Review kode: `docs/review/`.
