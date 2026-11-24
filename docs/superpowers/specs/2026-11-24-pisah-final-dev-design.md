# Rancangan: pisahkan kode FINAL (data 18 Nov) dari kode PENGEMBANGAN

Tanggal: 2026-11-24. Cabang kerja: `rapikan-final-dev` (git worktree terpisah, `.pio/build` utama tidak disentuh).

## Tujuan
Pembaca bisa mengikuti alur satu board demi satu board, dari firmware sampai kesimpulan, tanpa tersesat di kode
percobaan. Fungsi TIDAK boleh berubah, dan harus terbukti: kode final = kode yang menghasilkan data 18 Nov.

## Bukan tujuan
- Mengubah perilaku firmware, protokol, rumus metrik, atau angka hasil.
- Mengganti nama env PlatformIO (`run_bench.py` membaca `.pio/build/host_k1_stop|host_k2_stop`).
- Menghapus kode pengembangan (hanya dipindah dan diberi label).

## Pendekatan yang dipilih (Anda memilih opsi 1)
Pindah fisik firmware + pindah fisik skrip PC, dibuktikan dengan gerbang biner di worktree.
Opsi lain (hanya `tools/` dan dokumen, firmware tetap) ditolak: tidak memenuhi permintaan "logger satu folder" dan
"final vs dev" yang nyata.

## Tata letak sasaran
```
src/
  host/
    bersama/host_util.h            dipakai final DAN dev (DWT, parse)
    final/                         = firmware data 18 Nov
      k1_stop/main.cpp             env host_k1_stop (K1S)
      k2_stop/main.cpp             env host_k2_stop (K2S)
      bersama/                     stop_common.h stop_link.h stop_window.h ds28e38_od.h
    dev/                           TIDAK dipakai di data final
      tahap_stop/                  s0a_lse s0_stop s1_rtc s2_rx (tangga menuju Stop)
      tahap_d/                     k1_software k2_hardware + bersama/host_{link,ram,rx,time}.h (tanpa Stop)
      eksperimen/                  ds_overdrive k1_rx monolit sederhana softwaretes softwaretes_mbedtls
  logger/                          SATU folder: f103/main.cpp (alur) + modul/ (INA226, tepi, laporan)
  attacker/main.cpp
lib/bench_proto/                   kontrak protokol 3 board (header-only, tetap di lib/)
include/                           mbedtls_k1_config.h (final); dev/ = konfigurasi & header uji
tools/
  1_akuisisi/  2_kalibrasi/  3_analisis/  4_verifikasi/  demo/  dev/
```
Diagnostik (`DIAG_SIG`, `DIAG_LONG`, `K1S_AUTO`, `K2S_AUTO`, `DBG_RX`) adalah `#ifdef` DI DALAM `main.cpp` final,
jadi tidak bisa dipisah ke berkas lain tanpa menggandakan kode; ia diberi label di README dan di `platformio.ini`.

## Kendala keras
1. `firmware.bin` host_k1_stop, host_k2_stop, logger, attacker harus identik dengan `tools/4_verifikasi/biner_acuan.json`
   (sha256 sama dengan manifes data 18 Nov). Env dev juga diperiksa (gerbang penuh).
2. Objek `src/` ditautkan menurut urutan path. Karena itu logger memakai `logger/f103/` lalu `logger/modul/`
   ('f' < 'm'), meniru urutan lama `logger/f103` < `logger_common`.
3. Kode bersama yang punya `.cpp` tidak boleh di `lib/` (tata letak Flash bergeser). `lib_archive` tidak diubah.
4. Nama env tetap. Komentar tidak mengubah biner (tidak ada `__FILE__`/`__LINE__`).

## Skrip PC
Modul tetap bernama sama; tiap skrip memuat `tools/_jalur.py` yang menambahkan semua subfolder `tools/` ke
`sys.path`. Dengan begitu `import run_bench` tetap berlaku di mana pun skrip dijalankan.

## Pembuktian fungsi (urutan)
1. Gerbang biner dasar (sebelum pindah) lulus di worktree.
2. Setelah pindah firmware: `cek_biner.py banding` semua env IDENTIK.
3. Setelah pindah skrip: `python -m unittest discover -s tests` lulus (53 uji); `analisis_d.py --stop` ke folder
   sementara dibandingkan dengan `data/analisis/stop_final/` (kecuali stempel waktu); gambar dibandingkan byte per byte.
4. `run_bench.py analyse` pada satu CSV mentah final = baris di `runs.csv`.

## Diserahkan ke pelaksana
Penamaan berkas README, isi `PETA_ALUR.md`, dan pembaruan path di dokumen. Logbook lama tidak ditulis ulang
(riwayat); tabel pemetaan path lama→baru ada di `docs/PEMETAAN_PATH.md`.
