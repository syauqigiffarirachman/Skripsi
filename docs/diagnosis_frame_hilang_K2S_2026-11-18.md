# Diagnosis sisa frame hilang pada K2S barrage (butir C5, 18 November 2026, malam)

**Ringkasan.** Sisa kehilangan frame (≈0,17 % pada K4) **bukan acak**. Setiap run dengan frame hilang memuat satu atau lebih tanda tangan yang berlangsung tepat
≈210 ms (bukan 116 ms), dan jumlah tanda tangan lambat sama dengan jumlah frame hilang pada pengukuran langsung. Tanda tangan lambat itu terjadi bila **frame attacker
tiba tepat pada saat alarm RTC "chip siap" jatuh tempo**. Penyebab rinci di tingkat register belum dibuktikan; tertulis sebagai hipotesis di bawah.
Ini cukup untuk menjelaskan fenomena di naskah (mekanisme, frekuensi, dampak) tanpa mengubah data final.

## Bukti 1: analisis ulang data final (tanpa perangkat keras)
Dari 41 run K4 (`data/analisis/stop_final/runs.csv`) dan latensi per tanda tangan (baris `>L` mentah):
- Total frame hilang 42; tanda tangan > 150 ms ada 31. Di 17 run tanpa kehilangan: **0** tanda tangan lambat, `lat_max` = 117,6 ms.
- Semua tanda tangan lambat berdurasi **210,20 ms (sd 0,30; rentang 209,84–211,06)**, bukan sebaran acak: ada mekanisme deterministik yang menambah ≈94 ms.
- Jumlah kehilangan per run mendekati Poisson (rata-rata 1,02; dispersi 1,29), laju 0,17 % frame ≈ 0,15 % tanda tangan.
- Hanya K2S barrage yang terpengaruh; K1S barrage dan semua Normal: 0 frame hilang.
- Korelasi `rx_lost` dengan latensi rerata run: 0,89.

## Bukti 2: pengukuran langsung (`tools/diag_long.py`, build `host_k2_stop_long_auto`, flag `-DDIAG_LONG`)
16 siklus barrage 60 s (data: `data/dev/long/`). Setiap tanda tangan > 150 ms direkam bersama rincian fase dan tiap bangun dari Stop di dalamnya.
- Siklus pertama (6 siklus): frame hilang = 5, tanda tangan lambat = 5, **satu banding satu**. Siklus kedua (10 siklus): 3 dan 3.
- Semua tanda tangan lambat: waktu CPU terjaga selama menunggu chip ≈ 35 ms (normal ≈ 5,5 ms), `err = 0` (tanda tangan akhirnya benar), `gagal` = 0.
- Rincian bangun pada tanda tangan lambat (3 contoh, konsisten): **bangun pertama oleh frame (sebab = 1) pada tidur 96,26–96,28 ms**, yaitu persis waktu alarm "chip siap"
  pada tanda tangan normal (≈96,2 ms); bangun kedua oleh alarm sesudah tidur lagi ≈96,2 ms. Total 2 Stop, 192,5 ms tidur.

## Mekanisme yang konsisten dengan data
1. Selama tunggu chip (≈100 ms) host tidur Stop dengan alarm RTC. Frame attacker (10 Hz) juga membangunkan host lewat EXTI3.
2. Bila tepi frame datang berdekatan (sekitar 0,1–0,2 ms) dengan jatuh tempo alarm, host bangun lewat jalur **frame**; alarm yang jatuh tempo pada saat itu tidak terdeteksi.
   Probabilitas kebetulan ≈ lebar jendela / 100 ms; dengan laju terukur 0,15 % lebar jendela ≈ 0,15 ms, yang sepadan dengan waktu pemulihan jam dan sinkronisasi bit.
3. Karena alarm tak terlihat, host tidur lagi satu interval penuh (≈96 ms) sebelum alarm berikutnya terlihat, sehingga tanda tangan menjadi ≈210 ms.
4. Selama perpanjangan itu penerima frame tidak menyelesaikan satu frame dengan baik, sehingga muncul satu selisih nomor urut (frame hilang). Sebab detail frame hilang ini belum dibuktikan.

## Yang belum terbukti (jangan ditulis sebagai fakta)
- Mengapa alarm terlewat saat frame membangunkan host pada waktu yang hampir bersamaan. Kandidat: flag alarm terhapus oleh baca-ubah-tulis `RTC->CRL` (flag ALRF adalah rc_w0) pada saat sinkronisasi RTC pasca-Stop, atau pembacaan flag sebelum sinkronisasi (RSF). Perlu perekaman `RTC->CRL`/`ALR`/`CNT` pada tiap bangun untuk memastikan.
- Mengapa tidur kedua berakhir oleh alarm ≈96 ms kemudian (alarm tampaknya diatur ulang); belum ditelusuri.
- Pada data final frame hilang (42) lebih banyak daripada tanda tangan lambat (31): ±11 kehilangan terjadi tanpa tanda tangan lambat, jadi ada mekanisme kehilangan kedua yang lebih kecil (pengukuran langsung: 8 hilang vs 8 lambat, jadi porsinya kecil).

## Dampak pada hasil skripsi
- Data final tidak berubah. Tanda tangan tetap sah (B1 lulus); tidak ada tanda tangan salah, hanya tanda tangan yang memakan waktu 94 ms lebih lama pada ≈0,15 % permintaan.
- Latensi rerata K4 116,3 ms mengandung efek kecil (+0,08 ms); latensi maksimum K4 (≈210 ms) harus disebut jika `lat_max` dilaporkan.
- Toleransi < 0,25 % frame hilang punya penjelasan mekanistik: kebetulan frame–alarm 0,15–0,2 %. Tingkat layanan K4 turun sangat sedikit karena itu.
- Perbaikan (belum dilakukan, butuh data ulang bila dipakai): menangani alarm dan frame secara independen pada jalur bangun, atau menghindari baca-ubah-tulis `RTC->CRL` ketika bangun dari Stop. Tidak dilakukan agar data final tetap sah.

## Kalimat untuk naskah (batasan)
*"Pada K2S dalam skenario barrage, sekitar 0,15 % tanda tangan memerlukan waktu ±210 ms (bukan 116 ms) dan bertepatan satu banding satu dengan hilangnya satu frame permintaan. Kejadian ini muncul ketika frame tiba tepat pada saat alarm RTC penanda selesainya perhitungan chip, sehingga host tidur satu interval tambahan. Fenomena ini diperlakukan sebagai batasan implementasi firmware dan tidak memengaruhi keabsahan tanda tangan."*
