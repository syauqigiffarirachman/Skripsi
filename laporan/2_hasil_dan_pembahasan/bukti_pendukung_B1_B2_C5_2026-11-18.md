# Bukti pendukung data final: B1, B2, dan diagnosis C5 (18 November 2026, malam)

Tiga pemeriksaan tambahan di atas firmware Stop final, dikumpulkan di satu tempat. Bukan data utama; ketiganya menjawab pertanyaan penguji "apakah tanda tangannya sah" (B1), "apakah klaim hemat bergantung pada konfigurasi software" (B2), dan "mengapa masih ada frame hilang pada K4" (C5). Prosedur bangku B1/B2: `docs/prosedur/prosedur_B1_B2.md`.

| Bagian | Pertanyaan | Kesimpulan singkat |
|---|---|---|
| 1. B1 | Sahkah tanda tangan dari jalur layanan sebenarnya? | LULUS untuk K1S dan K2S |
| 2. B2 | Berubahkah klaim bila tabel comb Mbed TLS aktif? | Arah sama, keunggulan latensi turun 82 % menjadi 58 % (data pengembangan, N kecil; pakai sebagai batasan) |
| 3. C5 | Mengapa frame hilang ~0,17 % pada K4? | Bukan acak: tanda tangan lambat ~210 ms saat frame tiba bersamaan alarm RTC |

## 1. Hasil B1: verifikasi tanda tangan pada jalur layanan firmware Stop (18 November 2026, malam)

**Kesimpulan: LULUS untuk K1S dan K2S.** Tanda tangan yang keluar dari jalur layanan sebenarnya (bangun dari Stop oleh frame attacker, penerima frame,
pembuatan tanda tangan) sah menurut verifikasi ECDSA P-256 murni Python di komputer (`tools/4_verifikasi/p256.py`, independen dari Mbed TLS dan dari DS28E38).
Ini menutup butir 14 Bagian 9 rekap 18 Nov dan butir B1 di daftar kekurangan.

### Cara
- Build diagnostik `host_k1_stop_diag_auto` / `host_k2_stop_diag_auto` (flag `-DDIAG_SIG`): menyalin r‖s 30 tanda tangan pertama dan mengirimnya sebagai baris `>SIG` sebelum `>DONE`.
  Kode layanan tidak diubah; build final (`host_k1_stop`, `host_k2_stop`) dibangun ulang dari bersih dan identik bit demi bit dengan biner data final
  (sha256 `2ca6e8d0d861…` dan `5b4e017827e3…`, lihat `data/manifes_firmware_stop.md`).
- Attacker mode A (1 permintaan/detik, 60 s) dinyalakan saat `>START`; verifikasi 30 tanda tangan pertama dengan `tools/4_verifikasi/b1_verifikasi.py`.
- Rekaman mentah: `data/dev/b1/20261118_223404_K1S_sig.csv`, `data/dev/b1/20261118_223643_K2S_sig.csv`.

### Hasil
| Arsitektur | Diterima | Sah di komputer | Dirusak (s xor 1) ditolak | Pemeriksaan lain |
|---|---|---|---|---|
| K1S (Mbed TLS, comb mati) | 30 | **30/30** | 30/30 | kunci publik = d × G; 30 nilai r berbeda semua (nonce hedged) |
| K2S (DS28E38 overdrive) | 30 | **30/30** | 30/30 | ROM dari firmware = ROM chip pada rekaman pengembangan (`4d2b423200000094`); kunci publik chip pada kurva; pesan = SHA-256(ROM‖page0‖digest‖0x00‖MANID) |

### Catatan jujur
- Yang diverifikasi ialah build diagnostik, bukan biner data final itu sendiri (biner berbeda karena tambahan penyalinan hasil). Kode jalur layanan identik; pembuktian bahwa biner final menghasilkan tanda tangan sah bersifat tak langsung.
- Pada K2S byte keluaran `ds_sign` berurutan **r‖s**; pada rekaman pengembangan lama (`tools/dev/tahap_stop/ds_od.py`) pengurai memakai s‖r untuk tampilan eksperimen itu. Skrip B1 menguji kedua urutan secara otomatis dan melaporkan yang berlaku; tidak memengaruhi keabsahan.
- Host dalam keadaan setelah flash lewat ST-Link (domain debug aktif); fungsi tanda tangan tidak terpengaruh, tetapi sesi ini bukan sesi ukur arus.
- Cakupan: 30 tanda tangan per arsitektur pada satu papan dan satu chip; bukan uji kriptografis menyeluruh (tidak menguji distribusi nonce selain keunikan r pada K1S).

### Kalimat untuk naskah
*"Keabsahan kriptografis tanda tangan pada jalur layanan firmware varian Stop diverifikasi di komputer dengan implementasi verifikasi ECDSA P-256 independen: 30 dari 30 tanda tangan K1S dan 30 dari 30 tanda tangan K2S sah, dan seluruh tanda tangan yang sengaja dirusak ditolak."*

## 2. Hasil B2: sensitivitas konfigurasi software, tabel comb Mbed TLS aktif (18 November 2026, malam)

Build `host_k1_stop_comb_auto` (kode K1S identik; hanya `MBEDTLS_ECP_FIXED_POINT_OPTIM = 1`, `include/dev/mbedtls_k1_comb_config.h`; sha256 biner `968cf7681a64…`).
Diukur di bangku dengan `tools/dev/tahap_stop/k1s_run.py` (1 IDLE, 3 run skenario A, 2 run skenario B; semua run lolos pemeriksaan standar; `gagal` = 0 pada semua run,
jadi tabel comb muat di heap). Berkas mentah: `data/dev/k1s/*_comb.csv`. **Data pengembangan, bukan data final**: host dalam keadaan setelah flash ST-Link
(lantai IDLE 2,557 mA, bukan 1,99 mA), N kecil, shunt nominal 0,100 Ω. Energi dihitung relatif terhadap IDLE sesi yang sama sehingga lantai tak banyak memengaruhi.

### Hasil
| Metrik | K1 baseline (comb mati, data final, N=30) | K1S comb aktif (N=3 A, 2 B) | K2 DS28E38 (data final) |
|---|---|---|---|
| Latensi sign (ms) | 654,88 | **278,56** (sd 0,02) | 116,23 |
| Energi dinamis per layanan (mJ), Normal | 41,03 | **16,9** | 3,01 |
| Tidur CPU Normal (%) | 35,08 | 71,70 | 98,17 |
| Tingkat layanan Barrage (%) | 15,04 | **34,94** | 85,50 (K4) |
| Energi per layanan Barrage (mJ) | 40,59 | 16,7 | 3,00 |

Latensi 278,6 ms cocok dengan pengukuran pengembangan 13 Nov (278,6 ms), sehingga kedua pengukuran saling menguatkan.

### Keunggulan DS28E38 terhadap software dengan comb aktif
| Metrik | vs comb mati | vs comb aktif |
|---|---|---|
| Latensi | 82,2 % lebih cepat (5,6×) | **58,3 %** lebih cepat (2,4×) |
| Energi per tanda tangan | 92,7 % lebih hemat (13,6×) | **82,2 %** lebih hemat (5,6×) |
| Tingkat layanan Barrage | 85,5 % vs 15,0 % | 85,5 % vs 34,9 % |

**Tafsiran:** arah dan kesimpulan tidak berubah, tetapi besar keunggulan turun. Latensi 58 % (bukan 82 %) dan energi 82 % (bukan 93 %) ialah batas bawah yang adil
bila penguji menyoal konfigurasi software. Keunggulan atas konfigurasi software terbaik tetap bermakna; angka 82 % harus selalu ditulis bersama label "konfigurasi 1c, tabel comb dimatikan".

### Batasan
- Satu papan, 5 run (dev); belum 30 run, tanpa urutan ABBA, tanpa POR bersih; nilai energi untuk perbandingan kasar (±beberapa persen), bukan untuk tabel utama.
- Tabel comb dihitung sekali lalu di-cache (diduga pada tanda tangan pertama sejak boot, belum diverifikasi di kode). Run terukur ialah siklus host lanjutan (k1s_run mendahuluinya dengan satu rekaman sinkron), tidak ada tanda tangan lambat di dalamnya (lat_min 277,3 ms, lat_max 279,9 ms).
- Menyertakan comb aktif dalam data skripsi final akan membutuhkan 4 × 30 run ulang; disarankan menyatakannya sebagai batasan dan saran.

### Kalimat untuk naskah
*"Pada konfigurasi Mbed TLS dengan tabel comb dimatikan (konfigurasi penelitian), DS28E38 lebih cepat 82,2 %. Uji sensitivitas dengan tabel comb aktif (latensi software 278,6 ms; N kecil, data pengembangan) menurunkan keunggulan menjadi 58,3 % pada latensi dan 82,2 % pada energi per tanda tangan; arah kesimpulan tetap sama."*

## 3. Diagnosis sisa frame hilang pada K2S barrage (butir C5, 18 November 2026, malam)

**Ringkasan.** Sisa kehilangan frame (≈0,17 % pada K4) **bukan acak**. Setiap run dengan frame hilang memuat satu atau lebih tanda tangan yang berlangsung tepat
≈210 ms (bukan 116 ms), dan jumlah tanda tangan lambat sama dengan jumlah frame hilang pada pengukuran langsung. Tanda tangan lambat itu terjadi bila **frame attacker
tiba tepat pada saat alarm RTC "chip siap" jatuh tempo**. Penyebab rinci di tingkat register belum dibuktikan; tertulis sebagai hipotesis di bawah.
Ini cukup untuk menjelaskan fenomena di naskah (mekanisme, frekuensi, dampak) tanpa mengubah data final.

### Bukti 1: analisis ulang data final (tanpa perangkat keras)
Dari 41 run K4 (`data/analisis/stop_final/runs.csv`) dan latensi per tanda tangan (baris `>L` mentah):
- Total frame hilang 42; tanda tangan > 150 ms ada 31. Di 17 run tanpa kehilangan: **0** tanda tangan lambat, `lat_max` = 117,6 ms.
- Semua tanda tangan lambat berdurasi **210,20 ms (sd 0,30; rentang 209,84–211,06)**, bukan sebaran acak: ada mekanisme deterministik yang menambah ≈94 ms.
- Jumlah kehilangan per run mendekati Poisson (rata-rata 1,02; dispersi 1,29), laju 0,17 % frame ≈ 0,15 % tanda tangan.
- Hanya K2S barrage yang terpengaruh; K1S barrage dan semua Normal: 0 frame hilang.
- Korelasi `rx_lost` dengan latensi rerata run: 0,89.

### Bukti 2: pengukuran langsung (`tools/4_verifikasi/diag_long.py`, build `host_k2_stop_long_auto`, flag `-DDIAG_LONG`)
16 siklus barrage 60 s (data: `data/dev/long/`). Setiap tanda tangan > 150 ms direkam bersama rincian fase dan tiap bangun dari Stop di dalamnya.
- Siklus pertama (6 siklus): frame hilang = 5, tanda tangan lambat = 5, **satu banding satu**. Siklus kedua (10 siklus): 3 dan 3.
- Semua tanda tangan lambat: waktu CPU terjaga selama menunggu chip ≈ 35 ms (normal ≈ 5,5 ms), `err = 0` (tanda tangan akhirnya benar), `gagal` = 0.
- Rincian bangun pada tanda tangan lambat (3 contoh, konsisten): **bangun pertama oleh frame (sebab = 1) pada tidur 96,26–96,28 ms**, yaitu persis waktu alarm "chip siap"
  pada tanda tangan normal (≈96,2 ms); bangun kedua oleh alarm sesudah tidur lagi ≈96,2 ms. Total 2 Stop, 192,5 ms tidur.

### Mekanisme yang konsisten dengan data
1. Selama tunggu chip (≈100 ms) host tidur Stop dengan alarm RTC. Frame attacker (10 Hz) juga membangunkan host lewat EXTI3.
2. Bila tepi frame datang berdekatan (sekitar 0,1–0,2 ms) dengan jatuh tempo alarm, host bangun lewat jalur **frame**; alarm yang jatuh tempo pada saat itu tidak terdeteksi.
   Probabilitas kebetulan ≈ lebar jendela / 100 ms; dengan laju terukur 0,15 % lebar jendela ≈ 0,15 ms, yang sepadan dengan waktu pemulihan jam dan sinkronisasi bit.
3. Karena alarm tak terlihat, host tidur lagi satu interval penuh (≈96 ms) sebelum alarm berikutnya terlihat, sehingga tanda tangan menjadi ≈210 ms.
4. Selama perpanjangan itu penerima frame tidak menyelesaikan satu frame dengan baik, sehingga muncul satu selisih nomor urut (frame hilang). Sebab detail frame hilang ini belum dibuktikan.

### Yang belum terbukti (jangan ditulis sebagai fakta)
- Mengapa alarm terlewat saat frame membangunkan host pada waktu yang hampir bersamaan. Kandidat: flag alarm terhapus oleh baca-ubah-tulis `RTC->CRL` (flag ALRF adalah rc_w0) pada saat sinkronisasi RTC pasca-Stop, atau pembacaan flag sebelum sinkronisasi (RSF). Perlu perekaman `RTC->CRL`/`ALR`/`CNT` pada tiap bangun untuk memastikan.
- Mengapa tidur kedua berakhir oleh alarm ≈96 ms kemudian (alarm tampaknya diatur ulang); belum ditelusuri.
- Pada data final frame hilang (42) lebih banyak daripada tanda tangan lambat (31): ±11 kehilangan terjadi tanpa tanda tangan lambat, jadi ada mekanisme kehilangan kedua yang lebih kecil (pengukuran langsung: 8 hilang vs 8 lambat, jadi porsinya kecil).

### Dampak pada hasil skripsi
- Data final tidak berubah. Tanda tangan tetap sah (B1 lulus); tidak ada tanda tangan salah, hanya tanda tangan yang memakan waktu 94 ms lebih lama pada ≈0,15 % permintaan.
- Latensi rerata K4 116,3 ms mengandung efek kecil (+0,08 ms); latensi maksimum K4 (≈210 ms) harus disebut jika `lat_max` dilaporkan.
- Toleransi < 0,25 % frame hilang punya penjelasan mekanistik: kebetulan frame–alarm 0,15–0,2 %. Tingkat layanan K4 turun sangat sedikit karena itu.
- Perbaikan (belum dilakukan, butuh data ulang bila dipakai): menangani alarm dan frame secara independen pada jalur bangun, atau menghindari baca-ubah-tulis `RTC->CRL` ketika bangun dari Stop. Tidak dilakukan agar data final tetap sah.

### Kalimat untuk naskah (batasan)
*"Pada K2S dalam skenario barrage, sekitar 0,15 % tanda tangan memerlukan waktu ±210 ms (bukan 116 ms) dan bertepatan satu banding satu dengan hilangnya satu frame permintaan. Kejadian ini muncul ketika frame tiba tepat pada saat alarm RTC penanda selesainya perhitungan chip, sehingga host tidur satu interval tambahan. Fenomena ini diperlakukan sebagai batasan implementasi firmware dan tidak memengaruhi keabsahan tanda tangan."*
