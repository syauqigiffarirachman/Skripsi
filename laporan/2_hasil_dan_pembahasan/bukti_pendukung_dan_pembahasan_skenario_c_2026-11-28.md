# Bukti pendukung dan bahan pembahasan: skenario C (kapasitas), serta daftar gambar yang diubah dan ditambah

Dibuat 28 November 2026. Melengkapi `bukti_pendukung_B1_B2_C5_2026-11-18.md` dan `bahan_pembahasan_bab4_2026-11-18.md` dengan data 27 November (K5 dan K6). Rincian angka lengkap dan draf kalimat naskah ada di `laporan/3_revisi_naskah/perubahan_skripsi_v5_skenario_c_2026-11-27.md`; dokumen ini merangkum bukti, pembahasan, dan **daftar gambar**.

**Catatan penamaan.** Dokumen lama menyebut Bab 4 sebagai "Hasil dan Pembahasan". Di naskah lengkap (v4), hasil dan pembahasan ada di **Bab 5**, dan gambarnya bernomor 5.x. Bab 4 naskah berisi implementasi dan tidak memuat grafik data. Dokumen ini memakai nomor naskah (5.x).

**Tegangan acuan.** Daya dan energi K5/K6 dinyatakan pada tegangan pengukuran 18 November (5,22 V), sama dengan K1–K4 (lihat bagian 1.3). Besaran lain tidak bergantung pada tegangan.

---

## 0. Jawaban singkat: apakah gambar sudah lengkap?

**Untuk grafik data Bab 5: ya, sudah cukup**, dengan dua catatan di bawah. Delapan gambar sudah dibuat (6 diperbarui, 2 baru) di `data/analisis/kapasitas/gambar/`.

**Yang belum selesai di luar delapan gambar itu:**

| Butir | Status | Siapa |
|---|---|---|
| Diagram garis waktu skenario C untuk Bab 3 (usulan Gambar 3.6, bagian 4.3) | belum ada | digambar manual |
| Gambar ulang diagram lama 2.1, 3.1, 3.2, 3.4 (butir M7 di v3) | status tidak bisa saya periksa | peneliti |
| Gambar jejak arus K5/K6 (opsional, bagian 4.4) | belum dibuat | bisa saya buat |
| Gambar Bland-Altman dan dekomposisi energi untuk K5/K6 (opsional) | belum dibuat | bisa saya buat |
| Penyalinan gambar ke `gambar_naskah/` dengan nomor naskah | belum | bisa saya kerjakan |

**Batas pemeriksaan saya.** Saya tidak bisa membuka PDF naskah dan melihat gambar yang sudah tercetak. Daftar "yang tidak berubah" didasarkan pada isi data dan skrip pembuat gambar, bukan pada pemeriksaan visual halaman naskah.

---

## 1. Bukti pendukung

### 1.1 Data yang diambil dan keabsahannya

| Pemeriksaan | Hasil | Berkas bukti |
|---|---|---|
| Biner host sama dengan 18 Nov | sha256 `2ca6e8d0d861…` (software), `5b4e017827e3…` (DS28E38) | `tools/4_verifikasi/biner_acuan.json` |
| Biner attacker baru, mode A/B tidak berubah | `17665901c2fa…`; uji regresi: A 59/59, B TL 85,55 % | logbook 27 Nov |
| Jumlah run sah | K5 = 30, K6 = 31; dua run K6 gugur (hilang 6 frame, 1,2 %) dan diganti tiga run tambahan | `data/analisis/kapasitas/runs.csv` |
| Frame rusak | 0 pada semua run K5/K6 | `ringkasan.md` |
| Frame tiba + hilang = jumlah dikirim attacker | cocok (516–517 pada K6) | logbook 27 Nov, bagian 4.2 |
| Jembatan ke 18 Nov (uji pendahuluan) | latensi −0,12 % (software), −0,07 % (DS28E38) | v5, bagian 1 |
| Periode attacker sesuai kapasitas | rho nyata K5 0,985; K6 0,998 | `ringkasan.md` |
| Run diambil tanpa debugger terpasang | ST-Link dicabut, POR penuh sebelum ukur | logbook 27 Nov |

**Batas frame hilang.** Batas yang dipakai di seluruh dokumen ini adalah 1 % dari permintaan yang tiba. Pada K6, periode serangan hampir sama dengan siklus host, sehingga frame berulang jatuh di celah saat prosesor baru pulih dari tidur (mekanisme sama dengan K4 pada 18 Nov, hanya lebih sering). Batas 1 % sudah diuji pada K4 (ambang 1,0 %, 41 run: semua besaran bergeser < 0,15 % dan keputusan uji sama; `bahan_pembahasan_bab4_2026-11-18.md` bagian 1) dan pada K6 (bagian 1.5). Analisis utama K1–K4 (18 Nov) tidak diubah; analisis utamanya tetap memakai 30 run K4.

### 1.2 Hasil utama (rerata run sah)

| Besaran | K1 SW A | K2 HW A | K5 SW C | K6 HW C | K3 SW B | K4 HW B |
|---|---|---|---|---|---|---|
| Laju serangan (permintaan/s) | 0,98 | 0,98 | 1,51 | 8,59 | 10,02 | 10,02 |
| Latensi satu tanda tangan (ms) | 654,9 | 116,2 | 654,1 | 116,3 | 654,9 | 116,3 |
| Tingkat layanan (%) | 100 | 100 | 98,35 | 99,61 | 15,04 | 85,50 |
| Prosesor tidur (% jendela) | 35,08 | 98,17 | 2,22 | 84,47 | 0,38 | 83,97 |
| Arus rata-rata (mA) | 11,82 | 3,14 | 16,39 | 8,37 | 16,73 | 8,45 |
| Throughput (tanda tangan/s) | 0,977 | 0,983 | 1,481 | 8,555 | 1,507 | 8,565 |

Besaran yang tidak bergantung tegangan (latensi, layanan, tidur, arus, throughput) dipakai apa adanya.

### 1.3 Daya dan energi pada tegangan 18 Nov

K5/K6 dihitung ulang dengan tegangan rerata K1–K4 (5,218 V software; 5,217 V DS28E38) sebagai ganti 5,13 V. Rumus: daya rata-rata dikali rasio tegangan; daya dinamis = daya rata-rata − daya diam IDLE 18 Nov; energi per permintaan = daya dinamis × lama jendela ÷ jumlah dilayani. Rumus ini dicek terhadap kolom lama (selisih relatif < 10⁻⁶ sebelum koreksi).

| Besaran | Tegangan asli | Dipakai (tegangan 18 Nov) | K1 | K2 | K3 | K4 |
|---|---|---|---|---|---|---|
| Daya dinamis K5 / K6 (mW) | 71,28 / 30,14 | **72,75 / 30,89** | 48,85 | 3,60 | 74,52 | 31,30 |
| Energi per permintaan K5 / K6 (mJ) | 48,13 / 3,52 | **49,12 / 3,61** | 49,98 | 3,67 | 49,44 | 3,65 |

Dengan koreksi ini K5 dan K6 jatuh sangat dekat dengan K3 dan K4. Itu konsisten dengan temuan bahwa **energi per permintaan hanya ditentukan arsitektur, bukan beban**.

> Angka ini berbeda dari `data/analisis/kapasitas/ringkasan.md` (yang memakai tegangan terukur 5,13 V). Naskah memakai angka kolom "Dipakai".

### 1.4 Uji statistik (K5 lawan K6, hari yang sama)

Uji Welch dengan koreksi Holm atas enam besaran utama, selisih diberi selang kepercayaan 95 % bootstrap. Seluruh enam besaran berbeda nyata (p Holm < 0,001), dengan arah: latensi −537,8 ms, tingkat layanan +1,27 poin, tidur prosesor +82,3 poin, arus −8,03 mA. Karena sebagian besar selisih ini adalah beda arsitektur yang sudah diketahui, nilai pentingnya bukan p, melainkan **besar selisih**.

**Perbandingan K3–K5 dan K4–K6 (lintas hari) hanya deskriptif.** Hampir semua uji menolak H0, tetapi selisihnya kecil dan sebagian besar berasal dari efek hari dan jeda awal attacker. Contoh: selisih latensi K3–K5 (−0,81 ms) hampir sama dengan selisih jembatan antar-hari (−0,78 ms). Jangan menarik kesimpulan dari nilai p lintas hari.

### 1.5 Sensitivitas aturan frame hilang (K6)

| Aturan | Run sah | Arus (mA) | Energi/tanda tangan (mJ) | Tidur (%) | Layanan (%) | Latensi (ms) |
|---|---|---|---|---|---|---|
| hilang ≤ 1 frame (setara aturan A/B) | 24 | 8,374 | 5,0145 | 84,44 | 99,613 | 116,24 |
| hilang < 1 % (dipakai) | 31 | 8,369 | 5,0148 | 84,47 | 99,612 | 116,31 |

Pergeseran antar-aturan: arus 0,07 %, tidur 0,03 poin, latensi 0,06 ms. **Aturan yang lebih longgar tidak mengubah kesimpulan.**

### 1.6 Temuan metode: jeda awal attacker

Attacker mengirim frame pertama sekitar dua periode ditambah 20 ms setelah perintah mulai. Rata-rata jendela 60 detik jadi memuat waktu menganggur di awal (A 2,02 s; B 0,22 s; C software 1,33 s; C DS28E38 0,26 s). Setelah dibuang (keadaan "tunak"), tidur prosesor software saat diserang menjadi **0,00 %** (jendela 0,38 % pada B, 2,22 % pada C). Efek ini ada juga pada data 18 Nov dan harus diungkap. Persentase penghematan energi per permintaan tidak terpengaruh.

---

## 2. Bahan pembahasan

**P1. Kapasitas tercapai tepat pada 1/T_sign.** Throughput pada skenario C (software 1,515/s; DS28E38 8,593/s dalam keadaan tunak) sama dengan throughput saat barrage. Menaikkan laju melebihi kapasitas hanya menambah permintaan yang dibuang (software 85 %, DS28E38 14,5 %), tanpa menambah tanda tangan.

**P2. Software kehilangan seluruh waktu tidurnya pada laju kapasitas.** Pada 1,5 permintaan per detik, prosesor software praktis tidak pernah tidur (2,22 % per jendela; 0,00 % tunak). Arusnya sama dengan barrage hari yang sama (16,71 vs 16,70 mA). Penyerang hanya butuh sekitar 15 % dari frame barrage untuk menimbulkan kerusakan penuh. Ini menjawab revisi penguji: titik serangan paling efisien adalah kapasitas host itu sendiri.

**P3. DS28E38 mempertahankan lantai tidur sekitar 84 %.** Bahkan pada 8,6 permintaan per detik, prosesor masih tidur 84,5 %. Penyebabnya, prosesor menunggu chip menghitung tanda tangan sambil tidur. Penurunan tidur dari kondisi normal ditentukan oleh jumlah tanda tangan yang dikerjakan, bukan oleh laju frame yang datang.

**P4. Keunggulan energi per permintaan tidak bergantung beban.** Penurunan energi per permintaan dari software ke DS28E38: −92,67 % (A), −92,68 % (C), −92,61 % (B). Pada tegangan 18 Nov hasil ini sedikit bergeser tetapi tetap sekitar 92,7 %. Keunggulan tidur prosesor makin besar mendekati kapasitas: 63,1 poin (A), 82,3 poin (C), 83,6 poin (B). Interaksi sudah hampir penuh pada ρ = 1.

**P5. Kapasitas efektif sedikit di bawah 1/T_sign.** Siklus layanan nyata software 660,2 ms terhadap latensi 654,1 ms; selisih 6,1 ms (0,93 %) adalah kerja di luar jendela latensi (menerima dan mengurai frame, mengirim balasan). Pada DS28E38 selisihnya mendekati nol. Karena itu serangan berperiode tepat 1/T_sign sedikit melampaui kapasitas efektif software, sehingga 1–2 permintaan per jendela terlewat (tingkat layanan 98,35 %).

**P6. Kuantisasi tingkat layanan K5.** Satu jendela hanya memuat sekitar 90 frame. Pergeseran jam attacker sebesar 0,1 % menentukan apakah frame ke-91 masuk jendela. Maka TL K5 bernilai 97,80 % atau 98,89 %, padahal jumlah yang dilayani sama (89). Ini bukan perbedaan perilaku papan.

**P7. DS28E38 kehilangan frame di penerima saat diserang tepat di kapasitas.** 0–6 frame per run, rerata 0,81 (15 run 0 frame, 9 run 1, 6 run 2, 1 run 4; 2 run gugur dengan 6 frame). Mekanismenya sama dengan K4: frame tiba tepat saat host pulih dari alarm tidur. Bukan salah ukur (frame rusak 0, jumlah tiba + hilang cocok). Tanda tangan lambat sekitar 227 ms (dua kali latensi) tercatat pada 12 dari 31 run K6, sejalan dengan 10 dari 30 run K4.

**P8. Urgensi latensi (jawaban revisi penguji 1).** Latensi menentukan: (a) energi per permintaan, karena energi ≈ daya aktif × waktu tanda tangan; (b) kapasitas layanan 1/T_sign, titik serangan mulai efektif (dibuktikan skenario C); (c) waktu respons terburuk, sekitar dua kali T_sign dengan satu permintaan tertunda; (d) laju yang harus dibangkitkan penyerang: 1,53/s (software) dan 8,6/s (DS28E38).

### 2.1 Keterbatasan yang harus ditulis

1. Skenario C diambil pada hari berbeda dari A/B. Perbandingan C dengan A/B bersifat deskriptif. Perbandingan paling bersih adalah K5 dengan K6.
2. Daya dan energi K5/K6 dinormalisasi ke tegangan 18 Nov (5,22 V); tegangan USB saat pengambilan data terukur 5,13 V. Efeknya hanya pada daya dan energi (selisih sekitar 2 %); metrik arus tidak terpengaruh.
3. Daya diam (IDLE) skenario C memakai IDLE 18 Nov, tidak diukur ulang.
4. Batas frame hilang 1 % ditetapkan setelah run awal skenario C terlihat; pengaruhnya diuji pada K4 dan K6 (bagian 1.5) dan tidak mengubah kesimpulan.
5. Hanya satu titik utilisasi (ρ = 1) yang diukur; titik di antara A, C, dan B adalah garis penghubung, bukan hasil ukur. Ini berlaku juga untuk Gambar 5.13 dan 5.14.
6. Penyerang dimodelkan sebagai pengirim periodik; penyerang adaptif tidak diuji.
7. Rata-rata jendela memuat jeda awal attacker (bagian 1.6).

---

## 3. Gambar yang diubah, ditambah, dan dibiarkan

Berkas ada di `data/analisis/kapasitas/gambar/`. Skrip: `tools/3_analisis/gambar_skenario_c.py`. Semua gambar memakai metrik jendela 60 detik, sama dengan A/B.

### 3.1 Diganti (data K5 dan K6 ditambahkan)

| No | Berkas | Yang berubah | Pesan yang harus terbaca |
|---|---|---|---|
| **5.7** | `Gambar_5.7_durasi_tidur_cpu.png` | dua batang baru; urutan batang: normal, kapasitas, barrage | software jatuh 35 → 2,2 %, DS28E38 98 → 84 % |
| **5.8** | `Gambar_5.8_daya_dinamis.png` | dua batang baru dengan batang galat; K5/K6 memakai tegangan 18 Nov | K5 72,75 mW hampir sama dengan K3 74,52 mW |
| **5.9** | `Gambar_5.9_energi_per_permintaan.png` | dua batang baru | energi per permintaan tetap per arsitektur: ± 49 mJ (software), ± 3,6 mJ (DS28E38) |
| **5.11** | `Gambar_5.11_tingkat_layanan_vs_model.png` | dua pasang batang baru (terukur dan model) | model 100 %, terukur 98,3 % dan 99,6 % |
| **5.12** | `Gambar_5.12_throughput_vs_batas.png` | dua batang baru; garis kapasitas 1,53 dan 8,60 | K5 dan K6 menyentuh garis kapasitas |

Caption yang disarankan untuk 5.8 dan 5.9: tambahkan kalimat "Pengukuran skenario kapasitas dihitung pada tegangan pengukuran 18 November."

### 3.2 Diperluas

| No | Berkas | Yang berubah |
|---|---|---|
| **5.1** | `Gambar_5.1_frame_hilang_K4_K6.png` | dari satu panel (K4) menjadi dua panel (K4 dan K6), dengan satu batas yang sama (1 % dari permintaan yang tiba). Sumbu jumlah run dibuat bilangan bulat |

### 3.3 Gambar baru

| No | Berkas | Isi | Letak usulan |
|---|---|---|---|
| **5.13** | `Gambar_5.13_durasi_tidur_terhadap_utilisasi.png` | tidur prosesor lawan seberapa penuh papan diserang (ρ), skala log, tiga titik per arsitektur | sub-bab baru 5.10 "Perilaku pada Kapasitas Layanan" |
| **5.14** | `Gambar_5.14_arus_terhadap_laju_datang.png` | arus rata-rata lawan permintaan per detik, dengan garis kapasitas tiap arsitektur | sub-bab 5.10, setelah 5.13 |

Catatan nomor: karena sub-bab baru 5.10 menggeser 5.10 lama dan seterusnya (v5 butir F7), **nomor gambar di Word akan bergeser**. Pakai penomoran otomatis Word dan klik Update Field.

### 3.4 Tidak diubah (dengan alasan)

| No | Isi | Alasan |
|---|---|---|
| 5.2 | Bland-Altman durasi tidur | membandingkan dua cara ukur pada K1–K4; K5/K6 boleh ditambah (opsional), tidak wajib |
| 5.3 | Arus diam antar blok | skenario C tidak punya IDLE baru |
| 5.4, 5.5 | Jejak arus satu run | ilustrasi satu run; K5/K6 opsional (bagian 4.4) |
| 5.6 | Latensi | latensi K5/K6 sama dengan K3/K4 (654,1 dan 116,3 ms); cukup disebut di teks |
| 5.10 (lama) | Interaksi arsitektur × skenario | rancangan 2×2 tidak boleh dicampur dengan skenario C; laju C berbeda per arsitektur |

### 3.5 Tabel yang ikut berubah (bukan gambar)

| Tabel | Perubahan |
|---|---|
| 3.2 | tambah baris K5, K6 |
| Tabel skenario (3.x, baru) | laju, utilisasi, tujuan, model serangan |
| 5.11 | catatan kaki "0,38 % = jeda awal; selama serangan 0,00 %" |
| 5.13 | tambah baris K5 (tiba 90–91, dilayani 89) dan K6 (tiba 511–517, dilayani 509–515) |
| Tabel baru 5.x | K5 lawan K6 dan pembanding sehari |

---

## 4. Gambar yang masih bisa ditambah (belum dibuat)

### 4.1 Daftar prioritas

| Prioritas | Gambar | Fungsi | Dibuat oleh |
|---|---|---|---|
| Wajib bila dosen menuntut | **3.6 Diagram garis waktu skenario C** | menjelaskan "periode serangan = latensi, sehingga permintaan baru tiba tepat saat yang lama selesai" (menjawab revisi penguji 3, perjelas skenario) | digambar manual |
| Opsional | Jejak arus K5 dan K6 (seperti 5.4) | memperlihatkan bentuk arus saat kapasitas | bisa saya buat |
| Opsional | Dekomposisi energi K5/K6 | membuktikan fase aktif sama dengan K3/K4 | bisa saya buat |
| Opsional | Bland-Altman dengan K5/K6 | konsistensi dua cara ukur tidur | bisa saya buat |

### 4.2 Pilihan yang menunggu keputusan Anda

- **Posisi skenario C.** Usul saya: tidak menambah hipotesis baru. Skenario ditambahkan setelah seminar proposal, sehingga dianalisis deskriptif plus K5 lawan K6, dan rancangan 2×2 tetap utuh.
- **Metrik utama skenario C.** Usul saya: metrik jendela (sama dengan A/B) sebagai utama, metrik tunak sebagai pelengkap.
- **Perbandingan C lawan B.** Usul saya: pakai pembanding sehari (run B 27 Nov, n kecil), bukan nilai p lintas hari.

---

## 5. Urutan kerja yang disarankan

1. Putuskan butir 4.2.
2. Ganti gambar 5.7, 5.8, 5.9, 5.11, 5.12 dan perluas 5.1 di Word dengan berkas pada bagian 3.
3. Tulis sub-bab baru 5.10 dan sisipkan 5.13 dan 5.14 (draf kalimat di v5 bagian 5.2).
4. Perbarui tabel (bagian 3.5).
5. Tambahkan keterbatasan (bagian 2.1).
6. Gambar Diagram 3.6 secara manual bila diperlukan.
7. Klik Update Field untuk daftar isi, tabel, dan gambar.
