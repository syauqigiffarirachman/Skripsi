# Skripsi: ECDSA P-256 software vs hardware offloading (DS28E38) di STM32F103

Terakhir diperbarui: 2026-11-24. **Mulai dari `docs/PETA_ALUR.md`**: peta dari firmware sampai kesimpulan.

## Yang mana kode final?

Data akhir skripsi = varian **Stop mode** yang diambil 18 Nov 2026 (4 blok ABBA, K2S-K1S-K1S-K2S; `data/tahap_stop.txt`).
Hanya empat firmware yang dipakai, dan keempatnya dapat dibuktikan identik dengan manifes data (`data/manifes_firmware_stop.md`):

| Board | MCU | Peran | Env FINAL | Sumber FINAL |
|---|---|---|---|---|
| **Host** | STM32F103C8 (Blue Pill), HSI 48 MHz, Stop mode | Menandatangani permintaan: K1S software (Mbed TLS), K2S hardware (DS28E38 overdrive) | `host_k1_stop`, `host_k2_stop` | `src/host/final/` |
| **Logger** | STM32F103C8 klon, HSE 72 MHz | INA226 (arus/daya), tangkap tepi pin status host, kendali run, CSV ke PC | `logger` | `src/logger/` (satu folder) |
| **Attacker** | STM32F030C8 | Pembangkit permintaan: frame 36 B (A = 1/s, B = 10/s) | `attacker` | `src/attacker/` |

Selain itu ada **kode pengembangan** (tangga menuju Stop, firmware tahap D tanpa Stop, eksperimen awal) yang sengaja
dipisah di `src/host/dev/` dan `tools/dev/`. Pinout dan arsitektur bangku: `docs/referensi/pinout_arsitektur_pengujian.md`.

## Peta dokumen

| Untuk | Buka |
|---|---|
| **Belajar** (konsep, kode, hasil, kuis) | `docs/belajar/` (mulai `00_peta_belajar.md`) |
| **Menulis/merevisi skripsi** (bahan paper) | `laporan/` (mulai `laporan/README.md`; naskah terbaru: `3_revisi_naskah/perubahan_skripsi_v4_kalibrasi_2026-11-21.md`) |
| **Memahami alur kode** | `docs/PETA_ALUR.md` |
| **Bangku ukur dan prosedur** | `docs/referensi/` dan `docs/prosedur/` |
| **Riwayat harian** | `logbook/` |

Peta lengkap: `docs/README.md`. Path lama ke baru (kode dan dokumen): `docs/referensi/PEMETAAN_PATH.md`.

## Struktur kode

```
src/
  host/
    final/                 FINAL: k1_stop (K1S), k2_stop (K2S), bersama/ (stop_common, stop_link, stop_window, ds28e38_od)
    bersama/host_util.h    dipakai final DAN dev (DWT, parse)
    dev/                   PENGEMBANGAN, tidak ada di data akhir:
      tahap_stop/            s0a_lse, s0_stop, s1_rtc, s2_rx (tangga menuju Stop)
      tahap_d/               k1_software, k2_hardware (WFI) + bersama/ (host_link, host_ram, host_rx, host_time)
      eksperimen/            ds_overdrive, k1_rx, monolit, sederhana, softwaretes, softwaretes_mbedtls
  logger/                  FINAL, satu folder: f103/main.cpp (alur) + modul/ (INA226, cuplik, tepi, laporan)
  attacker/                FINAL
lib/
  bench_proto/             kontrak protokol 3 board, header-only: frame attacker->host, baris host<->logger
  mbedtls/                 Mbed TLS 3.6.7 (dipakai K1S)
include/                   mbedtls_k1_config.h (FINAL); dev/ = konfigurasi dan header uji (bench.h, powertest.h, ...)
boards/                    definisi board kustom STM32F030C8
tools/                     skrip PC menurut aliran data: 1_akuisisi 2_kalibrasi 3_analisis 4_verifikasi demo dev (lihat tools/README.md)
tests/                     uji unit alat PC (unittest, tanpa perangkat keras)
data/                      runs/ (CSV mentah), analisis/stop_final/ (hasil), kalibrasi/, manifes
docs/ laporan/ logbook/    dokumen menurut audiens (lihat docs/README.md): docs/ = teknis + belajar, laporan/ = bahan paper skripsi, logbook/ = jurnal harian
ds9481_comm/               pustaka Python/C untuk bicara ke DS28E38/DS2476 (pengembangan awal)
```

`platformio.ini` dikelompokkan: KELOMPOK 1 FINAL (4 env), KELOMPOK 2 PENDUKUNG (kode final + satu flag: `*_auto`,
`*_diag_auto`, `*_long_auto`, `*_comb*`, `*_dbg`, `logger_selftest`), KELOMPOK 3 DEV. Nama env tidak berubah.

## Build dan upload

```
pio run -e host_k1_stop         # build (FINAL; flash host Stop: lihat bagian Stop di bawah)
pio run -e k1_sw -t upload       # (dev, tahap D) upload host lewat ST-Link '$'
pio run -e logger -t upload      # upload logger lewat ST-Link 'X'
pio run -e attacker -t upload    # ST-Link tanpa nomor seri: cabut ST-Link host dan logger dulu
```

Upload host dan logger dikunci ke nomor seri ST-Link, jadi tidak bisa menimpa board lain. Setelah flash lewat OpenOCD, DBG_SLEEP tetap menyala sampai power-on reset: sebelum mengukur arus jalankan `python tools/1_akuisisi/run_bench.py nodebug` atau cabut-pasang daya host.

## Varian deep sleep (Stop mode)

Tahap D (dev) memakai tidur Sleep (WFI), IDLE ~11,6 mA. Varian Stop (FINAL) mengganti jendela pengamatan dengan Stop + alarm RTC
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
- **SWD tidak bisa tersambung selama Stop.** Flash lewat `tools/1_akuisisi/flash_stop.py` (mencoba berulang sampai inti kebetulan
  bangun), bukan `pio run -t upload`:

```powershell
python tools/1_akuisisi/flash_stop.py .pio/build/host_k1_stop/firmware.elf
```

- Setelah flash, DBGMCU_CR = 0x307 menahan Stop sampai firmware menolkannya (jendela aman 5 s setelah boot) atau power-on reset.
- Host terkunci (SWD mati dan flash_stop gagal): pasang jumper BOOT0 = 1, cabut-pasang daya, lalu
  `python tools/dev/tahap_stop/pulihkan_host.py [firmware.elf]`. Setelah selesai BOOT0 = 0 dan cabut-pasang daya lagi.
- Jangan probe langsung PC14/PC15 (kapasitansi probe menghentikan LSE) dan jangan beri 3V3 eksternal bersamaan dengan 5 V.
- Ukur arus Stop **tanpa ST-Link** dan setelah POR bersih: ST-Link menyumbang ~0,58 mA.
- Resistor seri jalur sinyal antar-board adalah **10 kOhm** sejak 17 Nov (logger PA9 -> host PA10, attacker PA9 -> host PA3);
  tahap D memakai ~1 kOhm. Pinout lengkap (diperbarui 18 Nov, termasuk bagian 2b host Stop): `docs/referensi/pinout_arsitektur_pengujian.md`.
- Status keandalan bangun-dari-Stop (frame rusak/hilang pada K2S barrage) dan keputusan terbuka: `docs/review/laporan_review_kode_2026-11-18.md` (H1, H2).

## Alat PC (`tools/`)

Disusun menurut aliran data: `1_akuisisi` (ambil data) -> `2_kalibrasi` -> `3_analisis` (tabel, gambar) -> `4_verifikasi`
(bukti). `demo/` untuk sidang, `dev/` untuk pengembangan. Rincian per skrip: `tools/README.md`. Uji PC bangku tanpa
perangkat keras: `python -m unittest discover -s tests`.

## Aturan saat mengubah kode

Data akhir (Stop, 18 Nov 2026) diambil dengan biner `host_k1_stop` `2ca6e8d0d861`, `host_k2_stop` `5b4e017827e3`, `logger` `7ac6b702300d`, `attacker` `faf092a1fff6`; data tahap D dengan `k1_sw` `c55d8ca31cc0`, `k2_hw` `4df67c4f9975`, `logger` `7ac6b702300d`. Perapian atau refaktor yang tidak dimaksudkan mengubah perilaku harus menghasilkan `firmware.bin` identik:

```
python tools/4_verifikasi/cek_biner.py banding          # semua env harus IDENTIK
python tools/4_verifikasi/cek_pemisahan.py              # kode final tidak boleh bergantung pada dev/
```

Bila firmware sengaja diubah: `python tools/4_verifikasi/cek_biner.py simpan <env>` dan catat alasannya di logbook.

Hal yang diketahui menggeser tata letak Flash walau kodenya sama:
- kode bersama yang punya `.cpp` ditaruh di `lib/` (objek pustaka ditautkan dalam grup terpisah); header-only di `lib/` aman;
- `lib_archive = no` (ikut mengubah penautan `Wire`);
- memindah berkas di `src/` sehingga urutan path berubah (objek `src/` ditautkan urut nama path).

## Hook pre-commit

Pemeriksaan cepat sebelum tiap commit (sintaks skrip, pemisahan final/dev, tautan dokumen, 55 uji unit; < 10 detik) ada di `.githooks/pre-commit`.
Aktifkan sekali per klon: `git config core.hooksPath .githooks`. Bila sumber firmware berubah, hook mengingatkan agar menjalankan
gerbang biner; `CEK_BINER=1 git commit ...` menjalankannya di hook (beberapa menit). Darurat: `git commit --no-verify`.
(Husky/Prettier tidak dipakai: repo ini bukan proyek Node dan Prettier akan memformat ulang JSON/Markdown yang hash-nya dijaga.)

Istilah proyek: `GLOSSARY.md`. Keputusan desain yang sulit dibalik: `docs/adr/`.

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
ditulis di samping `firmware.bin`: `tools/dev/provenance.py` (logika, teruji) dan `tools/dev/extra_script_provenance.py` (hook SCons).
Hook **belum didaftarkan** di `platformio.ini` dan belum pernah dijalankan oleh PlatformIO; petunjuk pendaftaran ada di
kepala berkasnya. Perlu diingat: mengubah `platformio.ini` membuat PlatformIO menghapus semua folder build.
Sampai hook aktif, `sesi.txt` hanya mencatat hash ELF untuk sesi campuran K1/K2, jadi catat hash `firmware.bin` biner data final
di logbook (lihat temuan H1 di laporan review).

Catatan harian ada di `logbook/`. Bahan belajar: `docs/belajar/`. Review kode: `docs/review/`.
