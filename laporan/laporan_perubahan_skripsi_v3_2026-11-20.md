# Laporan perubahan SKRIPSI_FULL_Syauqi_v3 (20 November 2026)

Acuan: `paper/SKRIPSI_FULL_Syauqi_v3_2026-11-18.pdf` (93 halaman), proposal bertanda tangan `paper/[ttd]Proposal Skripsi_...pdf`, dan seluruh laporan di `laporan/` dan `docs/`.
Cara pakai: tiap butir memuat **lokasi** (sub-bab dan nomor halaman cetak), **teks sekarang**, **teks pengganti**, dan **sumber**. Tempel teks pengganti ke Word, lalu **Update Field** pada daftar isi, daftar tabel, dan daftar gambar.

Keputusan Anda yang dipakai di laporan ini:
1. Data skripsi = data *deep sleep* (Stop). Shapiro-Wilk dan ANOVA **tidak** dikerjakan (proposal bertanda tangan tidak menyebutnya).
2. Hasil B2 (Mbed TLS dengan tabel comb aktif) **tidak dipakai**; baseline tetap Mbed TLS ±655 ms dan kesimpulan tidak berubah.
3. **Varian konfigurasi Mbed TLS dihapus dari naskah** (Kelompok H di bawah), **termasuk penyebutan "tabel comb dimatikan"** (Opsi B dipilih, 20 November).
4. Asumsi baterai ilustratif (1000 mAh, 3,7 V) **disetujui** untuk subbab 5.11.9.
5. Jawaban sidang untuk pertanyaan B2 ditambahkan sebagai butir 19 di `laporan/antisipasi_pertanyaan_penguji_2026-11-18.md`; butir 3 di berkas itu tidak diubah.

Catatan pembacaan: saya membaca teks PDF, bukan tampilan halamannya. Format tabel dan isi gambar tidak bisa saya periksa. Nomor halaman mengikuti nomor cetak di PDF.

---

## 0. Ringkasan perubahan

| Kode | Jenis | Isi | Lokasi utama |
|---|---|---|---|
| H1-H13 | Hapus/ubah | Varian konfigurasi Mbed TLS (Tabel 4.3, perbandingan comb aktif, klaim ketergantungan konfigurasi) | 1.3, 3.2.2, 3.3.1, 3.3.3, 4.1, 4.3.4, 5.1, 5.11.5, 5.11.6, 5.12, 6.1, 6.2, Lampiran A dan B |
| T1 | Tambah | Verifikasi tanda tangan pada jalur layanan final (B1) | 3.4.4, 4.7.1 (Tabel 4.4), 4.7.4, 5.12 |
| T2 | Tambah | Sensitivitas batas frame hilang (B3) | 3.4.6, subbab baru 5.2.6 |
| T3 | Ubah | Ketertelusuran firmware: sidik `firmware.bin` (B4) | 4.7.4 |
| T4 | Ubah | Mekanisme sisa frame hilang (C5) | 4.3.3, 5.2.1, 5.4, 5.12 |
| T5 | Tambah | Dekomposisi energi per tanda tangan (C1) | subbab baru 5.6.4, 5.11.1 |
| T6 | Ubah | Ancaman terhadap validitas (C6) | 5.12 |
| T7 | Tambah | Implikasi umur baterai (C7) dan batas waktu respons (C4) | subbab baru 5.11.9, 5.12 |
| T8 | Tambah | Penyimpangan terhadap proposal bertanda tangan | Tabel 5.3 diperluas |
| T9 | Ubah | Pelaporan uji hipotesis: t, derajat bebas, p < 10⁻¹⁰⁰ (B5) | Tabel 5.12 |
| K1-K6 | Koreksi | Nama berkas, angka mutlak di Intisari, Flash/RAM, sitasi, Lampiran D, istilah | lihat Kelompok K |
| M1-M8 | Menunggu | Kalibrasi, pull-up, offset, dan item yang butuh tangan atau keputusan Anda | lihat Kelompok M |

Yang **tidak** berubah: seluruh angka hasil utama Bab 5 (latensi 654,88 dan 116,23 ms, daya dinamis, durasi tidur, tingkat layanan, interaksi +20,51 poin), status H1-H4 dan hipotesis interaksi, serta seluruh kesimpulan Bab 6.

---

## Kelompok H. Penghapusan varian konfigurasi Mbed TLS

Prinsip (Opsi B, keputusan Anda): naskah **tidak lagi menyebut konfigurasi Mbed TLS apa pun** selain "konfigurasi tetap pada seluruh run": tidak ada varian, perbandingan, klaim ketergantungan konfigurasi, maupun penyebutan tabel comb. Konfigurasi yang dipakai tetap terdokumentasi di kode dan `include/` pada repositori, sehingga dapat direproduksi dari sana.

### H1. Sub-bab 1.3 butir 2 (hal. 4)
Sekarang: "...Pembanding perangkat lunak dibatasi pada pustaka Mbed TLS dengan satu konfigurasi, yaitu tabel percepatan comb dimatikan, sehingga besar selisih terhadap hardware offloading bergantung pada konfigurasi ini. Keabsahan kriptografis tanda tangan diverifikasi di komputer pada tahap pengembangan, tidak pada setiap run pengukuran."

Pengganti:
> ...Pembanding perangkat lunak dibatasi pada pustaka Mbed TLS 3.6.7 dengan satu konfigurasi tetap (Sub-bab 3.3.3). Keabsahan kriptografis tanda tangan diverifikasi di komputer pada tahap pengembangan dan pada jalur layanan firmware final dengan build diagnostik (Sub-bab 3.4.4), tidak pada setiap run pengukuran.

### H2. Sub-bab 3.2.2 butir 3, variabel kontrol (hal. 25)
Sekarang: "...konfigurasi Mbed TLS yang sama (tabel comb dimatikan), kebijakan tanpa antrean..."
Pengganti: "...konfigurasi Mbed TLS yang sama pada seluruh run, kebijakan tanpa antrean..."

### H3. Tabel 3.4 baris 2, perangkat lunak (hal. 28)
Sekarang: "ECDSA P-256 perangkat lunak dengan tabel percepatan comb dimatikan dan nonce hedged (HMAC-DRBG); konfigurasi ini memengaruhi latensi dan dinyatakan sebagai batasan (Sub-bab 1.3)"
Pengganti: "ECDSA P-256 perangkat lunak dengan konfigurasi tetap dan nonce hedged (HMAC-DRBG)"

### H4. Sub-bab 3.3.3 butir c (hal. 32)
Sekarang: "...pengumpulan sumber acak di luar jendela pengukuran latensi, serta tabel comb dimatikan dengan optimasi rakitan dan optimasi khusus kurva P-256 tetap aktif. Konfigurasi ini dinyatakan secara eksplisit karena besar keunggulan hardware offloading bergantung pada konfigurasi pembanding (Noseda dkk., 2022)."

Pengganti (Opsi B):
> ...pengumpulan sumber acak di luar jendela pengukuran latensi, serta konfigurasi pustaka yang sama pada seluruh run.

Catatan: konfigurasi pustaka (nilai `MBEDTLS_ECP_FIXED_POINT_OPTIM` dan sejenisnya) tidak tertulis di naskah tetapi terlihat di repositori publik. Pertanyaan penguji tentang hal ini disiapkan jawabannya pada butir 19 `antisipasi_pertanyaan_penguji`.

### H5. Tabel 4.1 baris "Pustaka kriptografi" (hal. 43)
Sekarang: keterangan "Diadopsi dengan konfigurasi khusus (Sub-bab 4.3.4)"
Pengganti: "Konfigurasi tetap (Sub-bab 3.3.3)"

### H6. Sub-bab 4.3.4 (hal. 50): hapus paragraf varian dan Tabel 4.3
Hapus seluruh paragraf yang dimulai "Selama pengembangan, beberapa varian konfigurasi Mbed TLS diukur untuk memahami pengaruhnya terhadap latensi (Tabel 4.3)." dan **Tabel 4.3** beserta catatannya (278,6; 654,7; 687,4; 4.331,9 ms).
Pertahankan pelajaran kompilasi, dengan rujukan tabel yang baru:
> Pustaka tidak dibangun ulang otomatis setelah berkas konfigurasi diubah, sehingga hasil pertama sempat keliru (Tabel 4.7 butir 5); sejak itu ditetapkan aturan membersihkan hasil kompilasi sebelum membangun ulang.

Akibat: nomor tabel di Bab 4 bergeser satu. Bila memakai *cross-reference* Word, cukup Update Field. Bila tidak, ubah manual:

| Lama | Baru | Dirujuk di |
|---|---|---|
| Tabel 4.4 | Tabel 4.3 | 4.3.5, 5.4, 5.11.5 |
| Tabel 4.5 | Tabel 4.4 | 4.7.1 |
| Tabel 4.6 | Tabel 4.5 | 4.7.2 |
| Tabel 4.7 | Tabel 4.6 | 4.7.3 |
| Tabel 4.8 | Tabel 4.7 | 4.3.2 ("Tabel 4.8 butir 9"), 4.8 |

### H7. Tabel 5.1, baris "Arsitektur" (hal. 58)
Sekarang: "Software-based (Mbed TLS 3.6.7, comb table dinonaktifkan) dan hardware offloading (DS28E38, ...)"
Pengganti: "Software-based (Mbed TLS 3.6.7, konfigurasi tetap) dan hardware offloading (DS28E38, ...)"

### H8. Sub-bab 5.11.5 (hal. 78): tulis ulang
Judul baru: **5.11.5 Sensitivitas terhadap Cara Akses DS28E38**
> Besar keunggulan hardware offloading bergantung pada cara akses DS28E38. Cara akses kecepatan standar dengan penantian tetap 130 ms menghasilkan sekitar 192,5 ms per tanda tangan (Tabel 4.3), sehingga latensi 116 ms pada penelitian ini adalah hasil konfigurasi overdrive dan polling. Kontribusi overdrive, polling, dan mode Stop terhadap hasil akhir tidak dipisahkan dalam data penelitian. Hal ini sejalan dengan Noseda dkk. (2022) bahwa manfaat offloading tidak dapat digeneralisasi begitu saja, karena bergantung pada perangkat dan implementasinya.

Tambahan opsional (dari eksperimen pengembangan Tabel 4.3, tanpa Stop; sumber `docs/bahan_pembahasan_bab4_2026-11-18.md` bagian 4):
> Pada eksperimen pengembangan tanpa mode Stop, penurunan dari 192,5 ms menjadi 115,6 ms (76,9 ms) berasal sekitar 70 % dari kecepatan overdrive dan sekitar 30 % dari polling; kedua efek hampir aditif.

### H9. Tabel 5.14, baris Noseda dkk., kolom "Keterangan" (hal. 79)
Sekarang: "...diduga karena CPU tidur dalam selama IC bekerja, akses overdrive, dan konfigurasi pembanding (indikatif, platform dan definisi energi berbeda)"
Pengganti: "...diduga karena CPU tidur dalam selama IC bekerja dan akses overdrive (indikatif, platform dan definisi energi berbeda)"

### H10. Sub-bab 5.12 butir 4 (hal. 81): hapus
Hapus butir "Besar keunggulan hardware offloading bergantung pada konfigurasi Mbed TLS; dengan comb table aktif keunggulan latensi jauh lebih kecil." Butir sesudahnya bergeser; lihat daftar lengkap pengganti di T6.

### H11. Sub-bab 6.1, paragraf "Penelitian ini masih memiliki kekurangan" (hal. 83)
Sekarang: "...ketergantungan hasil pada konfigurasi Mbed TLS serta cara akses DS28E38 (overdrive dan polling), arus diam papan..."
Pengganti: "...ketergantungan hasil pada cara akses DS28E38 (overdrive dan polling), arus diam papan..."

### H12. Sub-bab 6.2 saran 4 (hal. 84)
Sekarang: "Membandingkan beberapa konfigurasi dan pustaka perangkat lunak, misalnya Mbed TLS dengan comb table aktif dan pustaka ringan seperti micro-ecc, dengan protokol pengukuran yang sama."
Pengganti: "Membandingkan pustaka perangkat lunak lain, misalnya pustaka ringan seperti micro-ecc, dengan protokol pengukuran yang sama."

### H13. Lampiran A dan B (hal. 89 dan 91)
- Lampiran A: **hapus baris istilah "Comb table"** beserta artinya ("Tabel praperhitungan pada Mbed TLS untuk mempercepat perkalian titik kurva eliptik"). Periksa bahwa istilah lain di tabel tidak bergeser.
- Lampiran B, Tabel B.1, 14 November: "Percepatan akses DS28E38, pengukuran Mbed TLS tanpa optimasi, program jendela 60 detik" menjadi "Percepatan akses DS28E38, pengukuran latensi ECDSA perangkat lunak, program jendela 60 detik".

### Catatan konsistensi di luar naskah
Data pengembangan (`data/dev/k1s/*_comb.csv`) dan dokumen `docs/hasil_B2_...md` tetap ada di repositori publik. Itu tidak bertentangan dengan naskah selama naskah tidak mengklaim apa pun tentang konfigurasi lain. Jawaban sidang untuk "apakah Mbed TLS bisa lebih cepat" perlu disesuaikan: lihat butir M8.

---

## Kelompok T. Penambahan dan penyesuaian isi

### T1. Verifikasi tanda tangan pada jalur layanan final (B1)
**Sumber:** `docs/hasil_B1_verifikasi_tanda_tangan_2026-11-18.md`, `tools/b1_verifikasi.py`, `tools/p256.py`, rekaman `data/dev/b1/`.

**(a) Sub-bab 3.4.4, paragraf kedua (hal. 38).** Ganti paragraf "Keabsahan kriptografis tanda tangan tidak diperiksa pada setiap run pengukuran..." dengan:
> Keabsahan kriptografis tanda tangan tidak diperiksa pada setiap run pengukuran, karena firmware hanya melaporkan keberhasilan penandatanganan agar tidak membebani jendela ukur. Keabsahannya diverifikasi terpisah di komputer dengan implementasi verifikasi ECDSA P-256 yang independen dari Mbed TLS dan dari DS28E38, pada dua tahap. Pada tahap pengembangan, tanda tangan perangkat lunak diverifikasi 30 dari 30 dan tanda tangan DS28E38 pada kecepatan overdrive 269 dari 269, termasuk pemeriksaan bahwa tanda tangan yang sengaja dirusak ditolak. Pada jalur layanan firmware varian Stop, build diagnostik (bendera kompilasi DIAG_SIG; lingkungan host_k1_stop_diag_auto dan host_k2_stop_diag_auto) mengirim nilai r‖s dari 30 tanda tangan pertama ke komputer sebelum pesan >DONE, tanpa mengubah kode layanan. Hasilnya 30 dari 30 tanda tangan K1S dan 30 dari 30 tanda tangan K2S sah, dan seluruh tanda tangan yang sengaja dirusak ditolak. Pada K1S kunci publik sama dengan d × G dan ke-30 nilai r berbeda (nonce hedged); pada K2S kunci publik IC terletak pada kurva P-256. Yang diverifikasi adalah build diagnostik, bukan biner data akhir itu sendiri, sehingga kesahan tanda tangan pada biner data akhir bersifat tak langsung. Blok kripto perlakuan software-based pada firmware mode tidur dalam adalah salinan harfiah blok yang telah diverifikasi pada tahap pengembangan.

**(b) Sub-bab 4.7.1 dan Tabel 4.4 (lama 4.5), hal. 53-54.**
- Kalimat pembuka: "Bagian sistem yang tidak berubah antara versi Sleep dan varian Stop diuji secara fungsional pada tahap pengembangan awal." menjadi "...diuji secara fungsional pada tahap pengembangan awal, dan jalur tanda tangan firmware varian Stop diuji pada butir 7."
- Tambahkan baris 7:

| No. | Bagian yang diuji | Kriteria | Hasil |
|---|---|---|---|
| 7 | Tanda tangan pada jalur layanan firmware varian Stop (build diagnostik) | Tanda tangan sah menurut verifikasi independen di PC; tanda tangan yang dirusak ditolak | K1S 30/30 dan K2S 30/30 sah; dirusak ditolak 30/30; lulus |

**(c) Sub-bab 5.12 butir 12 (lihat daftar lengkap di T6).**

**(d) Lampiran D (Tabel D.1):** tambahkan `tools/b1_verifikasi.py` dan `tools/p256.py` (verifikasi tanda tangan independen) ke baris skrip uji.

### T2. Sensitivitas batas frame hilang (B3)
**Sumber:** `tools/analisis_sensitivitas.py`, `data/analisis/stop_final/sensitivitas/`, `docs/bahan_pembahasan_bab4_2026-11-18.md` bagian 1.

**(a) Sub-bab 3.4.6 (hal. 40):** pada kalimat "...ditetapkan peneliti sebagai toleransi kurang dari 0,25 % setelah penyebab kehilangan frame diselidiki." tambahkan: "Pengaruh pilihan ambang ini terhadap kesimpulan diperiksa pada Sub-bab 5.2.6."

**(b) Subbab baru 5.2.6, sesudah 5.2.5 (sebelum 5.3):**

> **5.2.6 Sensitivitas terhadap Batas Frame Hilang**
> Batas frame hilang kurang dari 0,25 % per run pada skenario barrage perlakuan hardware offloading ditetapkan setelah penyebab kehilangan diselidiki (Sub-bab 3.4.6). Agar kesimpulan tidak bergantung pada ambang tersebut, K4 dianalisis ulang pada empat ambang dan pada seluruh 41 run K4, termasuk 11 run gugur (Tabel 5.x).
>
> | Ambang frame hilang | Run K4 lolos | Latensi (ms) | Daya dinamis (mW) | Durasi tidur (%) | Energi per layanan (mJ) | Tingkat layanan (%) |
> |---|---|---|---|---|---|---|
> | 0 (tanpa kehilangan) | 17 | 116,24 | 25,70 | 83,96 | 3,0000 | 85,46 |
> | 0,25 % (dipakai) | 30 | 116,30 | 25,69 | 83,97 | 3,0000 | 85,50 |
> | 0,35 % | 36 | 116,34 | 25,69 | 83,97 | 3,0000 | 85,51 |
> | 1,0 % (seluruh run) | 41 | 116,38 | 25,68 | 83,97 | 3,0002 | 85,53 |
>
> Rerata antar-ambang bergeser 0,14 ms pada latensi (0,12 %), 0,025 mW pada daya dinamis (0,10 %), dan 0,07 poin pada tingkat layanan. Perbandingan K3 terhadap K4 memberi penghematan latensi 82,24 % (30 run) dan 82,23 % (41 run), daya dinamis 58,00 % dan 58,02 %, serta energi per layanan 92,61 % pada keduanya; seluruh keputusan uji tetap sama (p < 10⁻¹⁰⁰, kecuali energi per layanan dan tingkat layanan yang tetap di bawah 10⁻⁸⁸). Pemilihan ambang karenanya tidak memengaruhi kesimpulan. Seluruh 11 run gugur gugur hanya karena frame UART hilang (2 sampai 4 dari 598-600), bukan karena galat tanda tangan atau pin status.

Label "Tabel 5.x" diberi nomor otomatis Word. Daftar tabel perlu di-update.

### T3. Ketertelusuran firmware (B4), sub-bab 4.7.4 (hal. 56)
**Sumber:** `data/manifes_firmware_stop.md`, `docs/hasil_B1_...md`.
Masalah: draft memakai sidik **ELF** (`3e9c7595…`, `5d32d741…`) tanpa label, sedangkan manifes dan B1 memakai sidik **`firmware.bin`**.

Ganti seluruh sub-bab:
> Kode yang menghasilkan data akhir dibekukan pada commit 313aada, dan data akhir disimpan pada commit 518f54c dan 43e701a. Firmware yang dipakai adalah lingkungan host_k1_stop dan host_k2_stop. Sidik SHA-256 berkas firmware.bin hasil pembangunan bersih ialah 2ca6e8d0d86115d0… untuk K1S (47.684 byte) dan 5b4e017827e39e17… untuk K2S (31.308 byte); firmware logger (7ac6b702300d58ab…) dan attacker node (faf092a1fff6f2de…) identik dengan acuan 16 November. Sidik firmware tidak dicatat pada saat sesi pengambilan data, sehingga kesesuaiannya dengan biner yang tertanam di papan dibuktikan secara tidak langsung: tidak ada berkas sumber yang lebih baru daripada biner saat sesi dimulai, pembangunan ulang dari bersih menghasilkan sidik yang sama, dan identitas pembangunan logger (4C364231) tercatat pada baris #RUN,BEGIN seluruh run. Pemakaian Flash varian Stop sekitar 72,8 % (K1S) dan 47,8 % (K2S) dari 64 KB; pemakaian RAM belum diukur.

Sidik lengkap untuk lampiran atau catatan kaki:

| Peran | Lingkungan | sha256 firmware.bin | Ukuran (byte) |
|---|---|---|---|
| K1S | host_k1_stop | 2ca6e8d0d86115d09cf9db24b72e511a24f4463b329c0ceaeb76ed485b814a5c | 47.684 |
| K2S | host_k2_stop | 5b4e017827e39e17a213bf972b8f3346c15142ec0e02576bb8847680e6ae2f71 | 31.308 |
| logger | logger | 7ac6b702300d58abb06fcaa2451e7e53801d40424203ee77df1a34cdf3a27e68 | 46.812 |
| attacker | attacker | faf092a1fff6f2def7e93b690bfd9eca41a92137d3d33ed6ee66fe77d53cbde9 | 15.568 |

Sesuaikan juga 5.12 butir 13: "Pemakaian RAM firmware varian Stop belum diukur, dan PCB logger serta attacker node telah dirancang tetapi belum difabrikasi."

### T4. Mekanisme sisa frame hilang (C5)
**Sumber:** `docs/diagnosis_frame_hilang_K2S_2026-11-18.md`, `tools/diag_long.py`.

**(a) Sub-bab 4.3.3, dua kalimat terakhir paragraf "Dua perbaikan" (hal. 49).** Sekarang: "...Sisa kehilangan sekitar 0,17 % tidak dapat dihilangkan tanpa mengubah desain penerima, sehingga ditetapkan toleransi kurang dari 0,25 % per run (Sub-bab 3.4.5)."
Pengganti:
> Sisa kehilangan sekitar 0,17 % diselidiki lebih lanjut dan ternyata tidak acak: tiap run dengan frame hilang memuat tanda tangan yang berdurasi tepat sekitar 210 ms (simpangan baku 0,3 ms), bukan 116 ms (Sub-bab 5.2.1). Karena kehilangan itu bertepatan dengan frame yang tiba saat alarm RTC penanda selesainya perhitungan IC jatuh tempo, penerima tidak dapat diperbaiki tanpa mengubah desain jalur bangun, sehingga ditetapkan toleransi kurang dari 0,25 % per run (Sub-bab 3.4.5).

**(b) Sub-bab 5.2.1, tambahkan paragraf setelah "Total frame hilang pada run sah adalah 13 frame, seluruhnya pada K4. Run gugur tidak dianalisis." (hal. 60):**
> Kehilangan frame pada K4 memiliki mekanisme yang dapat dijelaskan. Pada 41 run K4 terdapat 42 frame hilang dan 31 tanda tangan yang berdurasi lebih dari 150 ms; seluruh tanda tangan itu berdurasi 210,20 ms (simpangan baku 0,30 ms), dan tidak ada satu pun pada 17 run tanpa kehilangan (latensi maksimum 117,6 ms). Pengukuran langsung pada 16 siklus barrage menunjukkan 8 frame hilang dan 8 tanda tangan lambat, satu banding satu; pada tanda tangan lambat itu host terbangun oleh frame pada tidur ke-96,26-96,28 ms, tepat saat alarm "IC siap" jatuh tempo, lalu tidur satu interval tambahan sekitar 96 ms. Tanda tangan tetap sah (kegagalan tanda tangan nol) dan hanya memakan waktu sekitar 94 ms lebih lama pada sekitar 0,15 % permintaan. Penyebab di tingkat register, kemungkinan terhapusnya penanda alarm RTC saat sinkronisasi setelah Stop, belum dibuktikan dan diperlakukan sebagai dugaan. Sekitar 11 kehilangan pada data akhir tidak disertai tanda tangan lambat, sehingga ada mekanisme kehilangan kedua yang porsinya kecil.

**(c) Sub-bab 5.4 (hal. 66), akhir paragraf pertama:** tambahkan "Latensi maksimum K4 adalah sekitar 210 ms akibat kejadian pada Sub-bab 5.2.1; kejadian ini menambah rerata latensi K4 sekitar 0,08 ms."

**(d) Sub-bab 5.12 butir 5:** lihat T6.

### T5. Dekomposisi energi per tanda tangan (C1)
**Sumber:** `tools/dekomposisi_energi.py`, `data/analisis/stop_final/dekomposisi_energi.md`, gambar `data/analisis/stop_final/gambar_bab4/gambar_4_12_dekomposisi_energi.png`.

**(a) Subbab baru 5.6.4, sesudah 5.6.3 (hal. 70):**
> **5.6.4 Dekomposisi Energi per Tanda Tangan**
> Energi dinamis per tanda tangan pada kondisi normal diuraikan menurut fase pin status, yaitu CPU aktif, CPU tidur di dalam episode (pada hardware offloading berarti IC sedang menghitung), dan tidur di luar episode (Tabel 5.x dan Gambar 5.x). Jumlah ketiga fase sama dengan energi per permintaan utama (selisih 0,00 %).
>
> | Kelompok | Fase | Durasi (ms/tanda tangan) | Arus rerata (mA) | Energi dinamis (mJ) | Porsi |
> |---|---|---|---|---|---|
> | K1 | CPU aktif (hitung perangkat lunak) | 664,1 | 13,82 | 40,98 | 99,9 % |
> | K1 | tidur di luar episode | 359,0 | 2,02 | 0,05 | 0,1 % |
> | K2 | CPU aktif (kirim perintah dan baca hasil) | 18,6 | 13,01 | 1,07 | 35,5 % |
> | K2 | IC menghitung (CPU tidur) | 100,9 | 5,56 | 1,88 | 62,4 % |
> | K2 | tidur di luar episode | 897,5 | 2,00 | 0,06 | 2,0 % |
>
> Pada perangkat lunak, 99,9 % energi dinamis berasal dari CPU yang aktif selama sekitar 664 ms pada 13,8 mA. Pada DS28E38 CPU aktif hanya 18,6 ms, yaitu 36 kali lebih singkat, dan selama IC menghitung arus papan 5,56 mA atau sekitar 3,6 mA di atas arus diam. Penurunan energi per tanda tangan sekitar 13,6 kali (41,0 menjadi 3,0 mJ) karenanya berasal dari waktu CPU aktif yang jauh lebih singkat dan arus selama menunggu yang 2,5 kali lebih rendah daripada arus CPU. Energi terbesar pada K2 (62 %) justru berasal dari IC yang sedang menghitung. Angka 3,6 mA adalah batas atas bagi IC, karena tarikan regulator dan papan pada rel 3,3 V tidak terpisah. Nilai mutlak mJ mengikuti kalibrasi shunt (Sub-bab 5.2.4).

**(b) Sub-bab 5.11.1 (hal. 76):** pada kalimat "...sehingga efisiensi energi bersih pada tingkat papan tetap positif." tambahkan "Dekomposisi energi (Sub-bab 5.6.4) menunjukkan bahwa fase IC menghitung menyumbang 62 % energi dinamis K2 dan tetap jauh lebih kecil daripada energi CPU aktif pada perangkat lunak."

**(c) Gambar:** salin `gambar_4_12_dekomposisi_energi.png` sebagai gambar baru; penomoran otomatis Word.

### T6. Sub-bab 5.12 Keterbatasan Penelitian: daftar pengganti lengkap (hal. 80-81)
Butir comb dihapus (H10); butir keabsahan tanda tangan dan frame hilang diperbarui; ditambah butir dari analisis ancaman validitas (C6) dan batas antrean (C4). Daftar baru (16 butir):

1. Kalibrasi shunt INA226 terhadap acuan presisi dan pembanding multimeter belum dilakukan, sehingga nilai mutlak arus, daya, dan energi masih sementara. Persentase penghematan tidak terpengaruh. *(ganti setelah A1 selesai; lihat M1)*
2. Seluruh run berasal dari satu papan host dan satu IC DS28E38, sehingga variasi antarunit perangkat tidak terukur.
3. Latensi dan tingkat layanan hardware offloading mencerminkan konfigurasi overdrive dan polling; kontribusi overdrive, polling, dan mode Stop tidak dipisahkan.
4. **(diubah)** Sekitar 0,1-0,2 % frame UART hilang pada barrage hardware offloading. Kehilangan ini bertepatan satu banding satu dengan tanda tangan berdurasi sekitar 210 ms yang terjadi ketika frame tiba tepat saat alarm RTC penanda selesainya perhitungan IC jatuh tempo (Sub-bab 5.2.1). Tanda tangan tetap sah, kehilangan dibatasi kurang dari 0,25 % per run, dan latensi maksimum K4 sekitar 210 ms. Penyebab di tingkat register belum dibuktikan.
5. Arus diam papan sekitar 1,99 mA didominasi LED daya yang sengaja dipertahankan, sehingga nilai mutlak daya rata-rata mencerminkan papan ini, bukan konsumsi cip. Daya dinamis dihitung terhadap daya diam ini dan hanya sah untuk membandingkan kedua arsitektur pada data ini.
6. Latensi berarti lama pembuatan satu tanda tangan, bukan waktu tunggu permintaan. Latensi software-based tidak memuat pengumpulan sumber acak dan pemulihan jam (selisih tetap sekitar 9,3 ms terhadap pin status), sedangkan latensi hardware offloading berselisih sekitar 3,3 ms.
7. **(diubah, C4)** Kebijakan tanpa antrean merupakan variabel kontrol; dengan antrean, tingkat layanan dapat berbeda tetapi waktu tunggu bertambah. Waktu tunggu tidak diukur. Karena host hanya menampung satu permintaan tertunda, waktu respons terburuk dibatasi sekitar dua kali latensi (sekitar 233 ms pada K4 dan sekitar 1,31 s pada K3); batas ini diturunkan dari struktur firmware, bukan diukur.
8. **(diubah, C6)** Pengujian dilakukan pada suhu ruang tanpa pengukuran suhu, pada catu USB (bukan baterai), dan hanya dengan mode Stop; mode Standby tidak diuji. Jam inti 48 MHz dengan osilator internal, sehingga latensi absolut perangkat lunak tidak dapat dibandingkan langsung dengan penelitian lain yang memakai 72 MHz.
9. Landasan 24 instruksi NOP setelah perintah tidur merupakan perbaikan empiris; gejala serupa tercatat pada errata STM32F103 (STMicroelectronics, 2015), tetapi akar penyebab pada papan ini tidak diselidiki.
10. Uji jalur galat hanya dilakukan pada hardware offloading dan tidak mencakup gangguan osilator LSE maupun penurunan tegangan catu. Ketahanan fisik DS28E38 juga tidak diuji.
11. **(diubah, T1)** Keabsahan kriptografis tanda tangan tidak diperiksa per run pada data akhir. Keabsahannya diverifikasi pada tahap pengembangan dan pada jalur layanan firmware varian Stop dengan build diagnostik (30 dari 30 untuk tiap arsitektur), bukan pada biner data akhir itu sendiri.
12. Pemakaian RAM firmware varian Stop belum diukur, dan PCB logger serta attacker node telah dirancang tetapi belum difabrikasi.
13. **(baru, C6)** Urutan empat blok ABBA mengurangi drift waktu tetapi bukan pengacakan penuh; ketergantungan pada blok diperiksa dengan analisis leave-one-block-out (Sub-bab 5.2.3).
14. **(baru, C6)** Penyerang dimodelkan sebagai pengirim permintaan periodik pada laju tetap (1 dan 10 permintaan/detik); penyerang adaptif tidak diuji.
15. **(baru, C6)** Pengujian terbatas pada satu algoritme dan satu kurva (ECDSA P-256), sehingga hasil tidak otomatis berlaku untuk algoritme atau perangkat lain.
16. **(baru, C6)** Batas frame hilang pada barrage hardware offloading ditetapkan setelah hasil awal terlihat; pengaruhnya terhadap kesimpulan diperiksa pada Sub-bab 5.2.6 dan tidak mengubah keputusan uji. Karena simpangan baku antar-run sangat kecil, nilai p sangat kecil, sehingga kesimpulan didasarkan pada besar selisih dan interval kepercayaannya.

Catatan: bila Anda ingin 5.12 lebih ringkas, butir 13-16 dapat digabung menjadi satu butir "Ancaman terhadap validitas lainnya".

### T7. Implikasi umur baterai (C7)
**Sumber:** `tools/proyeksi_baterai.py`, `data/analisis/stop_final/proyeksi_baterai.md`. Asumsi baterai (1000 mAh, 3,7 V, regulator diabaikan) **disetujui Anda** (20 November). Nilai mutlak menunggu kalibrasi.

Subbab baru **5.11.9 Implikasi bagi Umur Baterai** (sesudah 5.11.8), dan ubah kalimat pembuka 5.11 (hal. 75) dari "Sub-bab 5.11.6-5.11.8" menjadi "Sub-bab 5.11.6-5.11.9":

> Untuk memberi gambaran praktis, hasil diproyeksikan ke baterai ilustratif 1000 mAh pada 3,7 V (13,3 kJ), tanpa memperhitungkan regulator (Tabel 5.x). Proyeksi ini dihitung dari energi dan arus terukur, bukan umur baterai yang diukur.
>
> | Kelompok | Energi per layanan (mJ) | Tanda tangan per baterai (ribu) | Arus rerata (mA) | Umur dengan lantai papan (jam) | Umur tanpa lantai (jam) | Tingkat layanan |
> |---|---|---|---|---|---|---|
> | K1 software, normal | 41,03 | 325 | 9,68 | 103 | 130 | 100 % |
> | K2 DS28E38, normal | 3,01 | 4.426 | 2,55 | 392 | 1.767 | 100 % |
> | K3 software, barrage | 40,59 | 328 | 13,72 | 73 | 85 | 15,0 % |
> | K4 DS28E38, barrage | 3,00 | 4.440 | 6,91 | 145 | 203 | 85,5 % |
>
> Pada beban normal, DS28E38 memperpanjang umur dari sekitar 103 menjadi sekitar 392 jam (3,8 kali) dengan lantai papan 1,99 mA, dan menghasilkan sekitar 13,6 kali lebih banyak tanda tangan per baterai. Serangan memperpendek umur 29 % pada perangkat lunak (103 menjadi 73 jam) sambil hanya melayani 15 % permintaan, dan 63 % pada DS28E38 (392 menjadi 145 jam) sambil melayani 85,5 %; karena pekerjaan yang diselesaikan berbeda, umur K3 dan K4 tidak setara. Lantai papan pada arus diam 1,99 mA didominasi LED daya; lantai tingkat cip jauh lebih kecil (puluhan mikroampere), sehingga kolom "tanpa lantai" lebih dekat ke perangkat terintegrasi.

### T8. Penyimpangan terhadap proposal bertanda tangan
Proposal yang telah disahkan memuat rancangan dengan mode Sleep (WFI). Naskah kini memakai Stop, tetapi Tabel 5.3 hanya membandingkan dengan Bab 3 skripsi. Perluas Tabel 5.3 (hal. 59) menjadi **"Penyesuaian terhadap proposal dan rancangan Bab 3"** dan tambahkan kalimat pembuka: "Proposal penelitian yang telah disahkan merancang pengujian pada mode Sleep (WFI). Selama pengembangan mode pengujian dialihkan ke mode tidur dalam (Stop) karena lebih relevan bagi simpul bertenaga baterai (Sub-bab 1.1). Penyesuaian terhadap proposal dan rancangan Bab 3 dirangkum pada Tabel 5.3." Baris tambahan (lima baris yang sudah ada tetap):

| Aspek | Proposal | Pelaksanaan | Dampak |
|---|---|---|---|
| Mode tidur | Sleep (WFI); Stop dan Standby tidak diuji (Batasan 6) | Stop dengan regulator daya rendah dan alarm RTC; Sleep dan Standby tidak diuji | Durasi tidur berarti durasi tidur dalam; H3 disesuaikan |
| Pengukur latensi dan tidur | DWT CYCCNT pada 48 MHz; lama tidur dari cap waktu timer di sekitar WFI | DWT untuk software-based; RTC 32,768 kHz untuk hardware offloading dan lama tidur; frekuensi inti diukur terhadap RTC | DWT berhenti selama Stop |
| Cara akses DS28E38 | Kecepatan standar; latensi pendahuluan ~202,2 ms | Overdrive dengan polling; 116,2 ms | Kapasitas ~4,95 menjadi ~8,60 permintaan/detik; utilisasi barrage 2,0 menjadi 1,17 |
| Uji pendahuluan latensi | ~655,5 ms (SW) dan ~202,2 ms (HW) | ~654,9 ms (SW) dan ~116,2 ms (HW), firmware Stop | Laju uji 1 dan 10 permintaan/detik tetap |
| Urutan blok | Perangkat lunak, DS28E38, perangkat lunak, DS28E38; tiga kali pemrograman | DS28E38, perangkat lunak, perangkat lunak, DS28E38 (ABBA); dua kali pemrograman | Memakai host yang sudah bersih berisi K2S |
| Larik latensi | 400 x 4 byte | 600 x 4 byte | K4 melayani 514-515 tanda tangan per jendela |
| Resistor seri jalur sinyal | 1 k pada attacker ke host | 10 k pada attacker ke host dan logger ke host | Membatasi arus balik antar-papan (Sub-bab 4.2.2) |
| Register DBGMCU_CR | Dinolkan setelah pemrograman | Dinolkan firmware setelah jendela aman 5 detik; ST-Link tidak dioperasikan selama pengukuran | Papan tetap dapat diprogram |
| Papan host | Regulator 3,3 V | Regulator XC6219B332MR, kristal 32,768 kHz, resistor LED pada USB-C dilepas | Syarat mode Stop dan lantai arus |
| Kelas RFC 7228 | Di antara Kelas 1 dan 2 | Mendekati Kelas 1 (Flash 64 KB di bawah 100 KiB) | Penyempurnaan klasifikasi |
| Alat | Osiloskop GW Instek GDS-2000A (opsional) dan Jamovi tercantum | Tidak dipakai; dihapus dari Tabel 3.3 dan 3.4 | *Konfirmasi Anda (M4)* |

### T9. Pelaporan uji hipotesis (B5), Tabel 5.12 (hal. 74)
**Sumber:** `data/analisis/stop_final/sensitivitas/p_nilai.csv`. Tambahkan kolom "Welch t (df)" dan tulis p sebagai batas:

| Skenario | Uji | Metrik | Selisih atau penghematan [IK 95 %] | Welch t (df) | p Holm | Keputusan |
|---|---|---|---|---|---|---|
| Normal | H0a | Tsign | 82,25 % [82,25; 82,26] | 27.728 (49,3) | < 10⁻¹⁰⁰ | Tolak H0 |
| Normal | H0b | Pdyn | 92,62 % [92,61; 92,64] | 4.410 (39,4) | < 10⁻¹⁰⁰ | Tolak H0 |
| Normal | H0c | SCPU | +63,08 poin [63,08; 63,09] | -35.874 (46,5) | < 10⁻¹⁰⁰ | Tolak H0 |
| Barrage | H0a | Tsign | 82,24 % [82,24; 82,25] | 21.194 (55,4) | < 10⁻¹⁰⁰ | Tolak H0 |
| Barrage | H0b | Pdyn | 58,00 % [57,97; 58,02] | 3.067 (53,4) | < 10⁻¹⁰⁰ | Tolak H0 |
| Barrage | H0c | SCPU | +83,59 poin [83,58; 83,60] | -16.267 (29,1) | < 10⁻¹⁰⁰ | Tolak H0 |

Catatan di bawah tabel: "t dihitung sebagai perangkat lunak dikurangi hardware offloading (Welch-Satterthwaite). Nilai p terkoreksi Holm terbesar adalah 7 x 10⁻¹⁰³, sehingga ditulis di bawah 10⁻¹⁰⁰. Uji Mann-Whitney U memberikan p = 3,0 x 10⁻¹¹ pada keenam perbandingan (nilai minimum untuk 30 lawan 30 data tanpa tumpang tindih)." Nilai ini dibaca dari `banding_sw_hw.csv` (kolom `p_mwu_cek` = 3,019859e-11); catatan di bawah Tabel 5.12 di draft, yang eksponennya terbaca rusak pada teks PDF, perlu dicocokkan dengan nilai ini. Kalimat sesudah tabel tetap.

---

## Kelompok K. Koreksi kecil

| Kode | Lokasi | Sekarang | Perbaikan |
|---|---|---|---|
| K1 | Sub-bab 4.6 (hal. 53) | `ujigalat_stop.py` dan `dsod.py` | `uji_galat_stop.py` dan `ds_od.py` (sesuai Lampiran D dan berkas di repositori). Tambahkan juga `b1_verifikasi.py`. |
| K2 | Intisari (hal. ix) | "...menurunkan energi per permintaan terlayani sebesar 92,6 % (dari sekitar 41 mJ menjadi 3,0 mJ) pada kedua skenario..." | Hapus tanda kurung: "...sebesar 92,6 % pada kedua skenario..." Alasan: mJ mutlak belum dikalibrasi. |
| K3 | Abstract (hal. xi) | "...reduced energy per served request by 92.6 % (from about 41 mJ to 3.0 mJ) in both scenarios..." | Hapus tanda kurung: "...by 92.6 % in both scenarios..." |
| K4 | Sub-bab 2.3.2 (hal. 13) | "IC mendukung kecepatan komunikasi standar dan overdrive pada bus 1-Wire (Maxim Integrated, 2020)." | Tambahkan sumber: "...(Maxim Integrated, 2020, tabel perintah ROM: Overdrive-Skip ROM 3Ch dan Overdrive-Match ROM 69h)". Cantumkan halaman dari PDF datasheet Anda. |
| K5 | Daftar Pustaka | "Datasheet 19-100093 Rev. 2" untuk DS28E38 | Cocokkan nomor dokumen dan revisi dengan halaman judul PDF yang Anda pakai; hasil pencarian saya tidak dapat memastikannya (`docs/referensi_dokumen_E3_2026-11-18.md`). |
| K6 | Lampiran D (Tabel D.1) | Skrip uji tercantum sebagian | Tambahkan `tools/b1_verifikasi.py`, `tools/p256.py`, `tools/analisis_sensitivitas.py`, `tools/dekomposisi_energi.py`, `tools/proyeksi_baterai.py`, `tools/diag_long.py`, `tools/cek_biner.py`, `tools/manifes_firmware.py`, `tools/kalibrasi_shunt.py` (peran masing-masing satu baris). |

---

## Kelompok M. Menunggu kalibrasi, bangku, atau keputusan Anda

| Kode | Butir | Tempat di naskah | Yang dibutuhkan |
|---|---|---|---|
| M1 | **A1 kalibrasi shunt INA226** (`docs/prosedur_kalibrasi_bagian_A.md`) | Tabel 5.3 baris "Kalibrasi shunt" (`[diisi setelah pengukuran]`), Tabel 5.4 baris terakhir (`[...]`), paragraf sesudah Tabel 5.4, 5.2.4, 5.12 butir 1, 6.1 paragraf 2 dan kekurangan, 6.2 saran 1 | `data/kalibrasi/titik.csv`, spesifikasi multimeter, nilai resistor; saya jalankan ulang `analisis_d.py --stop --rshunt --offset --u-rel`, `dekomposisi_energi.py`, `proyeksi_baterai.py`, `gambar_bab4_stop.py --shunt`. Semua angka mutlak (Tabel 5.8, 5.6.1, 5.6.2, 5.6.4, 5.11.9) berubah. |
| M2 | **A2 offset nol INA226** | 3.4.5 atau 4.6 (belum disebut di naskah) | Setelah diukur, tambahkan kalimat: "Offset nol INA226 ditetapkan sebesar [nilai] satuan (2,5 µV per satuan) berdasarkan pembacaan beban nol; analisis memakai [−2,0] satuan." |
| M3 | **A3 pull-up 1-Wire** | 4,7 kΩ tertulis sebagai fakta di Tabel 3.3 baris 1, 3.2.2, 3.3.3 b, 4.2.1, Tabel 3.5 butir 7 | Nilai terukur PA15-3V3 (DS9121 dicabut dan terpasang). Bila tidak diukur, tulis "nominal 4,7 kΩ (tidak diukur)". |
| M4 | **A4 osiloskop dan Jamovi** | Dihapus dari Tabel 3.3 dan 3.4 (T8) | Konfirmasi bahwa keduanya memang tidak dipakai. |
| M5 | **C2 angka literatur** | Tabel 2.1 dan Tabel 5.14 | Cek langsung ke makalah: Noseda dkk. (27-50 % waktu, 51-87 % energi), Schläpfer dan Rüst (60 ms-1 s), Peradeniya (336 ms). Hasil Anda 17,7 % dan 7,3 % konsisten dengan rentang itu. |
| M6 | **C7 asumsi baterai** | Subbab 5.11.9 (T7) | **Selesai**: asumsi 1000 mAh, 3,7 V disetujui. |
| M7 | **E4 gambar ulang** | Gambar 2.1, 3.1, 3.2, 3.4; Gambar 5.x baru (T5) | Digambar ulang; saya tidak dapat memeriksa tampilan gambar. |
| M8 | **Persiapan sidang** | `laporan/antisipasi_pertanyaan_penguji_2026-11-18.md` | **Selesai**: butir 19 ditambahkan (jawaban untuk pertanyaan B2/comb). Butir 3 sengaja tidak diubah. |
| M9 | Bagian pembuka | Pembimbing dan penguji (halaman pengesahan, prakata), Prakata butir 4, daftar isi/tabel/gambar | Isi nama; Update Field. |
| M10 | Demo sidang | Arus demo K2S.A 3,12 vs 2,55 mA di skripsi; penyebab belum diuji | Tidak masuk naskah; uji dengan ST-Link dicabut dan POR bersih bila ingin dijelaskan. |

---

## Urutan pengerjaan yang disarankan

1. Kelompok H (hapus varian): paling cepat dan memengaruhi penomoran tabel Bab 4, jadi lakukan **pertama**.
2. T3, T9, K1-K6: koreksi cepat tanpa data baru.
3. T1, T2, T4, T5, T6, T8: penambahan isi dari bahan yang sudah ada.
4. T7 (asumsi baterai sudah disetujui).
5. M1-M3 (kalibrasi dan pull-up) di bangku; sesudahnya saya jalankan ulang analisis dan perbarui semua angka mutlak.
6. Update Field, periksa nomor tabel dan gambar, lalu periksa akhir.

## Lampiran: sumber angka dalam laporan ini

| Angka | Sumber |
|---|---|
| B1 30/30, DIAG_SIG, kalimat naskah | `docs/hasil_B1_verifikasi_tanda_tangan_2026-11-18.md` |
| Sidik firmware, ukuran, build-id logger | `data/manifes_firmware_stop.md` dan `.json` |
| Ambang 0 / 0,25 / 0,35 / 1,0 %; 17 / 30 / 36 / 41 run | `docs/bahan_pembahasan_bab4_2026-11-18.md` bagian 1; `sensitivitas/toleransi_k4.csv` |
| Frame hilang 42, tanda tangan lambat 31, 210,20 ms; 8 lawan 8 | `docs/diagnosis_frame_hilang_K2S_2026-11-18.md` |
| Dekomposisi energi | `data/analisis/stop_final/dekomposisi_energi.md` |
| Proyeksi baterai | `data/analisis/stop_final/proyeksi_baterai.csv` dan `.md` |
| t, df, p | `data/analisis/stop_final/sensitivitas/p_nilai.csv` |
| Batas waktu respons 2 x latensi | `docs/bahan_pembahasan_bab4_2026-11-18.md` bagian 7 (dari struktur kode, belum diukur) |
| Proposal | `paper/[ttd]Proposal Skripsi_...pdf`, Bab 1.3, 3.1, 3.2, 3.4, 3.5 |
