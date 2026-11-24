# Peta Belajar Skripsi (bahan sebelum 18 Nov 2026)

Dibuat 18 Nov 2026 dari: README, `docs/`, `laporan/`, `logbook/2026-11-10` s.d. `2026-11-17`, dan kode utama.
Hanya sekali ini disusun mundur. Mulai malam ini, catatan harian otomatis (jam 21.00) masuk ke
`docs/belajar/YYYY-MM-DD.md` dan membahas progres hari itu saja.

Progres 18 Nov (hasil Stop mode K1S/K2S, overdrive DS28E38, crash pasca-Stop dan NOP sled) sengaja TIDAK
dibahas di sini. Itu bahan catatan malam ini.

## Satu kalimat inti skripsi

> Saat perangkat IoT dibanjiri permintaan tanda tangan (serangan Denial of Sleep), apakah menyerahkan ECDSA ke chip
> DS28E38 (hardware) lebih hemat energi, lebih banyak tidur, dan tetap melayani lebih banyak permintaan
> dibanding menghitungnya sendiri di CPU STM32 (software, Mbed TLS)?

Jawaban terukur (tahap D, 120 run): latensi turun 69 %, daya dinamis turun 77 % (normal) / 34 % (barrage),
CPU tidur jauh lebih lama, dan layanan saat barrage naik dari 15 % ke 49 %.

## Urutan membaca yang disarankan

| No | Berkas | Isi | Waktu |
|---|---|---|---|
| 1 | `01_konsep_dasar.md` | ECDSA, DS28E38, 1-Wire, tidur CPU, timer, INA226, metrik, statistik | 60-90 mnt |
| 2 | `02_arsitektur_bangku_uji.md` | 3 board, kabel, protokol, alur satu run | 30 mnt |
| 3 | `03_penjelasan_kode.md` | Kode inti dibahas bagian demi bagian beserta alasan desainnya | 90 mnt |
| 4 | `04_pelajaran_per_hari.md` | Kronologi 10-17 Nov: apa, mengapa, pelajaran dari tiap bug | 45 mnt |
| 5 | `05_hasil_dan_cara_membaca.md` | Angka final, model kejenuhan, klaim yang boleh dan tidak boleh | 30 mnt |
| 6 | `06_latihan_dan_kuis.md` | 25 soal dengan jawaban, untuk persiapan sidang | 45 mnt |

## Peta berkas penting (jalan pintas)

| Mau paham | Buka |
|---|---|
| Tanda tangan software | `src/host/dev/tahap_d/k1_software/main.cpp` (`sign_digest`, `harvest_entropy`) |
| Tanda tangan hardware | `src/host/dev/tahap_d/k2_hardware/main.cpp` (`xpc`, `sign_digest`, `wait_ms`) |
| Cara CPU tidur dan diukur | `src/host/dev/tahap_d/bersama/host_time.h`, loop di `cmd_window` K1 |
| Penerimaan frame attacker | `src/host/dev/tahap_d/bersama/host_rx.h`, `lib/bench_proto/proto_attacker_host.h` |
| Kendali run PC-logger-host | `lib/bench_proto/proto_host_logger.h`, `src/host/dev/tahap_d/bersama/host_link.h`, `tools/run_bench.py` |
| Konfigurasi Mbed TLS | `include/mbedtls_k1_config.h` |
| Stop mode (deep sleep) | `docs/rencana_deep_sleep.md`, `src/host/final/bersama/stop_window.h` |
| Statistik akhir | `tools/analisis_d.py` |
| Pinout dan kabel | `docs/pinout_arsitektur_pengujian.md` |

## Tiga kebiasaan kerja proyek ini (patut ditiru)

1. **Build from base**: satu fungsi per tahap, dites dan distabilkan sebelum menambah fungsi.
2. **Gerbang biner identik**: setelah merapikan kode, `firmware.bin` harus sama bit demi bit (`tools/cek_biner.py banding`),
   supaya data yang sudah diambil tetap sah.
3. **Catat yang gagal dan yang salah**: logbook memuat dugaan yang terbukti SALAH beserta alasannya (mis. 13 Nov
   "comb tidak mengubah latensi" yang dicabut). Itu yang membuat hasilnya bisa dipercaya.

## Tambahan 21 Nov
- `08_alur_data_lengkap_2026-11-21.md`: **alur lengkap** dari kabel/firmware sampai kesimpulan (pengambilan data, kalibrasi INA226, analisis per run, tiap analisis statistik, penetapan kesimpulan), lengkap dengan berkas:baris, rumus, library, angka terbaru (R_eff 0,0821 Ohm), dan jebakan. Mulai dari sini bila ingin satu bacaan yang menyambung semuanya; `07` tetap berguna untuk pembuktian independen dan jawaban penguji, tetapi angkanya memakai shunt lama.

## Tambahan 19 Nov
- `07_dari_csv_ke_kesimpulan.md`: asal setiap angka dari CSV mentah sampai kesimpulan, kesenjangan paper-kode (Shapiro-Wilk, ANOVA), naskah menjawab penguji. Pembuktian independen: `tools/hitung_ulang_satu_run.py`.
