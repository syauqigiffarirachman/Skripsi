# Pemetaan path lama ke baru (24 Nov 2026): kode dan dokumen

Bagian 1 = kode (firmware dan skrip PC). Bagian 2 = dokumen (`docs/`, `laporan/`).

Pemindahan memisahkan kode FINAL (data 18 Nov) dari kode PENGEMBANGAN. Isi berkas tidak berubah selain path `#include`,
impor, dan komentar. Logbook, laporan, dan catatan belajar lama memakai path LAMA; cari di tabel ini.

## 1. Kode

| Path lama | Path baru |
|---|---|
| `extra_script_attacker.py` | `tools/dev/extra_script_attacker.py` |
| `extra_script_provenance.py` | `tools/dev/extra_script_provenance.py` |
| `include/bench.h` | `include/dev/bench.h` |
| `include/mbedtls_heavy_config.h` | `include/dev/mbedtls_heavy_config.h` |
| `include/mbedtls_k1_comb_config.h` | `include/dev/mbedtls_k1_comb_config.h` |
| `include/powertest.h` | `include/dev/powertest.h` |
| `monitor.py` | `tools/dev/monitor.py` |
| `src/host/deepsleep/ds28e38_od.h` | `src/host/final/bersama/ds28e38_od.h` |
| `src/host/deepsleep/k1_stop/main.cpp` | `src/host/final/k1_stop/main.cpp` |
| `src/host/deepsleep/k2_stop/main.cpp` | `src/host/final/k2_stop/main.cpp` |
| `src/host/deepsleep/s0_stop/main.cpp` | `src/host/dev/tahap_stop/s0_stop/main.cpp` |
| `src/host/deepsleep/s0a_lse/main.cpp` | `src/host/dev/tahap_stop/s0a_lse/main.cpp` |
| `src/host/deepsleep/s1_rtc/main.cpp` | `src/host/dev/tahap_stop/s1_rtc/main.cpp` |
| `src/host/deepsleep/s2_rx/main.cpp` | `src/host/dev/tahap_stop/s2_rx/main.cpp` |
| `src/host/deepsleep/stop_common.h` | `src/host/final/bersama/stop_common.h` |
| `src/host/deepsleep/stop_link.h` | `src/host/final/bersama/stop_link.h` |
| `src/host/deepsleep/stop_window.h` | `src/host/final/bersama/stop_window.h` |
| `src/host/eksperimen/ds_overdrive/main.cpp` | `src/host/dev/eksperimen/ds_overdrive/main.cpp` |
| `src/host/eksperimen/k1_rx/main.cpp` | `src/host/dev/eksperimen/k1_rx/main.cpp` |
| `src/host/eksperimen/monolit/main.cpp` | `src/host/dev/eksperimen/monolit/main.cpp` |
| `src/host/eksperimen/sederhana/sederhana.cpp` | `src/host/dev/eksperimen/sederhana/sederhana.cpp` |
| `src/host/eksperimen/sederhana/sederhanarequest.cpp` | `src/host/dev/eksperimen/sederhana/sederhanarequest.cpp` |
| `src/host/eksperimen/softwaretes/softwaretes.cpp` | `src/host/dev/eksperimen/softwaretes/softwaretes.cpp` |
| `src/host/eksperimen/softwaretes_mbedtls/README.md` | `src/host/dev/eksperimen/softwaretes_mbedtls/README.md` |
| `src/host/eksperimen/softwaretes_mbedtls/softwaretes_mbedtls.cpp` | `src/host/dev/eksperimen/softwaretes_mbedtls/softwaretes_mbedtls.cpp` |
| `src/host/k1_software/main.cpp` | `src/host/dev/tahap_d/k1_software/main.cpp` |
| `src/host/k2_hardware/main.cpp` | `src/host/dev/tahap_d/k2_hardware/main.cpp` |
| `src/host_common/host_link.h` | `src/host/dev/tahap_d/bersama/host_link.h` |
| `src/host_common/host_ram.h` | `src/host/dev/tahap_d/bersama/host_ram.h` |
| `src/host_common/host_rx.h` | `src/host/dev/tahap_d/bersama/host_rx.h` |
| `src/host_common/host_time.h` | `src/host/dev/tahap_d/bersama/host_time.h` |
| `src/host_common/host_util.h` | `src/host/bersama/host_util.h` |
| `src/logger_common/edge_stats.cpp` | `src/logger/modul/edge_stats.cpp` |
| `src/logger_common/edge_stats.h` | `src/logger/modul/edge_stats.h` |
| `src/logger_common/ina226.cpp` | `src/logger/modul/ina226.cpp` |
| `src/logger_common/ina226.h` | `src/logger/modul/ina226.h` |
| `src/logger_common/ina_report.cpp` | `src/logger/modul/ina_report.cpp` |
| `src/logger_common/ina_report.h` | `src/logger/modul/ina_report.h` |
| `src/logger_common/l3_capture.cpp` | `src/logger/modul/l3_capture.cpp` |
| `src/logger_common/l3_capture.h` | `src/logger/modul/l3_capture.h` |
| `tools/a3_clock.py` | `tools/dev/tahap_stop/a3_clock.py` |
| `tools/analisis_d.py` | `tools/3_analisis/analisis_d.py` |
| `tools/analisis_sensitivitas.py` | `tools/3_analisis/analisis_sensitivitas.py` |
| `tools/b1_verifikasi.py` | `tools/4_verifikasi/b1_verifikasi.py` |
| `tools/bench_message.py` | `tools/dev/tahap_d/bench_message.py` |
| `tools/biner_acuan.json` | `tools/4_verifikasi/biner_acuan.json` |
| `tools/cek_biner.py` | `tools/4_verifikasi/cek_biner.py` |
| `tools/dekomposisi_energi.py` | `tools/3_analisis/dekomposisi_energi.py` |
| `tools/demo_sidang.py` | `tools/demo/demo_sidang.py` |
| `tools/diag_long.py` | `tools/4_verifikasi/diag_long.py` |
| `tools/ds_od.py` | `tools/dev/tahap_stop/ds_od.py` |
| `tools/ecdsa_bench.py` | `tools/dev/tahap_d/ecdsa_bench.py` |
| `tools/ekspor_gambar_naskah.py` | `tools/3_analisis/ekspor_gambar_naskah.py` |
| `tools/emu_cycles/build.py` | `tools/dev/tahap_d/emu_cycles/build.py` |
| `tools/emu_cycles/cfg/cfg_noAsm.h` | `tools/dev/tahap_d/emu_cycles/cfg/cfg_noAsm.h` |
| `tools/emu_cycles/cfg/cfg_noComb.h` | `tools/dev/tahap_d/emu_cycles/cfg/cfg_noComb.h` |
| `tools/emu_cycles/cfg/cfg_noFixed.h` | `tools/dev/tahap_d/emu_cycles/cfg/cfg_noFixed.h` |
| `tools/emu_cycles/cfg/cfg_noNIST.h` | `tools/dev/tahap_d/emu_cycles/cfg/cfg_noNIST.h` |
| `tools/emu_cycles/common.h` | `tools/dev/tahap_d/emu_cycles/common.h` |
| `tools/emu_cycles/link.ld` | `tools/dev/tahap_d/emu_cycles/link.ld` |
| `tools/emu_cycles/run.py` | `tools/dev/tahap_d/emu_cycles/run.py` |
| `tools/emu_cycles/startup.c` | `tools/dev/tahap_d/emu_cycles/startup.c` |
| `tools/emu_cycles/test_mbedtls.c` | `tools/dev/tahap_d/emu_cycles/test_mbedtls.c` |
| `tools/emu_cycles/test_tinycrypt.c` | `tools/dev/tahap_d/emu_cycles/test_tinycrypt.c` |
| `tools/emu_cycles/test_uecc.c` | `tools/dev/tahap_d/emu_cycles/test_uecc.c` |
| `tools/flash_stop.py` | `tools/1_akuisisi/flash_stop.py` |
| `tools/gambar_bab4_stop.py` | `tools/3_analisis/gambar_bab4_stop.py` |
| `tools/hitung_ulang_satu_run.py` | `tools/3_analisis/hitung_ulang_satu_run.py` |
| `tools/k1_check.py` | `tools/dev/tahap_d/k1_check.py` |
| `tools/k1s_run.py` | `tools/dev/tahap_stop/k1s_run.py` |
| `tools/k2_check.py` | `tools/dev/tahap_d/k2_check.py` |
| `tools/kalibrasi_param.py` | `tools/2_kalibrasi/kalibrasi_param.py` |
| `tools/kalibrasi_shunt.py` | `tools/2_kalibrasi/kalibrasi_shunt.py` |
| `tools/manifes_firmware.py` | `tools/4_verifikasi/manifes_firmware.py` |
| `tools/p256.py` | `tools/4_verifikasi/p256.py` |
| `tools/provenance.py` | `tools/dev/provenance.py` |
| `tools/proyeksi_baterai.py` | `tools/3_analisis/proyeksi_baterai.py` |
| `tools/pulihkan_host.py` | `tools/dev/tahap_stop/pulihkan_host.py` |
| `tools/rfc6979_ref.py` | `tools/dev/tahap_d/rfc6979_ref.py` |
| `tools/run_bench.py` | `tools/1_akuisisi/run_bench.py` |
| `tools/s0_stop.py` | `tools/dev/tahap_stop/s0_stop.py` |
| `tools/s0a_lse.py` | `tools/dev/tahap_stop/s0a_lse.py` |
| `tools/s1_rtc.py` | `tools/dev/tahap_stop/s1_rtc.py` |
| `tools/s2_dump.sh` | `tools/dev/tahap_stop/s2_dump.sh` |
| `tools/s2_rx.py` | `tools/dev/tahap_stop/s2_rx.py` |
| `tools/uji_galat.py` | `tools/dev/tahap_d/uji_galat.py` |
| `tools/uji_galat_stop.py` | `tools/4_verifikasi/uji_galat_stop.py` |
| `tools/window_test.py` | `tools/dev/tahap_d/window_test.py` |

Berkas yang ditambah: `tools/_jalur.py` (jalur impor bersama), `tools/3_analisis/jalankan_analisis.py`, `tools/4_verifikasi/cek_pemisahan.py`, `tools/4_verifikasi/cek_reproduksi_analisis.py`, `tests/test_pemisahan.py`, `.githooks/pre-commit`, `GLOSSARY.md`, `docs/adr/`, `docs/PETA_ALUR.md`, `docs/PEMETAAN_PATH.md`, README per folder.


## 2. Dokumen

Dokumen yang dilebur menunjuk ke satu berkas baru; isinya utuh sebagai bagian di dalamnya.

| Path lama | Path baru |
|---|---|
| `docs/PEMETAAN_PATH.md` | `docs/referensi/PEMETAAN_PATH.md` |
| `docs/bahan_pembahasan_bab4_2026-11-18.md` | `laporan/2_hasil_dan_pembahasan/bahan_pembahasan_bab4_2026-11-18.md` |
| `docs/belajar/2026-11-18.md` | `docs/belajar/harian/2026-11-18.md` |
| `docs/belajar/2026-11-19.md` | `docs/belajar/harian/2026-11-19.md` |
| `docs/belajar/2026-11-20.md` | `docs/belajar/harian/2026-11-20.md` |
| `docs/belajar/2026-11-21.md` | `docs/belajar/harian/2026-11-21.md` |
| `docs/belajar/2026-11-22.md` | `docs/belajar/harian/README.md` |
| `docs/belajar/2026-11-23.md` | `docs/belajar/harian/README.md` |
| `docs/daftar_kekurangan_data_analisis_2026-11-18.md` | `laporan/5_status_dan_referensi/daftar_kekurangan_data_analisis_2026-11-18.md` |
| `docs/demo_sidang_panduan.md` | `docs/prosedur/demo_sidang_panduan.md` |
| `docs/diagnosis_frame_hilang_K2S_2026-11-18.md` | `laporan/2_hasil_dan_pembahasan/bukti_pendukung_B1_B2_C5_2026-11-18.md` |
| `docs/hasil_B1_verifikasi_tanda_tangan_2026-11-18.md` | `laporan/2_hasil_dan_pembahasan/bukti_pendukung_B1_B2_C5_2026-11-18.md` |
| `docs/hasil_B2_sensitivitas_comb_2026-11-18.md` | `laporan/2_hasil_dan_pembahasan/bukti_pendukung_B1_B2_C5_2026-11-18.md` |
| `docs/pinout_arsitektur_pengujian.md` | `docs/referensi/pinout_arsitektur_pengujian.md` |
| `docs/prosedur_B1_B2.md` | `docs/prosedur/prosedur_B1_B2.md` |
| `docs/prosedur_kalibrasi_bagian_A.md` | `docs/prosedur/prosedur_kalibrasi_bagian_A.md` |
| `docs/referensi_dokumen_E3_2026-11-18.md` | `laporan/5_status_dan_referensi/referensi_dokumen_E3_2026-11-18.md` |
| `docs/rencana_deep_sleep.md` | `docs/referensi/rencana_deep_sleep.md` |
| `docs/ringkasan_bab4_varian_stop.md` | `laporan/2_hasil_dan_pembahasan/ringkasan_bab4_varian_stop.md` |
| `laporan/antisipasi_pertanyaan_penguji_2026-11-18.md` | `laporan/4_sidang/antisipasi_pertanyaan_penguji_2026-11-18.md` |
| `laporan/kerangka_bab4_bab5_2026-11-18.md` | `laporan/2_hasil_dan_pembahasan/README.md` |
| `laporan/laporan_perubahan_skripsi_v3_2026-11-20.md` | `laporan/3_revisi_naskah/perubahan_skripsi_v3_2026-11-20.md` |
| `laporan/laporan_perubahan_skripsi_v4_kalibrasi_2026-11-21.md` | `laporan/3_revisi_naskah/perubahan_skripsi_v4_kalibrasi_2026-11-21.md` |
| `laporan/laporan_progres_revisi_paper_2026-11-15.md` | `laporan/arsip/laporan_progres_revisi_paper_2026-11-15.md` |
| `laporan/laporan_rekap_logbook_pendukung_bab4_2026-11-17.md` | `laporan/1_rekap_penelitian/rekap_logbook_10-16_nov_2026-11-17.md` |
| `laporan/laporan_rekap_logbook_pendukung_bab4_2026-11-18.md` | `laporan/1_rekap_penelitian/rekap_logbook_17-18_nov_2026-11-18.md` |
| `laporan/panduan_grafik_bab4_2026-11-16.md` | `laporan/arsip/panduan_grafik_bab4_2026-11-16.md` |
| `laporan/panduan_grafik_bab4_2026-11-18.md` | `laporan/2_hasil_dan_pembahasan/panduan_grafik_bab4_2026-11-18.md` |
| `laporan/revisi_bab1_propo_fix5_2026-11-18.md` | `laporan/3_revisi_naskah/revisi_bab1_propo_fix5_2026-11-18.md` |
| `laporan/revisi_bab2_propo_fix5_2026-11-18.md` | `laporan/3_revisi_naskah/revisi_bab2_propo_fix5_2026-11-18.md` |
| `laporan/revisi_bab3_propo_fix5_2026-11-18.md` | `laporan/3_revisi_naskah/revisi_bab3_propo_fix5_2026-11-18.md` |
| `laporan/revisi_bab3_v4_2026-11-16.md` | `laporan/arsip/revisi_bab3_v4_2026-11-16.md` |
