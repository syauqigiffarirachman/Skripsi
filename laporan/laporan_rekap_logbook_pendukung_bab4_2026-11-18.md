# Laporan Rekapitulasi Logbook Penelitian 17–18 November 2026 (Lanjutan)

**Dokumen Pendukung Bab 4 (Hasil dan Pembahasan) dan Bab 5 (Kesimpulan): Perubahan Data Penelitian ke Varian *Deep Sleep* (Stop Mode)**

Judul penelitian (ringkas): Perbandingan pembuatan tanda tangan digital ECDSA P-256 secara perangkat lunak (*software*) pada mikrokontroler STM32F103 dengan pengalihan ke chip keamanan DS28E38 (*hardware offloading*) pada kondisi normal dan kondisi serangan *barrage* (*Denial of Sleep*).

Tanggal penyusunan: 18 November 2026
Sumber: logbook harian 17 dan 18 November 2026, rencana varian *deep sleep* (`docs/rencana_deep_sleep.md`), berkas hasil analisis statistik data *deep sleep* (`data/analisis/stop_final/`, keluaran 18 November 2026 pukul 14.49), ringkasan Bab 4 (`docs/ringkasan_bab4_varian_stop.md`), serta laporan rekap sebelumnya.

**Hubungan dengan laporan sebelumnya.** Dokumen ini adalah **lanjutan** dari *Laporan Rekapitulasi Logbook Penelitian 10–16 November 2026* (`laporan/laporan_rekap_logbook_pendukung_bab4_2026-11-17.md`, selanjutnya disebut "Rekap 1"). Rekap 1 menguraikan pembuatan alat, pengambilan data tahap D, dan analisisnya. Dokumen ini menguraikan apa yang terjadi sesudahnya, menyatakan secara tegas **data mana yang kini dipakai untuk skripsi**, dan menunjukkan bagian Rekap 1 mana yang berubah peran. Nomor bagian di dokumen ini sengaja mengikuti urutan Rekap 1 agar mudah dibandingkan.

---

## Daftar Isi

0. Ringkasan Perubahan (baca ini lebih dulu)
1. Pendahuluan
2. Daftar Istilah Tambahan
3. Keputusan Perubahan Data yang Dipakai
4. Uraian Kegiatan per Hari
   - 4.1 Selasa, 17 November 2026
   - 4.2 Rabu, 18 November 2026 (dini hari sampai pagi)
   - 4.3 Rabu, 18 November 2026 (siang)
   - 4.4 Rabu, 18 November 2026 (sore)
   - 4.5 Rabu, 18 November 2026 (malam)
5. Rekapitulasi Hasil Akhir (Data *Deep Sleep*)
6. Pembahasan Pendukung
7. Bukti Keandalan dan Keabsahan Data
8. Perubahan terhadap Rencana Awal (Proposal) dan terhadap Rekap 1
9. Keterbatasan dan Hal yang Belum Tuntas
10. Bahan Penyusunan Kesimpulan dan Saran (Revisi)
11. Lampiran

---

## 0. Ringkasan Perubahan (baca ini lebih dulu)

| Pertanyaan | Jawaban |
|---|---|
| Data apa yang dipakai untuk skripsi? | **Data varian *deep sleep* (Stop mode)**: 4 kelompok × 30 run sah + pengukuran diam (IDLE), diambil 18 November 2026. |
| Bagaimana nasib data tahap D (15 November)? | **Tetap disimpan, tidak dihapus, tetapi berperan sebagai data awal pengembangan saja.** Angka tahap D tidak lagi menjadi angka utama Bab 4. |
| Mengapa berubah? | Varian *deep sleep* mencerminkan perangkat IoT bertenaga baterai yang tidur dalam di antara permintaan, sehingga paling relevan dengan topik *Denial of Sleep*, dan menjawab pertanyaan yang sama (software vs DS28E38, normal vs *barrage*) dengan alat dan prosedur yang sama. Alasan lengkap di Bagian 3.2. |
| Apa yang sama? | Rumusan masalah, hipotesis, desain 2 × 2, 30 ulangan, 60 detik per run, alat uji (host, penyerang, pencatat, INA226), konfigurasi software (Mbed TLS 1c), prosedur analisis (Welch + Holm + bootstrap). |
| Apa yang berbeda? | (1) Mode tidur host: **Stop** (jam utama mati, alarm RTC); (2) K2 memakai **overdrive 1-Wire + polling**; (3) prosedur "keadaan bersih" (daya penuh dimatikan tanpa debugger) wajib; (4) toleransi frame hilang < 0,25 % pada *barrage* varian Stop. |
| Apa yang perlu direvisi di Bab 3? | Bagian 3.5 di bawah (daftar revisi). |
| Hal yang tidak boleh dilupakan | Latensi dan tingkat layanan K2 mencerminkan konfigurasi overdrive + polling (bukan kecepatan biasa). Lihat Bagian 9. |

**Hasil inti data *deep sleep*** (rerata, N = 30 per kelompok):

| | K1 Software normal | K2 DS28E38 normal | K3 Software barrage | K4 DS28E38 barrage |
|---|---|---|---|---|
| Latensi tanda tangan | 654,9 ms | 116,2 ms | 654,9 ms | 116,3 ms |
| Tidur CPU | 35,08 % | 98,17 % | 0,38 % | 83,97 % |
| Tingkat layanan | 100 % | 100 % | 15,04 % | 85,50 % |

DS28E38 lebih cepat **82,3 %**, menghemat daya dinamis **92,6 %** (normal) dan **58,0 %** (*barrage*), dan menambah waktu tidur CPU **63,1 poin** (normal) dan **83,6 poin** (*barrage*). Seluruh pemeriksaan keutuhan data **memenuhi standar**.

---

## 1. Pendahuluan

### 1.1 Latar belakang penyusunan

Pada pengembangan awal (10–16 November, Rekap 1) host memakai mode tidur ringan (Sleep/WFI), yaitu hanya inti prosesor berhenti sementara jam, bus, dan periferal tetap menyala. Datanya (disebut tahap D) disimpan sebagai **data awal pengembangan**. Pada 17 November diputuskan menambah **varian *deep sleep* (Stop mode)**, yaitu mode tidur yang mematikan jam utama dan hampir seluruh rangkaian, sehingga arus diam mendekati lantai papan.

Pada 18 November varian itu selesai dibangun, diuji, dan dipakai mengambil data final. Peneliti memutuskan memakainya sebagai satu-satunya data dan analisis yang sah untuk skripsi (Bagian 3). Dokumen ini merangkum seluruh proses, hasil, dan perubahannya agar dapat dipakai sebagai:

1. bahan pendukung penulisan Bab 4 yang memakai data *deep sleep*,
2. dasar revisi Bab 3 dan penarikan kesimpulan Bab 5,
3. bukti tertulis proses, termasuk kesalahan dan koreksinya, bagi pembimbing dan penguji.

### 1.2 Cakupan

Dokumen ini mencakup kegiatan 17 November (rencana dan uji dasar *deep sleep*) dan 18 November (uji tahap lanjut, penyelesaian firmware, pengambilan data final, uji jalur galat, analisis). Bagian yang sama persis dengan Rekap 1 (desain percobaan, susunan alat uji, besaran yang diukur) tidak diulang; ia hanya dicatat bila berubah.

### 1.3 Catatan kejujuran data

- Angka di dokumen ini disalin dari berkas analisis dan logbook, bukan dari ingatan.
- Beberapa pengukuran **dibuang** karena dilakukan dalam keadaan yang belakangan terbukti tidak sah (debugger aktif, lihat Bagian 7.5). Data itu **tidak** dipakai dan tidak dianalisis, tetapi kejadiannya dicatat di sini supaya runtut.
- Beberapa dugaan yang dibuat selama pengerjaan terbukti **salah** dan dikoreksi pada hari yang sama (mis. dugaan bahwa ST-Link penyebab *crash*, dan dugaan bahwa DS28E38 penyebab arus diam lebih tinggi). Koreksinya dicatat di Bagian 4.
- Nilai mutlak arus, daya, dan energi masih memakai hambatan shunt nominal 0,100 ohm. Kalibrasi belum selesai (Bagian 9). Persentase penghematan tidak terpengaruh.
- Kriteria keabsahan data varian *deep sleep* punya **satu ketentuan khusus**: frame permintaan hilang di jalur penyerang–host ditoleransi sampai kurang dari 0,25 % per run pada skenario *barrage*, atas keputusan peneliti (Bagian 7.1). Frame **rusak** tetap tidak boleh ada.

---

## 2. Daftar Istilah Tambahan

Istilah dari Rekap 1 (Bagian 2) tetap berlaku. Berikut istilah baru.

| Istilah | Arti sederhana |
|---|---|
| *Deep sleep* / Stop mode | Mode tidur dalam: jam utama, bus, dan hampir semua periferal mati. Hanya jam kecil (RTC) dan sedikit rangkaian yang tetap hidup. Arus diam sangat rendah. |
| Sleep / WFI | Mode tidur ringan yang dipakai di tahap D: hanya inti prosesor berhenti, jam dan periferal tetap hidup. Arus diam jauh lebih besar. |
| K1S, K2S | Firmware K1 (software) dan K2 (DS28E38) versi Stop mode. |
| RTC, LSE | Jam kecil berkristal 32,768 kHz yang tetap hidup saat Stop. Dipakai sebagai alarm dan penghitung waktu tidur. |
| Overdrive 1-Wire | Mode cepat jalur komunikasi 1-Wire ke DS28E38 (sekitar 8 kali lebih cepat dari biasa). |
| Polling | Sesekali menanyakan ke chip "sudah selesai?" alih-alih menunggu waktu tetap. |
| POR penuh | Mematikan daya papan host sepenuhnya (USB-C, ST-Link, semua kabel sinyal dan GND dicabut, tunggu 10 detik, lalu dipasang kembali). |
| Keadaan bersih | Papan host dinyalakan lewat POR penuh dan tidak disentuh debugger sama sekali sebelum dan selama pengukuran. |
| *Debug power* | Bagian debug dari chip yang dinyalakan ST-Link/OpenOCD dan tidak mati sampai daya putus. Menaikkan arus diam dan mengubah perilaku bangun. |
| Lantai (arus diam) | Arus papan saat host tidur dan tidak melayani apa pun. Di sini sekitar 1,99 mA, kebanyakan milik LED daya dan komponen papan, bukan prosesor. |
| Bangun palsu | Host bangun dari tidur bukan karena alarm atau permintaan yang sah. |
| NOP | Instruksi "tidak melakukan apa-apa"; dipakai sebagai landasan agar instruksi penting tidak dijalankan terlalu cepat setelah bangun. |
| ABBA | Pola urutan blok untuk mengendalikan pergeseran waktu/suhu: K2S, K1S, K1S, K2S. |
| Toleransi frame hilang | Batas jumlah paket permintaan yang boleh tidak sampai di UART: kurang dari 0,25 % dari yang tiba per run (hanya *barrage* varian Stop). |

---

## 3. Keputusan Perubahan Data yang Dipakai

### 3.1 Keputusan

Pada 18 November 2026 peneliti memutuskan:

1. **Data yang dipakai sebagai hasil skripsi adalah data varian *deep sleep*** (4 × 30 run sah + IDLE, `data/tahap_stop.txt`) beserta seluruh analisisnya (`data/analisis/stop_final/`) dan data pendukungnya (Lampiran D).
2. **Data tahap D (*Sleep*/WFI, `data/tahap_d.txt`, analisis `data/analisis/20261116_012530_r0.1/`) dipertahankan sebagai data awal pengembangan saja**: bukti proses dan rujukan bahwa sistem ukur sudah tervalidasi sebelum varian *deep sleep* dibuat. Ia tidak dihapus dan tidak diubah.
3. Toleransi frame hilang kurang dari 0,25 % per run diberlakukan untuk *barrage* varian Stop (K4); frame rusak tetap harus nol.

### 3.2 Alasan

1. **Relevansi dengan topik.** *Denial of Sleep* hanya bermakna bila perangkat benar-benar tidur dalam di antara permintaan, seperti perangkat IoT bertenaga baterai. Varian Stop memenuhi syarat itu; arus diam turun ke lantai papan sebenarnya (1,99 mA).
2. **Selisih yang jelas terhadap lantai papan.** Beban yang berasal dari prosesor dan chip dapat dibedakan dari beban tetap papan.
3. **Pertanyaan penelitian tidak berubah.** RM1–RM3 dan H1–H4 dijawab dengan desain 2 × 2, 30 ulangan, 60 detik per run, alat ukur yang sama, dan prosedur analisis yang sama (Welch + Holm + bootstrap).
4. **Seluruh hipotesis didukung** oleh data *deep sleep* (Bagian 5.6).

### 3.3 Status data

| Himpunan data | Lokasi | Peran baru | Dipakai di Bab 4? |
|---|---|---|---|
| Data tahap D (15 Nov), 120 run sah | `data/runs/<sesi>/`, manifest `data/tahap_d.txt`; analisis `data/analisis/20261116_012530_r0.1/` | Data awal pengembangan (bukti proses; **bukan** data hasil) | **Tidak**; tidak dianalisis ulang dan tidak dibandingkan |
| **Data *deep sleep* final (18 Nov), 4 × 30 run sah + IDLE** | `data/runs/<sesi>/`, manifest `data/tahap_stop.txt`; analisis `data/analisis/stop_final/` | **Data utama skripsi** | **Ya, semua angka utama** |
| Data pilot *deep sleep* | `data/runs/20261118_094801_K1K2`, `…_093427_K2S`, dll. (analisis lengkap pilot tidak dihasilkan karena kelompok K4 tanpa run sah; analisis lengkap berhenti di tengah, hanya keluaran sebagian di `data/analisis/pilot_stop/`) | Data pengembangan (bukan hasil) | Tidak |
| Data pendukung pengembangan (overdrive, LSE, S0, S1, S2) | `data/dev/ds_od`, `s0a`, `s0`, `s1`, `s2` | Bukti rancangan dan keputusan | Sebagai pendukung (Lampiran D) |
| Data uji jalur galat | `data/dev/err/` | Bukti ketahanan | Ya, subbab ketahanan |
| Run dan sesi gugur | `data/dev/gugur/` | Dicatat, tidak dianalisis | Tidak |

### 3.4 Peta hubungan dengan Rekap 1

| Bagian Rekap 1 | Perubahan peran |
|---|---|
| 1 Pendahuluan, 2 Daftar Istilah, 3 Gambaran Umum | **Tetap berlaku.** Bagian 2 di dokumen ini hanya menambah istilah. |
| 4.1–4.7 Kegiatan 10–16 Nov | **Tetap berlaku sebagai riwayat pengembangan.** Dokumen ini melanjutkan dengan 4.1–4.5 (17–18 Nov). |
| 5 Hasil akhir tahap D | **Berubah peran: data awal pengembangan.** Angka utama Bab 4 kini diambil dari Bagian 5 dokumen ini. |
| 6 Pembahasan pendukung | Pada dasarnya tetap benar secara prinsip; angkanya diperbarui di Bagian 6 dokumen ini. |
| 7 Bukti keandalan | Prinsip tetap; angka dan prosedur diperbarui di Bagian 7 dokumen ini. |
| 8 Perubahan terhadap rencana awal | Diperluas di Bagian 8 dokumen ini. |
| 9 Keterbatasan | Diperbarui di Bagian 9 (beberapa butir lama tetap berlaku, ditandai). |
| 10 Bahan kesimpulan | Diganti oleh Bagian 10 dokumen ini. |

### 3.5 Dampak pada Bab 3 (revisi yang perlu dibuat)

Rumusan masalah, hipotesis, dan prosedur analisis tidak berubah. Yang perlu diubah atau ditambahkan:

1. **Mode tidur host:** dari Sleep/WFI menjadi **Stop mode** (jam RTC/LSE 32,768 kHz sebagai alarm dan penghitung waktu tidur; bangun dari Stop selalu pada HSI 8 MHz lalu PLL 48 MHz dipulihkan).
2. **Definisi K2:** DS28E38 dengan **overdrive 1-Wire** dan **polling** (menunggu chip sekitar 107 ms di Stop, lalu membaca hasil), bukan kecepatan biasa dengan tunggu tetap 130 ms. K1 (Mbed TLS konfigurasi 1c) **tidak berubah**.
3. **Cara membangunkan host** tanpa mengubah papan penyerang: sinkron bit dari byte pertama 0xA5 (Bagian 4.1 dan 4.2).
4. **Sumber waktu:** lama tidur dan total jendela dari RTC (LSE), bukan dari penghitung siklus prosesor yang berhenti saat Stop; dibandingkan dengan jam kristal papan pencatat (Bagian 7.2).
5. **Prosedur keadaan bersih** untuk pengukuran varian Stop (Bagian 7.5).
6. **Kriteria keabsahan data varian Stop** (toleransi frame hilang, Bagian 7.1).
7. **Urutan blok:** K2S, K1S, K1S, K2S (ABBA yang dimulai dari K2S), 15 A + 15 B per blok untuk K1S dan K2S (K2S memakai lebih banyak run B untuk menggantikan run yang melewati toleransi).
8. **Pernyataan keterbatasan:** latensi dan tingkat layanan K2 mencerminkan konfigurasi overdrive + polling (Bagian 9).

---

## 4. Uraian Kegiatan per Hari

### 4.1 Selasa, 17 November 2026: Rencana *deep sleep* dan uji dasar (langkah 1–7)

**Tujuan:** memutuskan dan menguji dasar varian *deep sleep* tanpa mengganggu data tahap D.

**Kegiatan dan hasil:**

1. **Keputusan (langkah 1).** Stop mode **ditambahkan** sebagai varian; data tahap D tetap dipakai (saat itu masih sebagai data utama). Titik ukur tetap di input 5 V USB-C seperti tahap D.
2. **Persiapan papan host (langkah 2).** Regulator papan XC6219B332MR (arus diam 25 µA tipikal). Resistor LED daya modul USB-C dilepas; LED daya Blue Pill (merah) **sengaja dipertahankan** atas keputusan peneliti. Kristal jam RTC: Epson MC-306 32,768 kHz. Resistor seri 10 kohm dipasang pada jalur pencatat→host dan penyerang→host untuk membatasi arus balik.
3. **Instrumen mikroampere (langkah 3) tidak diperlukan:** lantai papan sekitar 2 mA, kira-kira 80 satuan terkecil sensor INA226.
4. **Uji kristal LSE (langkah 4, 25 uji): LULUS.** Kristal hidup pada 23 dari 23 uji sah. Start dingin 3,5–4,1 s (rerata 3,79 s); frekuensi stabil dengan selisih tetap −47 ppm terhadap kristal pencatat (selisih antar-uji ≤ 0,2 ppm). Dua uji gugur karena kabel host–pencatat terlepas.
5. **Temuan penting: daya balik.** Saat USB-C host dicabut, host tetap berdaya lewat kabel sinyal (dioda proteksi pin). Cabut-pasang USB-C saja **tidak** mematikan host. Prosedur POR bersih ditetapkan: cabut ST-Link, cabut USB-C, lepas GND, tunggu 10 detik, sambung GND dahulu, colok USB-C terakhir. Diduga inilah penyebab pergeseran arus +0,78 mA pada 15 November (Rekap 1, Bagian 9 butir 12).
6. **Lantai Stop minimal (langkah 5): LULUS.** Stop dengan POR bersih tanpa ST-Link: **1,985 mA** (14.922 cuplikan). Dengan ST-Link terpasang setelah flash: 2,567 mA. Selisih ±0,58 mA berasal dari bagian debug yang dinyalakan ST-Link. Sisa arus sekitar 2 mA didominasi LED daya merah.
7. **Stop dengan alarm RTC (langkah 6, S1): LULUS.** Selisih lama tidur menurut host dan menurut pencatat **9–20 µs** per jendela 28,5 detik (batas lulus 1.000 µs); bangun palsu 0.
8. **Bangun karena permintaan penyerang (langkah 7, S2): LULUS.** Masalah: UART mati saat Stop, jadi awal permintaan yang membangunkan host sudah lewat. Penyelesaian tanpa mengubah papan penyerang: byte pertama setiap permintaan selalu 0xA5 dengan pola tepi yang pasti; host mengenali pola itu dan menyalakan penerima sekitar 17 µs sebelum byte kedua. Hasil: **59 dari 59 (1 permintaan/detik) dan 601 dari 601 (10/detik)**, frame rusak 0, hilang 0, bangun palsu 0.
9. Jalan menuju hasil S2 melewati empat kekeliruan yang dikoreksi (pembanding waktu salah awal, satu byte 0x00 hilang karena penerima dinyalakan terlalu dekat, salah hitung di batas jendela, dan alarm yang terhapus terlalu cepat). Semuanya dicatat di logbook 18 November.

**Kaitan dengan Bab 4:** bahan subbab perancangan varian *deep sleep* dan validasi mode tidur. **Tabel D1** (Lampiran D) merangkum uji dasar.

---

### 4.2 Rabu, 18 November 2026 (dini hari sampai pagi): Overdrive DS28E38 dan penggabungan

**Tujuan:** (a) mempercepat tanda tangan DS28E38 sesuai permintaan peneliti, (b) menggabungkan Stop ke firmware K1 dan K2.

**Kegiatan dan hasil:**

1. **Overdrive 1-Wire dan polling.** Catatan 11 November menyebut overdrive pernah dicoba dan chip tidak membalas. Penyebabnya ditemukan: pustaka OneWire yang dipakai tidak mendukung overdrive. Dibuat program sendiri (`src/host/eksperimen/ds_overdrive/`) yang mengatur waktu kedip kabel data sampai per sepersejuta detik.

   | Cara | Waktu satu tanda tangan |
   |---|---|
   | Kecepatan biasa, tunggu tetap 130 ms | 192,5 ms |
   | Kecepatan biasa, polling | 169,6 ms |
   | Overdrive, tunggu tetap 130 ms | 138,7 ms |
   | **Overdrive, polling** | **115,6 ms** (kirim 3,4 + chip 106,6 + baca 5,6) |

   269 tanda tangan overdrive berturut-turut, 0 gagal, **269 dari 269 valid** (diperiksa di komputer, termasuk uji tanda tangan yang sengaja dirusak). Chip selalu butuh 105,9–107,3 ms (simpangan baku 0,22 ms), itu batas bawah. Titik baca bit yang sah 2,0–3,0 µs; dipakai 2,5 µs.
2. **Firmware gabungan K1S dan K2S** dibuat dengan mesin jendela bersama (alarm RTC dimultipleks untuk "akhir jendela" dan "chip selesai").
3. **Kejadian: papan host terkunci.** Salah satu percobaan mematikan port debug saat tidur; papan *crash* sebelum port debug dinyalakan lagi, dan karena pin reset tidak tersambung ke ST-Link, papan tidak dapat dipulihkan dari komputer. Percobaan dihentikan, kode dikembalikan ke konfigurasi yang lulus uji S2, hasil di-*commit* (`e29e2c8`), dan logbook ditulis. Uji gabungan belum lulus saat itu.

**Kaitan dengan Bab 4:** Tabel D2 (Lampiran D) untuk subbab K2 dan latensi.

---

### 4.3 Rabu, 18 November 2026 (siang): Pemulihan, penyebab *crash*, K1S dan K2S lulus

**Tujuan:** memulihkan papan dan menyelesaikan integrasi.

**Kegiatan dan hasil:**

1. **Pemulihan papan.** Jumper BOOT0 dipindah ke 1, daya dicabut-pasang, lalu flash dihapus dan ditulis ulang lewat bootloader bawaan chip (`tools/pulihkan_host.py`). S2 diulang dan **lulus lagi** (59/59 dan 602/602).
2. **Penyebab *crash* setelah bangun dari Stop ditemukan.** Dua dugaan awal **salah**: bukan ST-Link (papan juga *crash* tanpa ST-Link), bukan regulator hemat daya. Penyebab sebenarnya: beberapa instruksi pertama sesudah bangun dari Stop kadang salah dieksekusi (contoh: penambahan 0x1000 terbaca 0x4000; satu instruksi dianggap tak dikenal), bergantung pada tata letak kode. **Obat:** 24 instruksi NOP pendek langsung setelah perintah tidur, dalam satu blok. Percobaan lain (menyimpan semua register, menaruh fungsi di RAM, mematikan regulator hemat daya, mematikan buffer tulis) tidak menolong dan dicatat agar tidak diulang.
3. **Dua kekeliruan lain diperbaiki.** (a) Pada beban penuh jendela molor dari 60 menjadi 87 detik karena batas jendela hanya diperiksa saat akan tidur; (b) satu tidur sangat singkat dapat terbaca mundur 1 tik dan menghasilkan lama tidur raksasa (S_cpu 3.784 %). Kapasitas catatan latensi dinaikkan dari 400 ke 600.
4. **Hasil integrasi** (ST-Link terpasang): K2S A 59/59, 116,2 ms, S_cpu logger 98,19 % = host 98,20 %; K1S A 59/59, 683 ms; K1S B jenuh seperti K1 baseline.

**Kaitan dengan Bab 4:** catatan implementasi (landasan NOP) untuk subbab keterbatasan dan perancangan.

---

### 4.4 Rabu, 18 November 2026 (sore): Pilot dan misteri lantai arus

**Tujuan:** menguji ketahanan K1S/K2S dan memahami mengapa arus diam berubah-ubah.

**Kegiatan dan hasil:**

1. **Uji tanpa ST-Link, lalu pilot.** K2S A dan B tanpa ST-Link lulus (hasil sama dengan dengan ST-Link). Pilot campuran 16 run (K1S dan K2S, ganti firmware otomatis 9 kali) dan pilot K2S 8 run: **0 *crash***. `run_bench.py` diperluas untuk mengenal K1S/K2S.
1b. **Kabel pencatat → host (PA9 → PA10) disambung kembali** oleh peneliti. Sebelumnya kabel itu putus dan firmware K1S/K2S dijalankan dalam mode otomatis (host memulai jendela sendiri dan melapor lewat jalur host → pencatat yang masih hidup); setelah kabel tersambung, `run_bench.py` biasa dipakai untuk semua data final.
2. **Misteri.** Lantai arus pagi tadi 1,95 mA berubah menjadi 2,565 mA pada firmware yang sama; K1S IDLE dan K2S IDLE kembar di 2,565 mA. Dugaan "DS28E38 penyebabnya" dan "ADC penyebabnya" **keduanya salah** (K1S tidak punya DS28E38 dan perbaikan ADC tidak berefek).
3. **Terpecahkan lewat uji POR penuh.** Setelah POR penuh lantai kembali **1,94 mA**; **satu sesi OpenOCD saja menaikkannya ke 2,52 mA** sampai POR penuh berikutnya. Jadi penyebabnya adalah *debug power*, sejalan dengan temuan 17 November (lantai 1,985 vs 2,567 mA). Aturan prosedur: data Stop hanya sah setelah POR penuh dan tanpa sesi OpenOCD (Bagian 7.5).
4. **Dampak ke hasil sebelumnya:** semua angka lantai 2,5–2,95 mA yang terlanjur dilaporkan siang itu berasal dari keadaan debug aktif dan **dibuang**.

---

### 4.5 Rabu, 18 November 2026 (malam): Perbaikan *barrage* K2S, data final, uji galat, analisis

**Tujuan:** menyelesaikan masalah frame hilang di K2S *barrage*, mengambil data final, uji jalur galat, dan analisis.

**Kegiatan dan hasil:**

1. **Frame rusak/hilang di K2S *barrage*.** Pencatat diagnostik menunjukkan dua mekanisme:
   - *Frame rusak:* cek "garis diam" lama hanya melihat satu sampel pin PA3; di 87 µs pertama sebuah frame pin sering tinggi sehingga host tidur di tengah frame dan bangun di tepi acak. **Perbaikan:** PA3 harus tinggi terus 120 µs sebelum tidur → frame rusak 0 dan bangun palsu 0.
   - *Frame hilang:* frame yang mulai tepat saat host pulih dari alarm (penerima mati ±0,3–0,5 ms). **Perbaikan:** pada bangun karena alarm, penerima langsung dinyalakan di jam lambat dan pindah ke jam cepat hanya saat garis diam ketat. Hasil: hilang rata-rata turun dari ±2 menjadi ±0,9 per 600 frame.
   - Sisa ±0,17 % tidak dapat dihilangkan tanpa mengubah desain; peneliti memutuskan toleransi < 0,25 % per run.
2. **Data final (langkah 12).** Urutan ABBA dengan POR penuh tiap ganti firmware dan tanpa OpenOCD: blok 1 K2S (31 dari 35 sah), blok 2 K1S (32 dari 32), blok 3 K1S (32 dari 32), blok 4 K2S (33 dari 40 sah). Hasilnya **tepat 30 run sah per kelompok**. Pemeriksaan keutuhan: **memenuhi standar** (Bagian 7.1).
3. **Uji jalur galat (langkah 10): 7 dari 7 lulus** (Bagian 7.7).
4. **Analisis dan ringkasan Bab 4** (`docs/ringkasan_bab4_varian_stop.md`) dibuat.
5. **Kekeliruan kecil yang dicatat:** dua run percobaan uji galat gagal sebelum mulai karena saya salah mengira batas jendela logger (batasnya 80 detik); datanya dipindah ke `data/dev/gugur/`.

**Kaitan dengan Bab 4:** seluruh Bagian 5–7 di bawah.

---

## 5. Rekapitulasi Hasil Akhir (Data *Deep Sleep*)

Seluruh angka bagian ini berasal dari analisis 18 November 2026 pukul 14.49 (`data/analisis/stop_final/`; 128 run sah, 30 per kelompok). Angka ditulis sebagai rerata, simpangan baku dalam kurung. **Nilai mutlak arus, daya, dan energi masih memakai shunt nominal 0,100 ohm** (Bagian 9). Setelan analisis: offset sensor −2,0 satuan terkecil, ketidakpastian kalibrasi 5 %, bootstrap 10.000 pengambilan ulang (seed 20261116), uji utama Welch dengan koreksi Holm atas enam perbandingan.

### 5.1 Tabel hasil utama

| Besaran | K1 Software normal | K2 DS28E38 normal | K3 Software barrage | K4 DS28E38 barrage |
|---|---|---|---|---|
| Latensi tanda tangan (ms, waktu nyata) | 654,88 (0,09) | 116,23 (0,06) | 654,91 (0,11) | 116,30 (0,09) |
| Persentase tidur CPU (%) | 35,08 (0,008) | 98,17 (0,005) | 0,377 (0,001) | 83,97 (0,03) |
| Daya dinamis (mW) | 40,10 (0,04) | 2,96 (0,02) | 61,18 (0,05) | 25,70 (0,04) |
| Arus dinamis (mA) | 7,69 (0,004) | 0,566 (0,003) | 11,73 (0,003) | 4,93 (0,007) |
| Energi per permintaan terlayani, dinamis (mJ) | 41,03 (0,04) | 3,01 (0,02) | 40,59 (0,04) | 3,00 (0,004) |
| Muatan dinamis per layanan (mC) | 7,87 (0,003) | 0,575 (0,003) | 7,78 (0,002) | 0,575 (0,0007) |
| Daya rata-rata total (mW) | 50,50 (0,04) | 13,33 (0,02) | 71,57 (0,05) | 36,06 (0,04) |
| Arus rata-rata total (mA) | 9,68 (0,004) | 2,554 (0,003) | 13,72 (0,003) | 6,91 (0,008) |
| Energi jendela 60 s (mJ) | 3.048 (2,6) | 800 (1,0) | 4.322 (3,2) | 2.167 (2,7) |
| Arus saat tidur (mA) | 2,04 | 2,37 | 2,08 | 5,63 |
| Tingkat layanan (%) | 100 | 100 | 15,04 | 85,50 |
| Permintaan masuk / dilayani / terlewat | 59 / 59 / 0 | 59 / 59 / 0 | 605 / 91 / 514 | 601–603 / 514–515 / 86–88 |
| Throughput (tanda tangan/detik) | 0,977 | 0,983 | 1,507 | 8,565 |
| Kapasitas maksimum (tanda tangan/detik) | 1,53 | 8,60 | 1,53 | 8,60 |

Lantai IDLE (host sendiri dan semua papan tersambung): **1,99 mA (10,4 mW)** untuk K1S dan K2S.

### 5.2 Perbandingan software dan DS28E38 (RM1, RM3, H1a–H1c)

Arah positif berarti DS28E38 lebih baik. Uji utama Welch dengan koreksi Holm atas enam perbandingan; interval kepercayaan bootstrap 10.000 kali.

| Skenario | Besaran | Software | DS28E38 | Perbaikan [IK 95 %] | p (Holm) | Keputusan |
|---|---|---|---|---|---|---|
| Normal | Latensi | 654,9 ms | 116,2 ms | lebih cepat 82,25 % [82,25; 82,26] (selisih 538,7 ms; 5,6 kali) | 4 × 10⁻¹⁷⁸ | H0 ditolak |
| Normal | Daya dinamis | 40,10 mW | 2,96 mW | hemat 92,62 % [92,61; 92,64] | 2 × 10⁻¹¹³ | H0 ditolak |
| Normal | Tidur CPU | 35,08 % | 98,17 % | naik 63,08 poin [63,08; 63,09] | 3 × 10⁻¹⁷⁴ | H0 ditolak |
| Normal | Energi per layanan | 41,03 mJ | 3,01 mJ | hemat 92,67 % (13,6 kali) | — | pendukung |
| Normal | Arus dinamis | 7,69 mA | 0,566 mA | hemat 92,64 % | — | pendukung |
| Normal | Daya rata-rata total | 50,50 mW | 13,33 mW | hemat 73,61 % | — | pendukung |
| Barrage | Latensi | 654,9 ms | 116,3 ms | lebih cepat 82,24 % [82,24; 82,25] (selisih 538,6 ms) | 3 × 10⁻¹⁹² | H0 ditolak |
| Barrage | Daya dinamis | 61,18 mW | 25,70 mW | hemat 58,00 % [57,97; 58,02] | 3 × 10⁻¹⁴¹ | H0 ditolak |
| Barrage | Tidur CPU | 0,38 % | 83,97 % | naik 83,59 poin [83,58; 83,60] | 7 × 10⁻¹⁰³ | H0 ditolak |
| Barrage | Energi per layanan | 40,59 mJ | 3,00 mJ | hemat 92,61 % (13,5 kali) | — | pendukung |
| Barrage | Arus dinamis | 11,73 mA | 4,93 mA | hemat 57,99 % | — | pendukung |
| Barrage | Daya rata-rata total | 71,57 mW | 36,06 mW | hemat 49,62 % | — | pendukung |
| Barrage | Tingkat layanan | 15,04 % | 85,50 % | naik 70,46 poin | — | (H4) |
| Barrage | Throughput | 1,507/detik | 8,565/detik | 5,68 kali | — | (H4) |

Keenam uji hipotesis utama menolak H0. Sebaran kedua kelompok tidak pernah tumpang tindih pada satu pun besaran (nilai Mann-Whitney 30 lawan 30 mentok di nilai minimum 3 × 10⁻¹¹). **Aturan persen terpenuhi:** rasio terkecil rerata software terhadap ketidakpastian gabungannya adalah 20,0 (syarat ≥ 3).

### 5.3 Pengaruh serangan barrage pada tiap cara (RM2)

| Cara | Besaran | Normal | Barrage | Perubahan |
|---|---|---|---|---|
| Software | Tidur CPU | 35,08 % | 0,38 % | turun 34,71 poin (praktis tidak pernah tidur) |
| Software | Daya dinamis | 40,10 mW | 61,18 mW | naik 52,55 % |
| Software | Tingkat layanan | 100 % | 15,04 % | turun 84,96 poin |
| Software | Throughput | 0,977/detik | 1,507/detik | naik 54,2 % (sudah di batas kapasitas) |
| Software | Energi per layanan | 41,03 mJ | 40,59 mJ | turun 1,1 % |
| DS28E38 | Tidur CPU | 98,17 % | 83,97 % | turun 14,20 poin |
| DS28E38 | Daya dinamis | 2,96 mW | 25,70 mW | naik 768 % (dari dasar yang sangat kecil) |
| DS28E38 | Tingkat layanan | 100 % | 85,50 % | turun 14,50 poin |
| DS28E38 | Throughput | 0,983/detik | 8,565/detik | naik 771 % |
| DS28E38 | Energi per layanan | 3,01 mJ | 3,00 mJ | turun 0,3 % |
| Kedua cara | Latensi | — | — | berubah kurang dari 0,07 % (praktis tetap) |

### 5.4 Interaksi cara dan skenario (RM3, hipotesis interaksi)

| Besaran | Keunggulan DS28E38 saat normal | Saat barrage | Selisih dari selisih [IK 95 %] | Arti |
|---|---|---|---|---|
| Tidur CPU | 63,08 poin | 83,59 poin | +20,51 [20,50; 20,52] poin | keunggulan lebih besar saat diserang (**hipotesis interaksi didukung**) |
| Daya dinamis | 37,14 mW | 35,48 mW | −1,66 [−1,69; −1,64] mW | keunggulan lebih kecil saat diserang |
| Arus dinamis | 7,12 mA | 6,80 mA | −0,32 mA | keunggulan lebih kecil saat diserang |
| Energi per layanan | 38,02 mJ | 37,59 mJ | −0,43 [−0,45; −0,41] mJ | keunggulan sedikit lebih kecil saat diserang (±1,1 % dari selisihnya) |
| Latensi | 538,65 ms | 538,61 ms | −0,04 [−0,10; 0,02] ms | tidak terbukti berbeda |

### 5.5 Tingkat layanan dan kesesuaian model (H4)

| Kelompok | TL terukur | TL model | Selisih | rho (beban) |
|---|---|---|---|---|
| K1 | 100 % | 100 % | 0 | 0,64 |
| K2 | 100 % | 100 % | 0 | 0,11 |
| K3 | 15,04 % | 15,24 % | −0,20 poin | 6,56 |
| K4 | 85,50 % | 85,83 % | −0,33 poin | 1,17 |

Ketiga kriteria H4 terpenuhi: TL 100 % pada kondisi normal, TL DS28E38 jauh lebih tinggi saat *barrage*, dan selisih terhadap model kurang dari 1 poin. **H4 didukung.**

### 5.6 Ringkasan status hipotesis

| Hipotesis | Isi ringkas | Status (data *deep sleep*) |
|---|---|---|
| H1a | Latensi software dan DS28E38 berbeda | Didukung pada kedua skenario (DS28E38 82,2–82,3 % lebih cepat) |
| H1b | Daya dinamis berbeda | Didukung pada kedua skenario (hemat 92,6 % normal, 58,0 % barrage) |
| H1c | Persentase tidur CPU berbeda | Didukung pada kedua skenario (+63,1 dan +83,6 poin) |
| Interaksi | Selisih tidur CPU lebih besar saat barrage | Didukung (+20,51 poin) |
| H4 | DS28E38 meningkatkan tingkat layanan saat barrage tanpa menurunkannya saat normal | Didukung (85,5 % vs 15,0 %; 100 % vs 100 %) |

### 5.7 Sebaran per kelompok dan pencilan

Rerata [minimum–maksimum] dari 30 run sah per kelompok (sumber: `ringkasan.md` Bagian 1):

| Besaran | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| Latensi sign (ms) | 654,88 [654,63–655,03] | 116,23 [116,12–116,37] | 654,91 [654,76–655,36] | 116,30 [116,21–116,46] |
| Tidur CPU (%) | 35,08 [35,07–35,10] | 98,17 [98,15–98,17] | 0,377 [0,374–0,379] | 83,97 [83,90–84,03] |
| Daya dinamis (mW) | 40,10 [40,04–40,16] | 2,96 [2,92–3,00] | 61,18 [61,08–61,25] | 25,70 [25,61–25,77] |
| Energi per layanan (mJ) | 41,03 [40,96–41,09] | 3,01 [2,97–3,05] | 40,59 [40,52–40,65] | 3,00 [2,99–3,01] |
| Arus dinamis (mA) | 7,69 [7,68–7,70] | 0,566 [0,561–0,572] | 11,73 [11,72–11,73] | 4,93 [4,91–4,94] |
| Throughput (sign/s) | 0,977 [0,977–0,978] | 0,983 [0,983–0,983] | 1,507 [1,507–1,508] | 8,565 [8,553–8,572] |
| Tegangan bus (V) | 5,219 [5,214–5,223] | 5,218 [5,206–5,224] | 5,218 [5,213–5,221] | 5,216 [5,204–5,222] |

**Pencilan** (pagar Tukey 3 × IQR; hanya dilaporkan, tidak dibuang): tiga nilai, semuanya pada K3, satu run (blok 2 `021_K1S_B`) pada latensi 655,36 ms (median 654,89) dan satu run (blok 3 `006_K1S_B`) pada arus dinamis 11,733 mA dan muatan 7,787 mC. Penyimpangannya kecil (< 0,1 % dari median pada latensi) sehingga tidak memengaruhi kesimpulan.

### 5.8 Daftar gambar yang tersedia

Berkas di `data/analisis/stop_final/gambar/` (24 gambar), mengikuti `laporan/panduan_grafik_bab4_2026-11-16.md`:

| Jenis | Berkas | Dipakai untuk |
|---|---|---|
| Box plot per kelompok | `box_lat_mean_ms_nyata`, `box_S_cpu_log`, `box_P_dyn_mW`, `box_E_lyn_mJ`, `box_I_dyn_mA`, `box_Q_lyn_mC`, `box_P_avg_mW`, `box_I_avg_mA`, `box_E_mJ`, `box_TL_pct`, `box_Theta_sps` | Hasil latensi, tidur CPU, daya, energi, tingkat layanan, throughput |
| Plot interaksi cara × skenario | `interaksi_lat_mean_ms_nyata`, `interaksi_S_cpu_log`, `interaksi_P_dyn_mW`, `interaksi_E_lyn_mJ`, `interaksi_I_dyn_mA`, `interaksi_TL_pct` | RM3 dan hipotesis interaksi |
| Kestabilan (drift) | `drift_I_avg_mA`, `drift_I_dyn_mA`, `drift_P_dyn_mW`, `drift_S_cpu_log`, `drift_V_avg`, `drift_lat_mean_ms_nyata` | Keandalan data, antar-blok |
| Kesesuaian instrumen | `ba_S_cpu` (Bland-Altman pin vs firmware) | Validasi instrumen |

Gambar tambahan yang belum ada: diagram jalur K2S (waktu terjaga vs tidur dalam satu permintaan), jika diinginkan di subbab perancangan.

---

## 6. Pembahasan Pendukung

Penjelasan bahasa sederhana untuk subbab pembahasan. Bagian yang masih sama prinsipnya dengan Rekap 1 hanya dirujuk.

### 6.1 Mengapa DS28E38 lebih cepat

Software tetap 655 ms karena perhitungan kurva eliptik dikerjakan prosesor 48 MHz (konfigurasi 1c, tabel comb dimatikan; catatan Rekap 1 Bagian 6.1 tentang pilihan konfigurasi tetap berlaku). DS28E38 kini 116 ms, dengan rincian: kirim perintah 3,4 ms (overdrive), chip menghitung ±106,6 ms, baca hasil 5,6 ms. Angka 106,6 ms adalah batas bawah dari chip (105,9–107,3 ms, simpangan baku 0,22 ms), jadi tidak bisa dipercepat lagi dari sisi mikrokontroler. Pada eksperimen pengembangan terpisah (`data/dev/ds_od`), kecepatan biasa dengan tunggu tetap 130 ms memberi 192,5 ms per tanda tangan, sehingga angka 116 ms adalah hasil konfigurasi overdrive dan polling. Latensi tidak berubah antara normal dan *barrage* (selisih < 0,07 %).

### 6.2 Mengapa prosesor lebih banyak tidur pada DS28E38

Pada software prosesor terjaga selama seluruh perhitungan (±650 ms per permintaan). Pada DS28E38 prosesor terjaga hanya sekitar 19 ms per permintaan (menerima frame, mengirim perintah ±3,4 ms, membaca hasil ±5,6 ms, dan memulihkan jam; dihitung dari S_CPU: 1,83 % × 60 s / 59 permintaan), lalu **tidur Stop sambil menunggu chip** (±107 ms). Karena itu pada kondisi normal prosesor tidur 98,2 % waktu (software 35,1 %). Saat *barrage* software praktis tidak pernah tidur (0,38 %), sedangkan DS28E38 tetap tidur 84,0 %.

### 6.3 Persentase tidur tinggi bukan karena DS28E38 malas melayani

Saat *barrage* DS28E38 melayani **514–515 permintaan per menit, sedangkan software hanya 91** (5,7 kali lebih banyak) dan masih tidur 84 % waktu. Pada kondisi normal keduanya melayani 100 %.

### 6.4 Mengapa penghematan daya dinamis lebih kecil saat diserang

Penghematan daya dinamis DS28E38 turun dari 92,6 % (normal) ke 58,0 % (*barrage*) karena DS28E38 mengerjakan 8,7 kali lebih banyak tanda tangan saat *barrage* (daya naik dari 2,96 ke 25,70 mW). Per tanda tangan yang selesai, penghematannya tetap ±92,6 % pada kedua skenario (energi per layanan 3,01 dan 3,00 mJ; selisih keunggulan antar-skenario hanya ±1,1 %). Jadi **energi per layanan** adalah ukuran paling adil untuk membandingkan efisiensi kerja, sedangkan **daya dinamis** menggambarkan beban total selama diserang.

### 6.5 Mengapa tingkat layanan turun saat diserang

Permintaan datang tiap 100 ms. Satu tanda tangan software butuh 655 ms dan DS28E38 116 ms, sehingga permintaan yang tiba selagi sibuk digabung (terlewat). Kapasitas maksimum = 1 / latensi: ±1,53 tanda tangan/detik untuk software dan ±8,60 untuk DS28E38. Throughput terukur menyentuh batas ini (1,507 dan 8,565). Model TL = 1 / (laju permintaan × latensi) memprediksi TL dengan selisih < 0,4 poin, sehingga permintaan terlewat disebabkan lamanya tanda tangan, bukan gangguan jalur komunikasi (frame rusak 0; frame hilang < 0,25 %). Pada beban 10 permintaan/detik DS28E38 hampir (tetapi belum) mampu mengimbangi (rho = 1,17).

### 6.6 Lantai arus dan daya rata-rata total

Daya rata-rata mencakup lantai papan (1,99 mA, 10,4 mW) yang kebanyakan milik LED daya Blue Pill dan komponen papan, bukan prosesor. Lantai ini sama pada K1S dan K2S (selisih < 0,003 mA) dan sama dengan lantai host sendiri (hasil S0, 1,985 mA), sehingga papan penyerang, pencatat, dan sensor tidak menyumbang arus yang terbaca. Board DS9121 menarik sekitar 0,06–0,09 mA saat diam (arus rata-rata turun ke 1,93 mA ketika board dicabut). Daya rata-rata total sudah memuat semuanya; penghematan total 73,6 % (normal) dan 49,6 % (*barrage*).

### 6.7 Arus saat tidur lebih tinggi pada K2S

Arus saat tidur K2S (2,37 mA di normal dan 5,63 mA saat *barrage*) lebih tinggi dari lantai 1,99 mA karena selama Stop menunggu, host menyuplai arus komputasi DS28E38 lewat jalur 1-Wire (strong pull-up). Itu bagian dari biaya hardware offloading dan sudah termasuk dalam daya dan energi yang dilaporkan.

### 6.8 Pemakaian memori

Angka memori pada Rekap 1 Bagian 6.7 diukur pada build tahap D dan tidak diukur ulang untuk firmware Stop. Belum ada angka resmi untuk varian Stop.

### 6.9 Aspek keamanan

Pembahasan Rekap 1 Bagian 6.8 tetap berlaku (kunci rahasia tidak pernah di memori host pada DS28E38; hardware offloading mengurangi dampak serangan, tidak mencegahnya; ketahanan fisik chip tidak diuji). Tambahan: pada varian Stop kunci rahasia software tetap berada di Flash (model ancaman software-based).

---

## 7. Bukti Keandalan dan Keabsahan Data

### 7.1 Keutuhan data

- Target 30 run sah per kelompok tercapai untuk keempat kelompok. Blok 1 (K2S): 35 run, 31 sah; blok 2 (K1S): 32 run, 32 sah; blok 3 (K1S): 32 run, 32 sah; blok 4 (K2S): 40 run, 33 sah. Total 139 run di sesi resmi (termasuk 8 run IDLE), 11 gugur, semuanya K2S *barrage* yang kehilangan 2 frame atau lebih (kriteria toleransi). Run gugur tidak dianalisis.
- **Kriteria khusus varian Stop** (keputusan peneliti, berlaku di `run_bench.py` dan `analisis_d.py --stop`): frame rusak harus 0; pada *barrage* varian Stop frame hilang diperbolehkan jika **kurang dari 0,25 % dari yang tiba per run**. Total frame hilang pada run sah: 13. Frame rusak: **0**.
- Tanda tangan gagal di sisi host (N_ggl): **0** pada seluruh run sah. Kesalahan komunikasi sensor arus: 0. Selisih terbesar persentase tidur menurut logger dan firmware: 0,08 poin (batas 0,5).
- Hasil pemeriksaan otomatis: **MEMENUHI STANDAR**.

### 7.2 Kesesuaian dua cara pengukuran

| Pasangan yang dibandingkan | Selisih rata-rata | Arti |
|---|---|---|
| Persentase tidur: pin (pencatat) vs firmware, 120 run | −0,014 poin (batas 95 %: −0,055 s.d. +0,026) | dua cara pengukuran hampir identik |
| Latensi K2S (RTC): pin vs firmware, 1.770 tanda tangan | +3,28 ms (simpangan baku 0,26 ms) | selisih tetap; pin mencakup pemulihan jam setelah bangun dan kerja di luar jendela ukur |
| Latensi K1S: pin vs penghitung siklus, 1.770 tanda tangan | +9,29 ms (simpangan baku 0,06 ms) | selisih tetap; pin juga mencakup pengumpulan sumber acak dan pemulihan jam |
| Model arus (arus aktif × proporsi aktif + arus tidur × proporsi tidur) vs arus terukur | maksimal 0,008 mA | data arus konsisten dengan sinyal pin |

Catatan: frekuensi inti host (±48 MHz) diukur terhadap RTC selama 2 detik setelah tiap jendela, dan dipakai untuk mengubah siklus menjadi milidetik nyata pada K1S. Penghitung siklus prosesor berhenti saat Stop, sehingga lama tidur dan latensi K2S dihitung dari RTC (LSE) dan dikoreksi dengan jam kristal pencatat. Galat jam RTC terhadap kristal pencatat: −64 s.d. −31 ppm (stabil), dikoreksi per run.

### 7.3 Kestabilan selama pengambilan data

| Blok | I_IDLE awal (mA) | I_IDLE akhir (mA) | Selisih (mA) | Status |
|---|---|---|---|---|
| 1 (K2S) | 1,990 | 1,990 | +0,001 | OK |
| 2 (K1S) | 1,990 | 1,992 | +0,002 | OK |
| 3 (K1S) | 1,992 | 1,991 | −0,001 | OK |
| 4 (K2S) | 1,989 | 1,984 | −0,004 | OK |

- Pergeseran arus diam terbesar 0,004 mA (batas 0,05 mA). Tegangan bus 5,20–5,23 V.
- **Uji tanpa satu blok (*leave-one-block-out*):** penghematan daya dinamis tetap 92,60–92,64 % (normal) dan 57,95–58,02 % (*barrage*); latensi 82,24–82,25 %; selisih tidur CPU 63,08–63,09 (normal) dan 83,57–83,61 poin (*barrage*). Kesimpulan tidak bergantung pada satu blok.
- Run sah dengan "late ringan" (≤ 10 cuplikan terlambat, selang < 5.000 µs): 62.

### 7.4 Ketidakpastian pengukuran

| Besaran | Ketidakpastian gabungan (relatif) | Komponen dominan |
|---|---|---|
| Latensi | 0,01–0,04 % | sangat kecil |
| Persentase tidur CPU | ≤ 0,03 % | sangat kecil |
| Daya dinamis, energi per layanan | 5,0–5,1 % | asumsi ketidakpastian kalibrasi shunt 5 % |
| Arus dinamis, muatan per layanan | 5,0–5,1 % | asumsi ketidakpastian kalibrasi shunt 5 % |

Ketidakpastian kalibrasi bekerja sebagai faktor pengali yang sama untuk semua kelompok sehingga saling meniadakan pada persentase penghematan. Catatan Rekap 1 Bagian 7.5 tentang nilai p yang sangat kecil karena sebaran antar-run sangat kecil tetap berlaku; kesimpulan ditekankan pada besar selisih, persentase, dan interval kepercayaan.

### 7.5 Prosedur keadaan bersih (keabsahan pengukuran Stop)

Pada varian Stop, **satu sesi OpenOCD saja** menaikkan lantai arus dari 1,94 ke 2,52 mA dan melonjakkan bangun palsu (99 dari 100 frame di satu percobaan) sampai POR penuh berikutnya. Maka seluruh data final diambil dengan prosedur: flash lewat ST-Link → **POR penuh** (cabut USB-C, ST-Link, semua kabel sinyal dan GND; tunggu 10 detik; pasang GND dulu; USB-C terakhir) → pengukuran **tanpa membuka OpenOCD sama sekali**. Konsistensi dibuktikan oleh IDLE di awal dan akhir tiap blok (1,984–1,992 mA).

### 7.6 Ketertelusuran firmware

Kode di repositori yang menghasilkan data final dibekukan di *commit* `313aada` ("Stop: cek idle ketat, penerima dini di jalur alarm, lantai arus terpecahkan"); data final di-*commit* di `518f54c` dan `43e701a`. Build yang dipakai: env `host_k1_stop` dan `host_k2_stop` (sha256 awal 3e9c7595becbb54c… dan 5d32d74195c1725d… pada build akhir; kode sumber tidak diubah selama pengambilan data). Firmware pencatat dan penyerang tidak diubah sejak tahap D (build pencatat 4C364231 tercatat di semua sesi).

### 7.7 Uji ketahanan dan jalur galat

Tidak ada *crash* atau macet pada 139 run final, 24 run pilot, dan seluruh uji galat; ganti firmware otomatis 9 kali pada pilot. Uji jalur galat (K2S, `tools/uji_galat_stop.py` dan uji tangan):

| Uji | Hasil |
|---|---|
| G1 penyerang berhenti total di detik 25 | LULUS: jendela 60,00 s, S_cpu 93,38 % = 93,41 %, rusak 0, run pulih 59/59 |
| G2 penyerang dimatikan lalu dinyalakan ulang | LULUS: 591 tiba, 506 dilayani, rusak 0, hilang 0, pulih 59/59 |
| G3 penyerang hidup/mati tiap 1,5 detik | LULUS: rusak 0, bangun palsu 0, S_cpu 88,02 % = 88,06 %, pulih 59/59 |
| G4 board DS9121 dicabut saat run berjalan (80 s) | LULUS: jendela 80,00 s, S_cpu 99,356 % = 99,355 %, 12 tanda tangan sukses lalu **67 gagal tercatat**, tanpa *crash*, host tetap tidur |
| G5 DS9121 dipasang kembali tanpa reboot | LULUS: 59/59, semua pemeriksaan lolos |
| G6 DS9121 tidak ada saat boot | LULUS: host melapor "siap = 0" dan `run_bench` **menolak** merekam data |
| G7 DS9121 dipasang, reboot | LULUS: siap = 1, run 59/59 |

K1S tidak diuji untuk galat (jalur Stop identik, tanpa DS28E38). Galat kristal LSE dan tegangan turun tidak diuji.

---

## 8. Perubahan terhadap Rencana Awal (Proposal) dan terhadap Rekap 1

Perubahan Rekap 1 Bagian 8 tetap berlaku. Tambahan akibat varian *deep sleep*:

| Aspek | Cara pada pengembangan awal (Rekap 1) | Cara untuk data skripsi (*deep sleep*) | Alasan / dampak |
|---|---|---|---|
| Data utama | Tahap D, mode Sleep/WFI | **Varian Stop (*deep sleep*)** | Lebih relevan dengan perangkat IoT bertenaga baterai. Data awal pengembangan dipertahankan tanpa diubah |
| Mode tidur host | WFI, jam 48 MHz tetap hidup | Stop: jam utama mati, alarm RTC/LSE, PLL dipulihkan saat bangun | Arus diam mendekati lantai papan (1,99 mA) |
| Implementasi K2 | Kecepatan biasa, tunggu tetap 130 ms | Overdrive 1-Wire + polling (116 ms) | Permintaan peneliti untuk meminimalkan latensi tanda tangan |
| Pembangunan penerima UART | DMA terus hidup | UART mati di Stop; bangun lewat sinkron bit byte pertama (0xA5), penyerang tidak diubah | Syarat dasar Stop; 0,1–0,2 % frame hilang pada *barrage* K2S (lihat Bagian 7.1) |
| Sumber waktu | Penghitung siklus, dikoreksi jam pencatat | RTC/LSE (lama tidur dan jendela), dikoreksi jam pencatat | Penghitung siklus berhenti saat Stop |
| Prosedur pengukuran | Cabut ST-Link; POR bila perlu | **POR penuh + tanpa OpenOCD** (keadaan bersih) | Debug power mengubah arus dan perilaku bangun |
| Kriteria keabsahan | Frame rusak = hilang = 0 | Rusak = 0; hilang < 0,25 % per run pada *barrage* | Keputusan peneliti (sisa frame hilang saat pemulihan dari alarm) |
| Urutan blok | K1, K2, K1, K2 | K2S, K1S, K1S, K2S (ABBA dimulai dari K2S) | Memakai host yang sudah bersih dan berisi K2S |
| Jumlah run per blok | acak | blok K1S: 15 A + 15 B; blok K2S: 15 A + 18/23 B (cadangan untuk run gugur) | K2S *barrage* sering melewati toleransi |
| Landasan instruksi | — | 24 NOP setelah perintah tidur | Instruksi pertama sesudah bangun kadang salah dieksekusi |
| Papan penyerang/pencatat | — | sama | Tidak diubah |
| Port serial penyerang | COM16 | COM22 → COM24 | Kabel USB penyerang goyah; cek port sebelum sesi |

---

## 9. Keterbatasan dan Hal yang Belum Tuntas

1. **Latensi dan tingkat layanan K2 mencerminkan konfigurasi overdrive + polling**, bukan kecepatan biasa (eksperimen terpisah: 192,5 ms pada kecepatan biasa dengan tunggu tetap, 115,6 ms pada overdrive + polling). Kontribusi masing-masing (overdrive, polling, mode Stop) tidak dipisahkan dalam data final.
2. **Daya dan energi "dinamis"** dihitung terhadap daya IDLE varian Stop (10,4 mW); hanya sah untuk membandingkan software dan DS28E38 di dalam data ini.
3. **Frame UART hilang 0,1–0,2 % pada K2S *barrage*** (frame yang mulai persis saat host pulih dari alarm); dibatasi < 0,25 % per run.
4. **Tingkat layanan K4 (85,5 %)** ditentukan oleh latensi 116 ms (kapasitas 8,6 tanda tangan/detik) dan beban 10 permintaan/detik; dengan tanda tangan lebih lambat kapasitas dan tingkat layanan lebih rendah.
5. **Kalibrasi shunt INA226 belum selesai** (butir 1 Rekap 1 tetap berlaku). Nilai mutlak mA/mW/mJ perlu dihitung ulang setelah kalibrasi; persentase tidak terpengaruh. Pembanding multimeter belum dilakukan.
6. **Satu papan host dan satu chip DS28E38** (butir 3 Rekap 1 tetap).
7. **Konfigurasi software memengaruhi besar keunggulan hardware** (butir 4 Rekap 1 tetap): konfigurasi 1c (tabel comb mati).
8. **Arti "latensi"** tetap lama pembuatan satu tanda tangan, bukan waktu tunggu antrean. Pada varian Stop, latensi K2S diukur dari RTC dan pin selisih ±3,3 ms (butir 5 Rekap 1 diperbarui).
9. **Lantai arus (1,99 mA) didominasi LED daya papan**, dipertahankan atas keputusan peneliti. Nilai mutlak daya rata-rata mencerminkan papan ini, bukan chip telanjang (1–20 µA tingkat chip).
10. **Jam 48 MHz, bukan 72 MHz**, sehingga latensi software absolut tidak dapat disamakan dengan penelitian lain.
11. **Landasan NOP** adalah obat empiris; akar penyebab di tingkat silikon tidak diselidiki lebih jauh. Layak dicatat sebagai catatan implementasi.
12. **Uji galat hanya pada K2S**, dan tidak mencakup galat LSE atau tegangan turun.
13. **Ketahanan fisik DS28E38 tidak diuji** (butir 11 Rekap 1 tetap).
14. **Keabsahan kriptografis tanda tangan pada run final tidak diperiksa per run.** Firmware hanya melaporkan keberhasilan (N_ggl = 0); tanda tangan tidak dikirim ke komputer. Keabsahan tanda tangan DS28E38 mode overdrive diperiksa di komputer pada eksperimen pengembangan (269 dari 269 valid, termasuk uji tanda tangan sengaja dirusak), dan blok kripto K1S adalah salinan harfiah blok K1 yang sudah diverifikasi pada pengembangan awal.
15. **Gambar varian Stop** sudah dibuat di `data/analisis/stop_final/gambar/` tetapi belum ditinjau peneliti.

---

## 10. Bahan Penyusunan Kesimpulan dan Saran (Revisi)

Pengganti Bagian 10 Rekap 1. Angka energi mutlak perlu diperbarui setelah kalibrasi shunt.

### 10.1 Rancangan kesimpulan

1. **Jawaban RM1 (latensi dan energi per permintaan).** Pada varian *deep sleep*, pembuatan tanda tangan ECDSA P-256 dengan DS28E38 (overdrive + polling) membutuhkan sekitar 116 ms, sedangkan software Mbed TLS (konfigurasi 1c) sekitar 655 ms, sehingga DS28E38 82,3 % lebih cepat (5,6 kali). Energi tambahan per tanda tangan yang selesai sekitar 3,0 mJ (normal dan *barrage*) untuk DS28E38 dibandingkan sekitar 41 mJ untuk software, penghematan sekitar 92,6 %.
2. **Jawaban RM2 (pengaruh serangan).** *Barrage* membuat prosesor software hampir tidak pernah tidur (0,38 %) dan hanya 15,0 % permintaan terlayani. Pada DS28E38 prosesor masih tidur 84,0 % dan 85,5 % permintaan terlayani. Latensi tiap tanda tangan tidak berubah oleh serangan.
3. **Jawaban RM3 (selisih dan interaksi).** Keunggulan DS28E38 dalam persentase tidur CPU naik saat diserang (dari +63,1 menjadi +83,6 poin; interaksi +20,5 poin). Keunggulan daya dinamisnya mengecil (92,6 % menjadi 58,0 %) karena DS28E38 mengerjakan 8,7 kali lebih banyak tanda tangan; per pekerjaan yang selesai keunggulannya tetap ±92,6 %.
4. **Hipotesis.** H1a, H1b, H1c, hipotesis interaksi, dan H4 seluruhnya didukung data *deep sleep*.
5. **Lantai papan.** Pada varian *deep sleep* arus diam host sama dengan lantai papan sendiri (1,99 mA), sehingga selisih yang diamati berasal dari prosesor dan chip, bukan dari papan penyerang, pencatat, atau sensor.
6. **Kesimpulan umum.** Pengalihan pembuatan tanda tangan ke DS28E38 membuat perangkat lebih cepat, jauh lebih hemat energi per pekerjaan, lebih banyak tidur, dan melayani jauh lebih banyak permintaan saat diserang, sehingga dampak serangan *Denial of Sleep* berkurang walaupun serangannya tidak tercegah.

### 10.2 Rancangan saran

1. Memisahkan kontribusi overdrive dan polling terhadap latensi DS28E38 dengan satu kelompok tambahan (mis. K2 kecepatan biasa pada Stop).
2. Kalibrasi shunt dengan acuan presisi dan verifikasi dengan multimeter, lalu hitung ulang nilai mutlak.
3. Menguji beberapa papan dan beberapa chip DS28E38 untuk variasi antar-unit.
4. Membandingkan beberapa konfigurasi software (tabel comb aktif, pustaka lain seperti micro-ecc).
5. Menambahkan penyaringan atau pembatasan laju permintaan agar serangan dapat dicegah.
6. Menguji Standby, pemakaian baterai sungguhan, dan papan tanpa LED daya agar lantai mendekati tingkat chip.
7. Mempertimbangkan antrean permintaan dan mengukur waktu tunggu.
8. Memperbaiki penerimaan frame saat pulih dari alarm (mis. penerima pembangun terpisah) agar toleransi frame hilang tidak diperlukan.

---

## 11. Lampiran

### Lampiran A. Kronologi ringkas (lanjutan)

| Tanggal | Kegiatan utama | Status akhir hari |
|---|---|---|
| 17 Nov | Rencana *deep sleep*; uji LSE (25 uji); lantai Stop 1,985 mA; S1 (9–20 µs); S2 (59/59, 601/601); resistor seri; prosedur POR bersih | Langkah 1–7 lulus |
| 18 Nov dini–pagi | Overdrive DS28E38 (192,5 → 115,6 ms); firmware K1S/K2S; papan host terkunci | Overdrive lulus; integrasi belum lulus |
| 18 Nov siang | Pemulihan BOOT0; penyebab *crash* (landasan NOP); K1S/K2S lulus; perbaikan jendela dan batas waktu | Integrasi lulus |
| 18 Nov sore | Pilot (0 *crash*); lantai arus 1,94 vs 2,52 mA (*debug power*) | Prosedur keadaan bersih ditetapkan |
| 18 Nov malam | Perbaikan *barrage* K2S; **data final 4 × 30 + IDLE**; uji galat 7/7; analisis; ringkasan Bab 4 | Data final memenuhi standar; **perubahan data ke *deep sleep* diputuskan** |

### Lampiran B. Kaitan bagian dokumen ini dengan Bab 4 (menggantikan Lampiran B Rekap 1)

| Subbab Bab 4 (usulan) | Bahan di dokumen ini | Gambar/tabel terkait |
|---|---|---|
| Perancangan varian *deep sleep* dan sistem uji | Bagian 3.5, 4.1–4.3, 8, Lampiran D | — |
| Validasi instrumen, prosedur keadaan bersih, keutuhan data | Bagian 7 | Tabel D (sebaran run per blok), Tabel D1 |
| Hasil latensi (RM1, H1a) | Bagian 5.1–5.2, 6.1 | Gambar 4.2 |
| Hasil persentase tidur CPU (H1c) | Bagian 5.1–5.2, 6.2–6.3 | Gambar 4.3 |
| Hasil daya dan energi (RM1, H1b) | Bagian 5.1–5.2, 6.4, 6.6–6.7 | Gambar 4.1, 4.4, 4.5, Tabel C |
| Pengaruh serangan (RM2) | Bagian 5.3 | Tabel B |
| Interaksi (RM3) | Bagian 5.4 | Gambar 4.6 |
| Tingkat layanan dan throughput (H4) | Bagian 5.5, 6.5 | Gambar 4.7, 4.8 |
| Uji hipotesis | Bagian 5.2, 5.6 | Tabel A |
| Sebaran, pencilan, dan daftar gambar | Bagian 5.7–5.8 | seluruh gambar di `data/analisis/stop_final/gambar/` |
| Ketahanan dan jalur galat | Bagian 7.7 | — |
| Keterbatasan penelitian | Bagian 9 | — |

### Lampiran C. Lokasi data dan berkas di repositori

| Isi | Lokasi |
|---|---|
| Logbook harian | `logbook/2026-11-17.md`, `logbook/2026-11-18.md` (dan 10–16 Nov untuk riwayat) |
| Rencana *deep sleep* dan status langkah | `docs/rencana_deep_sleep.md` |
| Ringkasan hasil untuk Bab 4 | `docs/ringkasan_bab4_varian_stop.md` |
| **Data mentah *deep sleep* final** | `data/runs/20261118_115044_K2S`, `20261118_124646_K1S`, `20261118_132320_K1S`, `20261118_140335_K2S` (manifest `data/tahap_stop.txt`) |
| **Analisis *deep sleep* final** | `data/analisis/stop_final/` (`ringkasan.md`, `pemeriksaan.md`, `runs.csv`, `deskriptif.csv`, `banding_sw_hw.csv`, `pengaruh_barrage.csv`, `interaksi_dd.csv`, `layanan.csv`, `ketidakpastian.csv`, `bland_altman.csv`, `leave_one_block_out.csv`, `latensi_pin_vs_dwt.csv`, `model_duty_per_run.csv`, `gambar/`) |
| Data awal pengembangan (tahap D) | `data/runs/<sesi>/` (manifest `data/tahap_d.txt`), analisis `data/analisis/20261116_012530_r0.1/` |
| Data pilot *deep sleep* | `data/analisis/pilot_stop/` (keluaran sebagian), `data/runs/20261118_094801_K1K2`, `…_093427_K2S`, `…_092654_K2S`, `…_092803_K2S`, `data/tahap_stop_pilot.txt` |
| Data uji dasar *deep sleep* | `data/dev/s0a/ringkasan.csv`, `data/dev/s0/`, `data/dev/s1/`, `data/dev/s2/` |
| Data overdrive DS28E38 | `data/dev/ds_od/` |
| Uji jalur galat | `data/dev/err/` (termasuk `stop_galat_ds/`), skrip `tools/uji_galat_stop.py` |
| Run dan sesi gugur | `data/dev/gugur/` |
| Skrip kendali dan analisis | `tools/run_bench.py`, `tools/analisis_d.py` (opsi `--stop`), `tools/k1s_run.py`, `tools/s2_rx.py`, `tools/flash_stop.py`, `tools/pulihkan_host.py`, `tools/ds_od.py` |
| Firmware Stop | `src/host/deepsleep/` (`stop_common.h`, `stop_window.h`, `stop_link.h`, `k1_stop/`, `k2_stop/`, `ds28e38_od.h`, `s2_rx/`), `src/host/eksperimen/ds_overdrive/` |
| Laporan rekap sebelumnya | `laporan/laporan_rekap_logbook_pendukung_bab4_2026-11-17.md` |
| Revisi Bab 3 (v4 ke v5) dan panduan grafik Bab 4 | `laporan/revisi_bab3_v4_2026-11-16.md`, `laporan/panduan_grafik_bab4_2026-11-16.md` |

### Lampiran D. Data pendukung pengembangan *deep sleep*

**Tabel D1. Uji dasar Stop (17 November)**

| Uji | Hasil |
|---|---|
| S0a: kristal LSE, 25 uji | Hidup di 23/23 uji sah; start dingin 3,5–4,1 s (rerata 3,79 s); −47 ppm stabil (≤ 0,2 ppm antar-uji) |
| S0: lantai Stop (POR bersih, tanpa ST-Link) | 1,985 mA (14.922 cuplikan); dengan ST-Link 2,567 mA |
| S1: alarm RTC, jendela 28,5 s, 5 jendela | selisih lama tidur host vs pencatat 9–20 µs; bangun palsu 0; arus tidur 1,988 mA |
| S2: bangun karena permintaan penyerang | 1 Hz: 59/59; 10 Hz: 601/601 (ST-Link menempel, 17 Nov) dan 602/602 (setelah pemulihan, 18 Nov); rusak 0, hilang 0, bangun palsu 0 |

**Tabel D2. Overdrive DS28E38 (18 November)** — lihat Bagian 4.2 butir 1 (192,5 / 169,6 / 138,7 / **115,6 ms**; 269/269 valid; chip 105,9–107,3 ms).

**Tabel D3. Hasil integrasi dan penyempurnaan (18 November)**

| Tahap | K2S | K1S |
|---|---|---|
| Integrasi awal (ST-Link, A) | 59/59, 116,2 ms, S_cpu 98,19 % | 59/59, 683 ms, S_cpu 32,39 % |
| Tanpa ST-Link, A | 59/59, 116,2 ms | — |
| *Barrage* sebelum perbaikan | 515/600 dilayani; rusak 1 dan hilang 1 (±60 % run) | jenuh seperti baseline |
| *Barrage* sesudah cek idle ketat + penerima dini (keadaan bersih) | rusak 0, bangun palsu 0, hilang rata-rata ±0,9 per 600 | tetap jenuh |
| Lantai IDLE bersih (POR penuh) | 1,988 mA | 1,991 mA |

**Tabel D4. Run gugur data final (semua K2S *barrage*, hilang ≥ 2 frame)**

| Blok | Run gugur | Frame hilang |
|---|---|---|
| 1 | 006, 008, 019, 034 | 2, 2, 2, 3 |
| 4 | 013, 020, 022, 024, 025, 032, 035 | 4, 2, 2, 3, 4, 2, 3 |

**Tabel D5. Prosedur ringkas pengambilan data final (untuk Bab 3/4)**

1. Bangun firmware (`pio run -e host_k1_stop` atau `host_k2_stop`).
2. Flash lewat ST-Link (`tools/flash_stop.py`, dibantu penyerang 10 permintaan/detik agar host sering bangun).
3. **POR penuh** (Bagian 7.5).
4. Sesi `run_bench.py plan A:15,B:<n> --secs 60` (IDLE otomatis di awal dan akhir sesi, urutan acak).
5. Tanpa menyentuh OpenOCD sampai sesi selesai; ulangi dari langkah 1 untuk blok berikutnya.
6. Analisis: `python tools/analisis_d.py --manifest data/tahap_stop.txt --stop`.

### Lampiran E. Ringkasan rancangan firmware Stop (bahan Bab 3)

1. **Satu jendela 60 detik** dikendalikan alarm RTC. Host melayani permintaan, lalu masuk Stop di antara kejadian.
2. **Masuk Stop:** penerima UART dan DMA dimatikan; SysTick dimatikan; PB1 diturunkan (tanda tidur untuk pencatat); event EXTI3 (tepi turun PA3) dan event alarm RTC (EXTI17) diizinkan; perintah SEV + WFE + WFE + 24 NOP.
3. **Bangun:** inti kembali pada HSI 8 MHz dengan PLL mati. Jam periferal diset ulang, PB1 dinaikkan, PLL dinyalakan.
4. **Bangun karena permintaan penyerang:** penerima dinyalakan lewat sinkron bit (mengenali pola tepi byte 0xA5 ±17 µs sebelum byte kedua) pada HSI; frame 36 byte diterima lewat DMA; PLL baru dipindah sesudah frame lengkap; frame divalidasi (penanda awal dan XOR) lalu dicatat.
5. **Bangun karena alarm:** bila garis PA3 diam, penerima langsung dinyalakan pada HSI (agar frame yang kebetulan mulai tidak hilang), baru pindah ke PLL setelah garis diam ketat 120 µs.
6. **Cek garis diam ketat sebelum tidur:** PA3 harus tinggi terus 120 µs dan penghitung DMA belum berkurang.
7. **Satu alarm dimultipleks** untuk dua tujuan: akhir jendela dan (K2S) selesainya chip menghitung (±107 ms, ditunggu dalam Stop).
8. **Waktu:** lama tidur dan lama jendela dari RTC (resolusi 30,5 µs); frekuensi inti diukur terhadap RTC setelah tiap jendela.
9. **Keselamatan:** jendela aman 5 detik setelah boot sebelum Stop pertama; port debug tidak dimatikan; pemulihan papan terkunci lewat BOOT0 (`tools/pulihkan_host.py`).
10. **K2S:** overdrive 1-Wire dengan polling mulai ±100 ms; hasil dibaca dalam ±5,6 ms.
