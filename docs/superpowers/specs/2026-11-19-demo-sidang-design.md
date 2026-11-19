# Rancangan: kode demo sidang (K1S/K2S x A/B)

Tanggal: 2026-11-19. Status: MENUNGGU PERSETUJUAN peneliti. Belum ada kode yang ditulis.

## Maksud dan alasan
Peneliti akan mendemokan langsung di depan penguji sidang. Yang ingin dilihat penguji: apakah kode yang
dibangun dan dilaporkan di paper benar-benar berperilaku seperti yang dilaporkan. Jadi demo harus
(a) memakai firmware dan jalur kendali yang SAMA dengan pengambilan data, (b) tampak langkah demi langkah
(peneliti yang mengetik, penguji yang melihat), dan (c) menutup dengan angka yang bisa dibandingkan dengan paper.

## Alur satu run (disepakati dengan peneliti)
```
logger> run A         (atau B)
[1] skrip tanya host (perintah logger 3 -> "?")        -> banner  HOST SIAP   (>ID,...,siap=1)
[2] hitung mundur ~5 d                                 -> banner  BISA MULAI MENYERANG
        peneliti mengetik a (mode A, 1/s) atau b (mode B, 10/s) di serial monitor ATTACKER (CH340)
[3] jeda tunggu (default 5 d, bisa diatur)             -> skrip kirim perintah 4 (S60)
        host membuka jendela 60 d (>START, PB1 naik); logger merekam
[4] 60 d: tampilan langsung (arus mA, detik berjalan, jumlah tepi PB1, baris host)
        akhir: >END, >L..., >DONE -> CSV disimpan, ringkasan dicetak
        "ketik x di attacker"
```
Dasar teknis (diperiksa di kode): penerima USART2 host baru dinyalakan saat jendela dibuka
(`rx_awake_start`, src/host/deepsleep/stop_common.h) dan penghitung frame di-reset di `cmd_window`
(stop_window.h), jadi attacker yang sudah berjalan sebelum jendela tidak merusak hitungan. Frame yang
datang sebelum jendela diabaikan.

## Sesi lengkap (empat run)
Urutan tetap supaya hanya satu kali flash: K1S.A -> K1S.B -> (flash host ke K2S) -> K2S.A -> K2S.B.
Perintah: `mode k1|k2` (flash host lewat ST-Link `$`, memakai `flash_host` di run_bench), `run A|B`,
`sesi` (menuntun keempat run), `ringkas` (tabel akhir).

## Tujuan
- Satu skrip PC baru, `tools/demo_sidang.py`, mengimpor `Logger`, `flash_host`, `host_id/wait_host`,
  `parse/analyse` dari `tools/run_bench.py` (tidak menyalin).
- Mengganti serial monitor logger (OUT USB logger klon rusak; perintah lewat nilai baud).
- Ringkasan akhir 4 baris (K1S/K2S x A/B): arus rata-rata, permintaan tiba/terlayani, latensi, jumlah
  frame rusak/hilang; kolom pembanding dari data skripsi (`data/analisis/stop_final/`) bila terbaca.
- Hasil mentah: `data/demo/<tanggal-jam>/` (CSV per run + ringkasan), tidak menimpa data skripsi.

## Bukan tujuan / anti-tujuan
- TIDAK mengubah firmware host, logger, attacker, `platformio.ini`, atau data skripsi.
- TIDAK menampilkan angka yang tampak bagus tetapi dihitung dengan rumus berbeda dari analisis skripsi:
  semua angka lewat fungsi analisis yang sama (rshunt/offset sama dengan analisis stop_final).
- Bukan pengganti `run_bench.py` untuk pengambilan data.

## Kendala dan risiko (jujur)
1. **Frame di tengah jalan.** Attacker sudah berjalan saat jendela dibuka; peluang sebuah frame sedang
   dikirim pada saat penerima menyala ~0,3 % (A) dan ~3 % (B). Dampaknya (frame rusak) BELUM diuji;
   kriteria data skripsi menuntut rusak = 0. Wajib dites di bangku (mis. 20 run B) sebelum demo.
   Bila terbukti bermasalah, pilihan cadangan: `--auto-attacker` (skrip memulai attacker tepat setelah
   `>START`, persis prosedur data) atau latihan ulang hingga run bersih.
2. **Prosedur beda.** Pengambilan data: attacker mulai sesudah `>START`. Demo: attacker mulai sebelum
   jendela. Sampaikan ke penguji; cek juga kalimat prosedur di Bab 3.
3. **ST-Link host terpasang selama demo** (perlu untuk flash K1<->K2). Catatan 15 Nov: itu menurunkan
   arus diam terbaca 0,03-0,11 mA. Angka demo akan sedikit berbeda dari tabel; skrip mencetak catatan
   itu di ringkasan. Setelah flash wajib reset bersih (`nodebug`, sudah di `flash_host`).
4. **Satu port satu pemakai.** Bila peneliti memegang COM attacker (serial monitor), skrip tidak boleh
   membukanya; mode `--auto-attacker` hanya dipakai bila serial monitor ditutup.
5. Kabel logger -> host PA10 pernah tercatat putus (18 Nov, uji K1S); periksa sebelum demo (`ping`).

## Pendekatan yang dipertimbangkan
- A. Skrip pengganti serial monitor logger, attacker diketik manual (DIPILIH peneliti, dengan jeda
  tunggu agar 60 d selalu penuh serangan).
- B. Sama, attacker dimulai otomatis setelah `>START` (disiapkan sebagai flag cadangan).
- C. `run_bench.py plan` otomatis penuh: aman, tapi bukan alur yang ingin ditunjukkan. Tidak dipilih.

## Keputusan saya (peneliti boleh koreksi)
- Jeda default 5 d dan hitung mundur 5 d; tulisan banner persis seperti diminta peneliti
  ("HOST SIAP", "BISA MULAI MENYERANG").
- Satu berkas skrip di `tools/`, perintah teks interaktif (tanpa GUI).
- Kolom pembanding paper bersifat tambahan: bila berkas stop_final sulit dibaca, ditunda.
- Uji skrip memakai bangku nyata; setiap aksi yang membuka COM/flash dijalankan hanya atas izin peneliti
  (aturan menjaga VS Code yang sedang berjalan).

## Diserahkan ke pelaksana
Nama berkas CSV, format tabel, penanganan Ctrl+C/galat (NOSTART/NOEND/NODONE), cara membaca stop_final,
dan tampilan langsung per detik.

## Kriteria selesai
Dry-run tanpa perangkat (`--kering`) mencetak urutan; sesi empat run di bangku berjalan dari awal sampai
ringkasan; 20 run B pra-uji tanpa frame rusak; angka ringkasan sama dengan hasil `run_bench.py analyse`
atas CSV yang sama.

## Hasil uji bangku (19 Nov 2026, logger COM18, attacker COM24)
Attacker diketik oleh peniru (pembantu di luar proyek) tepat saat banner BISA MULAI MENYERANG: attacker sudah
berjalan sebelum jendela dibuka, sama seperti alur manual.
- Biner K1S/K2S dibangun dari bersih: IDENTIK dengan `tools/biner_acuan.json`.
- Satu run K2S.A: 60/60 terlayani; angka ringkasan sama dengan `run_bench.py analyse` pada CSV yang sama.
- Sesi penuh (K1S.A, K1S.B, flash, K2S.A, K2S.B): 306 s total; flash di tengah sesi 12,5 s; flash persiapan 13,9 s.
- 20 run K2S.B berturut-turut (attacker sudah berjalan sebelum jendela): `rx_bad` = 0 di semua run (risiko
  "frame di tengah jalan" tidak terbukti). Frame HILANG 12 total: 0 hilang 9 run, 1 hilang 10 run, 2 hilang 1 run
  (run 14: 2/601 = 0,33 % > batas 0,25 %). Sebaran skripsi (41 run K2S.B): 0:17, 1:13, 2:6, 3:3, 4:2, jadi demo
  tidak lebih buruk dari data skripsi; hilang ~1 frame/run adalah temuan 18 Nov, bukan akibat alur demo.
- Keputusan: kriteria "rusak = 0" dibaca `rx_bad = 0` + hilang di bawah toleransi (aturan 18 Nov). Baris
  selesai-tapi-di-luar-toleransi diberi label PERIKSA, bukan GAGAL.
- PERINGATAN angka arus: demo (ST-Link terpasang, tanpa POR bersih) lebih tinggi dari tabel skripsi:
  K1S.A 9,99 vs 9,68 mA; K1S.B 13,73 vs 13,7; K2S.A 3,12 vs 2,55; K2S.B 7,38 vs 6,91 mA. Selisih terbesar pada
  K2S (arus tidur ~2,95 vs ~2 mA); penyebab belum dipastikan (dugaan ST-Link / POR), belum diuji dengan
  ST-Link dicabut + POR. Jumlah layanan dan latensi cocok dengan skripsi (K2S.A 116,0 vs 116,2 ms).
