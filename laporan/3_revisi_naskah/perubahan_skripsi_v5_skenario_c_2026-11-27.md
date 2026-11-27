# Laporan perubahan skripsi: skenario C, serangan tepat di kapasitas maksimum (27 November 2026)

Dasar: revisi dosen penguji pasca seminar proposal:
1. apa urgensi menghitung latensi tanda tangan;
2. tambahkan skenario serangan tepat di kapasitas maksimum software dan hardware untuk melihat perilaku papan;
3. perjelas skenario pengujian dan serangan.

Dibandingkan terhadap: `paper/SKRIPSI_FULL_Syauqi_v4_2026-11-20.pdf` beserta laporan kalibrasi
`perubahan_skripsi_v4_kalibrasi_2026-11-21.md`.
- **Semua angka di laporan ini sudah terkalibrasi** (R_eff 0,0821 Ω, offset −2,83).
- Letak ditulis **hal. cetak (PDF hal.)**, dengan hal. cetak = PDF − 13.

Sumber data:
- `data/tahap_kapasitas.txt` (manifest), `data/kapasitas_periode.json` (periode attacker), dan
  `data/analisis/kapasitas/` (ringkasan.md, banding.csv, deskriptif.csv, runs.csv).
- Catatan kerja: `logbook/2026-11-27.md`.
- Metrik "tunak" dihitung dari CSV mentah dengan aturan di bagian 2, T7.

---

## 0. Ringkasan

**Yang didapat**
1. **Kapasitas tercapai tepat pada laju 1/latensi.** Pada skenario C (laju = 1/T_sign), throughput kedua arsitektur
   sama dengan barrage 10/s. Laju yang lebih rapat dari itu tidak menambah satu pun tanda tangan.
2. **Software kehilangan seluruh waktu tidurnya sudah pada 1,53 permintaan/s.** Dalam keadaan tunak CPU tidak tidur
   sama sekali (0,000 %), dan arusnya sama dengan arus saat barrage (16,71 vs 16,70 mA). Penyerang cukup mengirim
   15 % dari jumlah frame barrage untuk menimbulkan kerusakan yang sama.
3. **DS28E38 tetap tidur sekitar 84 % waktu walau dibebani penuh** pada kapasitasnya sendiri (8,6 permintaan/s).
   Untuk mencapai kerusakan maksimum, penyerang harus mengirim 5,6 kali lebih rapat, dan itu pun tidak menghapus
   waktu tidur.
4. **Keunggulan energi per permintaan tidak bergantung pada beban.** Penghematannya −92,7 % pada C, sama dengan
   −92,7 % (A) dan −92,6 % (B).
5. **Kapasitas nyata sedikit di bawah 1/T_sign.**
   - Software membutuhkan ±6,1 ms per permintaan di luar jendela latensi, sehingga kapasitas efektifnya
     1,515/s, bukan 1,529/s, dan TL di ambang 97,8–98,9 %.
   - Pada DS28E38 overhead ini praktis nol (TL 99,61 %).
6. **Temuan metode: attacker mengirim frame pertama sekitar 2 periode setelah jendela mulai.** Rata-rata satu
   jendela karenanya memuat jeda menganggur di awal. Contoh paling jelas: angka "CPU software tidur 0,38 % saat
   barrage" di v4 seluruhnya berasal dari jeda ini; selama serangan, CPU tidak tidur sama sekali.

**Dampak ke kesimpulan**
- **Tidak ada kesimpulan v4 yang terbalik.** H1–H4 tetap didukung, dan arah serta besar keunggulan DS28E38 tetap.
- **Bertambah satu jawaban** untuk pertanyaan "perilaku di kapasitas maksimum", sekaligus jawaban konkret untuk
  "urgensi latensi".
- **Ada lima pernyataan v4 yang perlu diperhalus atau dikoreksi** (bagian 3):
  - "0,38 %";
  - "menyentuh kapasitas 1/T_sign";
  - "di bawah 1,53 permintaan/s semua terlayani";
  - "perkiraan model, bukan pengukuran";
  - besar interaksi +20,5 poin. Pada keadaan tunak nilainya +18,6 poin; arahnya tetap, sehingga hipotesis
    interaksi tetap didukung.

**Yang menunggu keputusan Anda:** enam butir di bagian 8. Yang terpenting adalah K1, toleransi frame hilang
skenario C.

---

## 1. Data yang diambil (27 Nov)

| Langkah | Sesi | Run sah |
|---|---|---|
| Uji pendahuluan K2S (5 A + 2 B): latensi dan koreksi jam attacker | 20261127_152237_K2S | 7/7 |
| Uji regresi attacker baru (1 A + 1 B) | 20261127_191956_K2S | 2/2 |
| Uji pendahuluan K1S (5 A + 2 B) | 20261127_192601_K1S | 7/7 |
| Blok 1, K1S skenario C | 20261127_193446_K1S | 15/15 |
| Blok 2, K2S skenario C | 20261127_195829_K2S | 14/15 |
| Blok 3, K2S skenario C | 20261127_201523_K2S | 14/15 |
| Blok 3b, K2S skenario C (pengganti) | 20261127_203236_K2S | 3/3 |
| Blok 4, K1S skenario C | 20261127_204806_K1S | 15/15 |

Kelompok baru:
- **K5 = software skenario C**, n = 30.
- **K6 = DS28E38 skenario C**, n = 31. Ketiga puluh satu run sah dipakai seluruhnya tanpa pemotongan, sama seperti
  aturan 18 Nov.

Pola ABBA: K1S | K2S | K2S | K1S.

Periode attacker:

| | Dikirim ke attacker | Periode nyata | Laju nyata |
|---|---|---|---|
| K1S | `p655095` | 654,100 ms | 1,529/s |
| K2S | `p116188` | 116,154 ms | 8,609/s |

Periode yang dikirim sudah dikoreksi dengan laju B terukur, yaitu jam attacker terhadap kristal logger.

Biner:
- host sama dengan 18 Nov (`host_k1_stop 2ca6e8d0d861`, `host_k2_stop 5b4e017827e3`);
- attacker baru `17665901c2fa` (mode C ditambahkan; mode A/B tidak berubah, regresi lulus).

IDLE tidak diambil ulang (keputusan Anda). P_idle/I_idle diambil dari IDLE 18 Nov (`stop_final`).

**Jembatan ke 18 Nov** (uji pendahuluan 27 Nov dibanding data final 18 Nov):

| Metrik | K1S | K2S |
|---|---|---|
| Latensi | 654,10 vs 654,88 ms (−0,12 %) | 116,15 vs 116,23 ms (−0,07 %) |
| I_avg A | −0,7 % | −0,7 % |
| I_avg B | −0,5 % | −0,4 % |
| TL B | sama | sama |
| V_bus | 5,13 vs 5,22 V | 5,13 vs 5,22 V |

Akibat tegangan yang lebih rendah, daya 27 Nov ±2 % lebih rendah walau arus hampir sama.

---

## 2. Apa yang didapat (temuan)

Angka "jendela" mengikuti definisi metrik v4 (rata-rata selama jendela 60 s). Angka "tunak" dihitung dari frame
permintaan pertama sampai akhir jendela (T7).

### Tabel ringkas

| Metrik | K1 SW A | K2 HW A | K5 SW C | K6 HW C | K3 SW B | K4 HW B |
|---|---|---|---|---|---|---|
| Laju datang (/s) | 0,98 | 0,98 | 1,51 | 8,59 | 10,02 | 10,02 |
| ρ terhadap T_sign | 0,64 | 0,11 | ≈ 1,00 | ≈ 1,00 | 6,56 | 1,17 |
| Latensi (ms) | 654,9 | 116,2 | 654,1 | 116,3 | 654,9 | 116,3 |
| TL (%) | 100 | 100 | 98,35 | 99,61 | 15,04 | 85,50 |
| Throughput jendela (/s) | 0,977 | 0,983 | 1,481 | 8,555 | 1,507 | 8,565 |
| Throughput tunak (/s) | 1,011 | 1,018 | **1,515** | **8,593** | **1,513** | **8,597** |
| CPU tidur, jendela (%) | 35,08 | 98,17 | 2,22 | 84,47 | 0,38 | 83,97 |
| CPU tidur, tunak (%) | 32,84 | 98,10 | **0,00** | **84,41** | **0,00** | **83,91** |
| I_avg jendela (mA) | 11,82 | 3,14 | 16,39 | 8,37 | 16,73 | 8,45 |
| I_avg tunak (mA) | 12,14 | 3,16 | 16,71 | 8,39 | 16,79 | 8,47 |
| Energi dinamis per permintaan E_lyn (mJ) | 49,98 | 3,67 | 48,13 | 3,52 | 49,44 | 3,65 |
| Muatan dinamis per permintaan Q_lyn (mC) | 9,58 | 0,701 | 9,41 | 0,692 | 9,48 | 0,701 |
| Energi papan per tanda tangan (mJ) | 63,07 | 16,64 | 56,77 | 5,02 | 57,93 | 5,14 |

Catatan tabel: K1–K4 berasal dari 18 Nov (V_bus 5,22 V); K5–K6 dari 27 Nov (V_bus 5,13 V). Baca catatan T8 sebelum
membandingkan kolom C dengan kolom A/B.

**Pembanding sehari** (run B 27 Nov, keadaan tunak; n = 2 untuk K1S, n = 3 untuk K2S):

| | Software B | DS28E38 B |
|---|---|---|
| Throughput | 1,5145/s | 8,6046/s |
| CPU tidur | 0,00 % | 83,96 % |
| I_avg | 16,70 mA | 8,43 mA |
| P_avg | 85,63 mW | 43,23 mW |

### T1. Kapasitas tercapai tepat pada λ = 1/T_sign
- **Throughput tunak C sama dengan barrage.** Software 1,5147/s lawan 1,5145/s (B, hari yang sama); DS28E38
  8,593/s lawan 8,605/s.
- **Laju di atas 1/T_sign hanya menambah permintaan yang dibuang** (K3 membuang 85 %, K4 membuang 14,5 %), tanpa
  menambah tanda tangan.
- **Titik serangan "maksimum yang masih efisien" bagi penyerang adalah kapasitas host itu sendiri.**

### T2. Software: deprivasi tidur sudah penuh pada laju 1,53/s
- **CPU terjaga terus.** Pin PB1 tinggi tanpa putus dari frame pertama sampai akhir jendela (CPU tidur tunak
  0,000 %).
- **Arus tunak sama dengan barrage hari yang sama:** 16,71 lawan 16,70 mA (selisih 0,05 %). Daya: 85,70 lawan
  85,63 mW.
- **Penyerang hanya perlu 15 % jumlah frame barrage** (1,51 lawan 10,02 per detik) untuk menimbulkan beban arus
  98–100 % barrage. Nilai 98 % (jendela) berbeda dari 100 % (tunak) karena jeda awal (T7).

### T3. DS28E38: lantai tidur ≈ 84 % bersifat struktural
- **CPU tetap tidur pada kapasitasnya sendiri.** Pada 8,6 permintaan/s CPU masih tidur 84,4 % (tunak), sedikit di
  atas barrage (83,9–84,0 %). Penyebabnya, throughput C sedikit lebih rendah (8,593 lawan 8,605/s).
- **Arus C = 99,6 % arus barrage** (tunak 8,39 lawan 8,43 mA).
- **Penurunan tidur dari A ke C ditentukan oleh jumlah tanda tangan yang dikerjakan**, bukan oleh laju frame
  datang. Ini sejalan dengan analisis 5.7 v4 (±19 ms terjaga per permintaan).

### T4. Keunggulan energi per permintaan tidak bergantung pada beban
Penurunan E_lyn dari software ke DS28E38:

| Skenario | Penurunan E_lyn |
|---|---|
| A | −92,67 % |
| C | −92,68 % |
| B | −92,61 % |

Q_lyn juga turun −92,6 % hingga −92,7 % di ketiganya.

Keunggulan CPU tidur pada jendela: A 63,1 poin, C 82,3 poin, B 83,6 poin. Interaksi durasi tidur sudah hampir
penuh pada kapasitas software (ρ = 1). Menaikkan laju sampai 6,6 kali kapasitas hanya menambah 1,3 poin.

### T5. Kapasitas efektif lebih rendah dari 1/T_sign (overhead layanan)

| | T_sign (latensi) | Siklus layanan nyata | Overhead | Kapasitas efektif |
|---|---|---|---|---|
| Software | 654,10 ms | 660,2 ms (C) / 660,3 ms (B tunak) | **6,1 ms (0,93 %)** | **1,515/s** (1/T_sign = 1,529/s) |
| DS28E38 | 116,21 ms | 116,22 ms (B tunak) | **≈ 0** | ≈ 8,60/s |

- **Siklus layanan nyata dihitung dari rentang sibuk dibagi jumlah permintaan dilayani.** Hasilnya sama di skenario
  C dan B tunak, dan sama pula di data 18 Nov (K1S B tunak 661,0 ms terhadap latensi 654,9 ms).
- **Overhead adalah kerja di luar jendela latensi** (terima dan urai frame, kirim balasan, persiapan). Rinciannya
  belum dipisah.
- **Akibat di software:** serangan berperiode = T_sign sedikit melebihi kapasitas efektif (ρ terhadap siklus layanan
  = 1,009). Antrean satu-tertunda pelan-pelan penuh, sehingga 1–2 permintaan per jendela terlewat:
  - blok 1: 91 datang, 89 dilayani, TL 97,80 %;
  - blok 4: 90 datang, 89 dilayani, TL 98,89 %.
- **Dua nilai TL K5 itu efek pembulatan.** Satu jendela hanya memuat ±90 frame, dan pergeseran jam attacker ±0,1 %
  menentukan apakah frame ke-91 masih masuk jendela. Jumlah yang dilayani sama (89); arus, energi, dan latensi
  sama.
- **Akibat di DS28E38:** TL 99,61 % (2 terlewat per ±515). Penyebabnya batas akhir jendela dan tanda tangan lambat
  yang jarang (T6).

### T6. DS28E38 kehilangan frame di penerima saat diserang tepat di kapasitas
- **Besarnya:** 0–6 frame per run, rerata 0,81 pada 31 run sah. Sebarannya: 15 run 0 frame, 9 run 1, 6 run 2,
  1 run 4. Dua run gugur dengan 6 frame (1,2 %).
- **Mekanismenya sama dengan 18 Nov:** frame mulai tepat saat host pulih dari alarm Stop. Di skenario C lebih
  sering karena periode serangan hampir sama dengan siklus host, sehingga fase frame bergeser pelan dan berulang
  kali jatuh di celah yang sama.
- **Bukan salah ukur:**
  - frame rusak 0;
  - tiba + hilang = 516–517, yaitu jumlah yang dikirim attacker;
  - software tidak pernah kehilangan frame (host software tidak masuk Stop selama serangan).
- **Ada tanda tangan lambat ±227 ms** (sekitar 2 kali latensi) pada 12 dari 31 run K6. Sebagai pembanding, di K4
  (18 Nov) ada 10 dari 30 run dengan tanda tangan lambat ±210 ms. Ini sejalan dengan butir keterbatasan 4 v4.

### T7. Temuan metode: jeda awal attacker (berlaku juga untuk data 18 Nov)
Attacker mengirim frame pertama sekitar **2 periode + 20 ms** setelah `>START`:

| Skenario | Jeda awal |
|---|---|
| A | 2,02 s |
| B | 0,22 s |
| C, K1S | 1,33 s |
| C, K2S | 0,26 s |

- **Akibatnya, rata-rata jendela memuat sebagian waktu menganggur.** Metrik tunak (dari frame pertama sampai tepi
  turun terakhir) menghilangkannya.
- **Software barrage:** CPU tidur 0,38 % (jendela) lawan 0,00 % (tunak). Angka 0,38 % = 0,227 s / 60 s, murni jeda
  awal.
- **Software A:** 35,08 % lawan 32,84 % tidur.
- **Interaksi durasi tidur (RM3):** +20,51 poin (jendela) lawan **+18,64 poin (tunak)**:
  - selisih A: 63,08 lawan 65,26 poin;
  - selisih B: 83,59 lawan 83,91 poin.

  Arahnya tetap positif, jadi hipotesis interaksi tetap didukung, tetapi besarnya bergantung definisi jendela.
- **Persentase penghematan energi per permintaan tidak terpengaruh.** Jeda awal tidak memuat permintaan.

### T8. Batas perbandingan lintas hari (C lawan A/B)
- **K5 lawan K6 adalah perbandingan paling bersih** (hari sama, ABBA).
- **K3 lawan K5 dan K4 lawan K6 membandingkan 18 Nov dengan 27 Nov.** Uji Welch di `banding.csv` memang "menolak
  H0" pada hampir semua metrik, tetapi sebagian besar selisihnya adalah efek hari, tegangan, dan jeda awal, bukan
  efek skenario:
  - selisih latensi K3–K5 −0,81 ms hampir sama dengan jembatan latensi −0,78 ms;
  - selisih I_avg K3–K5 −0,34 mA lenyap pada pembanding sehari tunak (16,71 lawan 16,70 mA).
- **Kesimpulan C lawan B diambil dari pembanding sehari tunak** (T2, T3), bukan dari nilai p lintas hari.
- **Dua sumber bias kecil lain:**
  - P_idle C diambil dari 18 Nov (V_bus lebih tinggi), sehingga P_dyn/E_lyn C bias ±−0,2 mW. Gunakan I_dyn/Q_lyn
    untuk C.
  - I_idle 27 Nov diperkirakan ±0,02 mA lebih rendah dari 18 Nov (jembatan K2S A), sehingga I_dyn C bias
    ≤ 0,3 %.

### T9. Jawaban konkret untuk pertanyaan dosen 1 (urgensi latensi)
Latensi tanda tangan adalah besaran yang **menentukan**:
- **(a) energi per permintaan.** E ≈ P_aktif × T_sign; keunggulan 92,7 % berasal dari T_sign yang 5,6 kali lebih
  singkat, ditambah CPU yang tidur selama IC bekerja.
- **(b) kapasitas layanan.** Θ_maks ≈ 1/T_sign, yaitu titik saat serangan mulai efektif. Skenario C membuktikannya:
  pada laju 1/T_sign software sudah kehilangan seluruh waktu tidurnya.
- **(c) waktu respons terburuk.** Sekitar 2 × T_sign dengan satu permintaan tertunda.
- **(d) besar laju yang harus dibangkitkan penyerang** untuk kerusakan maksimum: 1,53/s untuk software, 8,6/s untuk
  DS28E38.

Tanpa latensi, titik serangan C tidak bisa ditentukan.

---

## 3. Pengaruh terhadap kesimpulan dan pembahasan v4

| Pernyataan v4 (letak) | Status | Alasan / pengganti |
|---|---|---|
| H1–H4 didukung (5.10, 6.1) | **Tetap** | Skenario C tidak menguji ulang H1–H4; arah pada C sama: latensi sama, E_lyn −92,7 %, CPU tidur +82 poin |
| Hipotesis interaksi didukung, +20,5 poin (5.8, 6.1, Intisari) | **Tetap, tambah sensitivitas** | Tunak +18,6 poin (T7). Tetap positif |
| "CPU software hampir tidak pernah tidur (0,38 %)" saat barrage (Intisari, 5.7, Tabel 5.11, 5.11.6, 6.1) | **Diperhalus** | "selama serangan CPU tidak tidur sama sekali; 0,38 % berasal dari jeda sebelum frame pertama" |
| "Throughput sudah menyentuh kapasitas 1/T_sign 1,53 dan 8,60" (5.9, 5.11.4) | **Diperhalus** | Software mencapai kapasitas efektif 1,515/s = 99,1 % dari 1/T_sign; selisihnya overhead 6,1 ms (T5). DS28E38 = 1/T_sign |
| "Di bawah ~1,53 permintaan/s kedua arsitektur melayani seluruh permintaan … Perkiraan ini hasil model, bukan pengukuran" (5.11.4) | **Dikoreksi + diperbarui** | Titik 1,53 dan 8,60 kini **terukur**. Pada 1,529/s software melayani 98,35 %, bukan 100 %. Batas "seluruh terlayani" software ±1,515/s |
| "Hardware offloading mitigasi, bukan kekebalan" (5.11.4, 6.1) | **Diperkuat** | Pada kapasitasnya sendiri DS28E38 masih tidur 84 %; penyerang perlu laju 5,6 kali lebih tinggi untuk kerusakan maksimum (T2, T3) |
| Kapasitas sebagai patokan perancang (5.11.8) | **Diperhalus** | Patokan = 1/(T_sign + overhead layanan), bukan 1/T_sign murni |
| Selisih TL terhadap model −0,20/−0,33 poin (5.9) | **Tetap, tambah penjelasan** | Sejalan dengan overhead software (±0,14 poin pada K3) dan efek batas jendela; C memperlihatkan overhead itu langsung |
| Keterbatasan 14: "penyerang laju tetap 1 dan 10 /s" (5.12) | **Diubah** | Tambah laju ketiga 1/T_sign; penyerang tetap periodik dan tidak adaptif |
| Saran 7: "uji lebih banyak laju di sekitar kapasitas 1,53 dan 8,60" (6.2) | **Sebagian terpenuhi** | Satu titik (ρ = 1) terukur; sapuan ρ (mis. 0,5–1,5) masih saran |
| Saran 8: perbaiki penerimaan frame saat pulih dari alarm (6.2) | **Diperkuat** | Kehilangan frame lebih sering di ambang (T6) |

**Kesimpulan baru yang ditambahkan** (menjawab revisi 2): lihat draf 6.1 di bagian 5.

---

## 4. Rincian perubahan per bagian naskah

### A. Intisari / Abstract (PDF 10–11)
| No | Letak | Sekarang | Ganti / tambah |
|---|---|---|---|
| A1 | Paragraf 2 | "... pada kondisi normal (1 permintaan/detik) dan barrage attack (10 permintaan/detik), masing-masing 30 run sah ..." | Tambah: "..., ditambah skenario serangan tepat pada kapasitas layanan masing-masing arsitektur (laju = 1/latensi; 30 dan 31 run sah)" |
| A2 | Paragraf 3 | "... arsitektur perangkat lunak melayani 15,0 % permintaan dengan durasi tidur 0,38 % ..." | "... melayani 15,0 % permintaan dan CPU-nya tidak tidur selama serangan ..." |
| A3 | Paragraf 3, sebelum kalimat terakhir | – | Tambah kalimat: "Pada laju tepat sebesar kapasitasnya, CPU perangkat lunak sudah kehilangan seluruh waktu tidurnya pada 1,53 permintaan/detik, sedangkan CPU hardware offloading tetap tidur 84 % waktu pada 8,6 permintaan/detik." |
| A4 | Abstract | sama | Terjemahkan A1–A3 |

### B. Bab 1
| No | Letak | Sekarang | Ganti / tambah |
|---|---|---|---|
| B1 | 1.2 Perumusan Masalah, hal. 3 (PDF 16) | 3 rumusan | Lihat keputusan K2. Usul: **rumusan 4**: "Perilaku kedua arsitektur ketika diserang tepat pada kapasitas layanan maksimumnya (laju permintaan = 1/latensi tanda tangan) belum diketahui, termasuk apakah waktu tidur dan tingkat layanan sudah jatuh pada titik tersebut." |
| B2 | 1.3 butir 3, hal. 4 (PDF 17) | "Hanya dua skenario yang diuji ..., yaitu kondisi normal dan barrage attack." | "Tiga skenario diuji ...: kondisi normal, barrage attack, dan serangan pada kapasitas layanan." |
| B3 | 1.3 butir 4 | "... dua laju permintaan tetap, yaitu 1 ... dan 10 ..., masing-masing di bawah dan di atas kapasitas ..." | "... dua laju tetap bersama (1 dan 10 permintaan/detik) di bawah dan di atas kapasitas, serta satu laju per arsitektur sebesar 1/latensi tanda tangannya (sekitar 1,53 dan 8,61 permintaan/detik)." |
| B4 | 1.3 butir 11 | "... kehilangan ini dibatasi kurang dari 0,25 % ..." | Tambah: "(skenario kapasitas: kurang dari 1 %, lihat 3.4.x)", sesuai keputusan K1 |
| B5 | 1.4 Tujuan | – | Tambah tujuan yang selaras dengan RM4 (bila K2 = RM4) |

### C. Bab 2
| No | Letak | Ganti / tambah |
|---|---|---|
| C1 | 2.7 Hipotesis, hal. 19 (PDF 32) | **Jangan menambah hipotesis baru.** Skenario C ditambahkan setelah seminar proposal atas permintaan penguji, jadi hipotesis yang dibuat setelah data terlihat tidak sah sebagai uji konfirmatori. Tambah satu kalimat: "Skenario serangan pada kapasitas (Sub-bab 3.4.1) dianalisis secara deskriptif dan komparatif tanpa hipotesis terarah." |
| C2 | 2.6 Kerangka berpikir | Opsional: satu kalimat bahwa kapasitas 1/T_sign adalah titik saat serangan mulai efektif |

### D. Bab 3
| No | Letak | Ganti / tambah |
|---|---|---|
| D1 | 3.2.3, Tabel 3.2, hal. 25 (PDF 38) | Tambah baris **K5** (software, kapasitas, ±1,53 permintaan/detik, 30 run) dan **K6** (DS28E38, kapasitas, ±8,61 permintaan/detik, 31 run). Ubah "empat kelompok" menjadi "empat kelompok faktorial ditambah dua kelompok skenario kapasitas". Catatan: K5/K6 diambil 27 Nov dengan IDLE acuan 18 Nov |
| D2 | 3.4.1 Skenario Pengujian, hal. 34 (PDF 47) | Tambah butir 3 "Skenario C" (draf di bagian 5) dan **Tabel skenario** (draf di bagian 5). Ini sekaligus menjawab revisi 3 |
| D3 | 3.4.2 Prosedur | Tambah: (a) uji pendahuluan per arsitektur (5 A + 2 B) sebelum skenario C; (b) koreksi periode attacker terhadap jam logger, periode_kirim = T_sign × λ_B / 10; (c) blok ABBA K1S, K2S, K2S, K1S, 15 run per blok; (d) IDLE tidak diulang |
| D4 | 3.4.5 Metrik | Tambah: **ρ_nyata** = λ_eff × T_sign; **siklus layanan** T_svc = rentang sibuk / N_lyn; **kapasitas efektif** Θ_eff = 1/T_svc; **metrik tunak** (dari frame pertama sampai akhir jendela) sebagai pelengkap |
| D5 | 3.4.6 (aturan frame hilang) | Tambah aturan C: frame hilang < 1 % dari yang tiba, frame rusak tetap 0, beserta alasannya (sesuai K1) |
| D6 | 3.4.7 Uji | Tambah: K5 lawan K6 diuji dengan Welch + Mann-Whitney, Holm atas 6 metrik utama (latensi, TL, CPU tidur, I_avg, P_avg, E per tanda tangan). Perbandingan C lawan B (K3–K5, K4–K6) **deskriptif** karena lintas hari; pembanding sehari = run B 27 Nov |
| D7 | Persamaan 3.12 (model TL) | Tambah catatan: model memakai T_sign; kapasitas efektif memakai T_svc |

### E. Bab 4
| No | Letak | Ganti / tambah |
|---|---|---|
| E1 | 4.4 Implementasi Attacker Node, hal. 52 (PDF 65) | Tambah mode C: perintah `p<us>` + Enter (periode 1 ms–10 s), entri dibatalkan bila ada karakter asing, `s` menampilkan periode. Mode A/B tidak diubah (uji regresi 27 Nov: A 59/59, B TL 85,55 %) |
| E2 | 4.6 Skrip | Tambah `tools/1_akuisisi/periode_kapasitas.py` (periode dari uji pendahuluan), `run_bench.py` skenario C (`--idle-dari`, `rho_nyata`, toleransi per skenario), `tools/3_analisis/analisis_kapasitas.py` |
| E3 | 4.7.4 Ketertelusuran | Tambah sha attacker: lama `faf092a1fff6` (data 18 Nov), baru `17665901c2fa` (data 27 Nov) |

### F. Bab 5
| No | Letak | Ganti / tambah |
|---|---|---|
| F1 | 5.1 Pelaksanaan, hal. 58 (PDF 71) | Tambah tabel sesi 27 Nov (bagian 1) dan jembatan ke 18 Nov |
| F2 | 5.2.1 Keutuhan | Tambah: K5 frame hilang 0; K6 rerata 0,81, maksimum 4 di run sah; 2 run gugur (hilang 6); frame rusak 0 |
| F3 | 5.2.6 Sensitivitas, hal. 66 (PDF 79) | Tambah tabel sensitivitas K6 (bagian 6) |
| F4 | 5.7, Tabel 5.11, hal. 73 (PDF 86) | Baris CPU tidur: catatan kaki "0,38 % = jeda sebelum frame pertama; selama serangan 0,00 %" |
| F5 | 5.8 Interaksi | Tambah satu kalimat sensitivitas: tunak +18,6 poin |
| F6 | 5.9, Tabel 5.13, hal. 75–76 (PDF 88–89) | Tambah baris K5 (N_dtg 90–91, N_lyn 89, N_lwt 1–2, ρ ≈ 1,00) dan K6 (N_dtg 511–517, N_lyn 509–515, N_lwt 2, ρ ≈ 1,00). Ganti "masing-masing sudah menyentuh kapasitas maksimum 1/T_sign" dengan kalimat T5 |
| F7 | **Sub-bab baru 5.10 "Perilaku pada Kapasitas Layanan (Skenario C)"** sebelum rekapitulasi; rekap menjadi 5.11 dan pembahasan 5.12, dst. | Draf lengkap di bagian 5, dengan tabel K5/K6 dan pembanding |
| F8 | 5.10 Rekapitulasi | Tambah baris "Skenario C (deskriptif)" atau tabel terpisah K5 lawan K6 dari `banding.csv` |
| F9 | 5.11.4, hal. 80 (PDF 93) | Ganti paragraf "perkiraan untuk kebijakan tanpa antrean" (draf di bagian 5) |
| F10 | 5.11.6, tabel perbandingan, Pirretti | "0,38 % saat barrage" menjadi "0 % selama serangan (0,38 % per jendela)" |
| F11 | 5.11.7 Implikasi keamanan | Tambah: biaya penyerang. Kerusakan maksimum software cukup dengan 1,53/s; DS28E38 butuh 8,6/s dan tetap tidak menghapus waktu tidur |
| F12 | 5.11.8 Patokan perancang | "1/T_sign … 1,53 … 8,60" menjadi "1/(T_sign + overhead layanan): terukur 1,515 (software) dan 8,60 (DS28E38)" |
| F13 | 5.12 Keterbatasan | Ubah butir 14; tambah butir di bagian 7 |
| F14 | Gambar (opsional) | (a) CPU tidur lawan ρ (A, C, B) per arsitektur; (b) arus tunak lawan laju datang dengan garis kapasitas; (c) Tabel 5.13 diperluas. Lihat keputusan K5 |

### G. Bab 6
| No | Letak | Ganti / tambah |
|---|---|---|
| G1 | 6.1, paragraf metode, hal. 87 (PDF 100) | "eksperimen faktorial 2 × 2 ..." ditambah "... serta skenario serangan pada kapasitas layanan masing-masing arsitektur" |
| G2 | 6.1, RM2 | "(0,38 % waktu)" menjadi "tidak tidur sama sekali selama serangan" |
| G3 | 6.1, RM3 | Tambah "(+18,6 poin bila jeda awal dikeluarkan)" |
| G4 | 6.1, paragraf baru RM4 | Draf di bagian 5 |
| G5 | 6.2 Saran 7 | "menguji lebih banyak laju di sekitar kapasitas" menjadi "menyapu utilisasi 0,5–1,5 di sekitar kapasitas efektif (1,515 dan 8,60 permintaan/detik) dan memisahkan komponen overhead layanan ±6 ms pada perangkat lunak" |
| G6 | 6.2 Saran 8 | Tambah "kehilangan frame paling sering terjadi saat laju serangan menyamai siklus layanan" |
| G7 | 6.2 saran baru | "Menyalakan attacker sebelum jendela dibuka, atau membuang jeda sebelum frame pertama dari jendela, agar rata-rata jendela sama dengan keadaan tunak" |

### H. Lampiran, lambang, pustaka
- **Lampiran:** manifest `data/tahap_kapasitas.txt`, periode `data/kapasitas_periode.json`, sha biner (E3).
- **Daftar lambang:** ρ_nyata, T_svc, Θ_eff, K5, K6.
- **Pustaka:** tidak ada yang baru. Hukum utilisasi (Lazowska dkk., 1984) sudah ada dan dipakai juga untuk C.

---

## 5. Draf teks baru (siap ditempel, sesuaikan nomor)

### 5.1 Untuk 3.4.1 (butir 3 dan tabel)
> 3. Skenario C, serangan pada kapasitas layanan: laju permintaan disamakan dengan kapasitas masing-masing
> arsitektur, yaitu 1/T_sign hasil uji pendahuluan pada hari pengukuran (654,10 ms, atau sekitar 1,529
> permintaan/detik, pada software-based; 116,15 ms, atau sekitar 8,609 permintaan/detik, pada hardware offloading),
> sehingga utilisasi nominal ρ = 1. Periode yang dikirim ke attacker node dikoreksi dengan laju terukur pada
> skenario B agar periode nyata menurut jam logger sama dengan T_sign. Skenario ini menguji perilaku papan tepat
> di batas kemampuannya: apakah seluruh permintaan masih terlayani, dan seberapa besar waktu tidur yang tersisa.
> Prosedur diulang 30 kali (software-based) dan 31 kali (hardware offloading) dalam empat blok ABBA.

**Tabel 3.x Ringkasan skenario pengujian dan model serangan**

| Skenario | Laju permintaan | ρ software | ρ hardware | Posisi terhadap kapasitas | Tujuan |
|---|---|---|---|---|---|
| A, normal | 1/detik (sama untuk keduanya) | 0,64 | 0,11 | di bawah | layanan setara; dasar pembanding |
| C, kapasitas | 1/T_sign masing-masing (≈ 1,53 dan ≈ 8,61/detik) | ≈ 1,0 | ≈ 1,0 | tepat di batas | perilaku pada beban maksimum yang masih dapat dilayani |
| B, barrage | 10/detik (sama untuk keduanya) | 6,6 | 1,2 | di atas | dampak serangan berlebih |

Model serangan untuk semua skenario:
- frame permintaan 36 byte pada UART 115.200 bit/s;
- laju tetap (periodik) tanpa umpan balik dari host, sehingga penyerang tidak menunggu host selesai;
- host menampung paling banyak satu permintaan tertunda, sedangkan permintaan lain yang datang saat sibuk dicatat
  terlewat;
- jendela pengamatan 60 detik.

### 5.2 Sub-bab baru Bab 5: "Perilaku pada Kapasitas Layanan (Skenario C)"
> Skenario C membebani setiap arsitektur tepat sebesar kapasitasnya. Rincian per kelompok ditunjukkan pada
> Tabel 5.x, bersama skenario B hari yang sama sebagai pembanding.
>
> Pada software-based, CPU terjaga terus dari permintaan pertama sampai akhir jendela; durasi tidur dalam keadaan
> tunak 0,00 %, sama dengan saat barrage attack. Arus rata-rata tunak 16,71 mA dan throughput 1,515 tanda
> tangan/detik praktis identik dengan barrage pada hari yang sama (16,70 mA dan 1,515 tanda tangan/detik). Dengan
> demikian, laju 1,53 permintaan/detik, sekitar 15 % jumlah frame barrage, sudah cukup untuk menghapus seluruh waktu
> tidur arsitektur perangkat lunak. Tingkat layanan 98,35 % (97,80–98,89 %), bukan 100 %, karena kapasitas efektif
> sedikit di bawah 1/T_sign: satu siklus layanan berlangsung sekitar 660,2 ms, atau sekitar 6,1 ms (0,93 %) lebih
> lama dari latensi tanda tangan, akibat kerja di luar jendela latensi. Antrean satu-tertunda karenanya penuh
> perlahan dan satu sampai dua permintaan per jendela terlewat.
>
> Pada hardware offloading, CPU masih tidur 84,4 % waktu walaupun DS28E38 bekerja pada kapasitas penuhnya
> (8,59 tanda tangan/detik), hampir sama dengan barrage (84,0 %). Tingkat layanan 99,61 %. Penurunan waktu tidur
> dari kondisi normal ditentukan oleh jumlah tanda tangan yang dikerjakan, bukan oleh laju frame, sehingga pada
> arsitektur ini terdapat lantai tidur sekitar 84 % yang tidak dapat diturunkan oleh penyerang. Untuk menimbulkan
> beban maksimum, penyerang harus mengirim sekitar 8,6 permintaan/detik, atau 5,6 kali laju yang dibutuhkan untuk
> melumpuhkan arsitektur perangkat lunak.
>
> Pada titik ini hardware offloading tetap unggul: latensi 116,3 lawan 654,1 ms, energi dinamis per permintaan
> 3,52 lawan 48,13 mJ (−92,7 %, sama dengan kondisi normal dan barrage), arus rata-rata 8,37 lawan 16,39 mA,
> sambil mengerjakan 5,8 kali lebih banyak tanda tangan. Keenam metrik utama berbeda bermakna (Welch-Holm,
> p < 10⁻¹²). Skenario C juga memperlihatkan kelemahan arsitektur ini: penerima frame kehilangan 0–4 frame per run
> (rerata 0,8) karena fase kedatangan frame yang hampir sama dengan siklus host berulang kali jatuh pada saat host
> pulih dari mode Stop (Sub-bab 5.2.1).

**Tabel 5.x** (gunakan angka bagian 2: K5, K6, B 27 Nov tunak; tandai mana yang jendela dan mana yang tunak).

### 5.3 Pengganti paragraf 5.11.4 ("perkiraan untuk kebijakan tanpa antrean ...")
> ... dan memungkinkan perkiraan untuk kebijakan tanpa antrean. Skenario C mengukur langsung titik kapasitas
> tersebut. Kapasitas efektif perangkat lunak 1,515 permintaan/detik, sedikit di bawah 1/T_sign (1,529), karena
> setiap permintaan memerlukan sekitar 6,1 ms di luar penandatanganan; kapasitas efektif DS28E38 sekitar 8,60
> permintaan/detik, sama dengan 1/T_sign. Pada laju di bawah kapasitas efektif, setiap arsitektur melayani
> seluruh permintaan; pada laju antara kedua kapasitas, hanya hardware offloading yang masih melayani seluruh
> permintaan; di atas 8,60 permintaan/detik keduanya jenuh. Laju di atas kapasitas tidak menambah tanda tangan
> yang dikerjakan (throughput C sama dengan B), tetapi juga tidak menambah beban arus secara berarti; kerusakan
> maksimum sudah tercapai pada laju kapasitas. Laju-laju di antara titik A, C, dan B tetap merupakan hasil model.

### 5.4 Paragraf baru 6.1 (RM4 / revisi penguji)
> Terkait perilaku pada kapasitas maksimum, serangan dengan laju tepat sebesar kapasitas masing-masing arsitektur
> (1,53 dan 8,61 permintaan/detik) sudah menimbulkan beban yang sama dengan barrage attack. CPU arsitektur
> perangkat lunak tidak tidur sama sekali dan arusnya sama dengan saat barrage, sehingga penyerang cukup mengirim
> sekitar 15 % frame barrage untuk melumpuhkan penghematan tidurnya. CPU hardware offloading tetap tidur 84 %
> waktu pada kapasitas penuhnya, dan energi per permintaannya tetap 92,7 % lebih rendah. Latensi tanda tangan
> karenanya bukan sekadar ukuran kecepatan: latensi menentukan kapasitas layanan, titik saat serangan mulai
> efektif, dan laju yang harus dibangkitkan penyerang.

### 5.5 Jawaban untuk dosen penguji (lembar revisi)
1. **Urgensi latensi.** Latensi menentukan:
   - energi per permintaan (E ≈ P × T_sign);
   - kapasitas layanan (Θ_maks ≈ 1/T_sign), sekaligus laju minimum yang harus dibangkitkan penyerang untuk
     kerusakan maksimum;
   - waktu respons terburuk (±2 T_sign).

   Skenario C membuktikan butir kedua: pada laju 1/T_sign perangkat lunak sudah kehilangan seluruh waktu tidurnya.
   Hasilnya ada di Sub-bab 3.4.1, 5.10 baru, dan 5.11.4.
2. **Skenario di kapasitas maksimum.** Ditambahkan skenario C dengan laju = 1/T_sign masing-masing arsitektur
   (ρ = 1), 30 dan 31 run sah, sesi 27 Nov dengan biner host yang sama. Hasilnya ada di sub-bab baru 5.10 dan
   paragraf baru 6.1.
3. **Kejelasan skenario.** Ditambahkan Tabel 3.x: laju, utilisasi, posisi terhadap kapasitas, tujuan, dan model
   serangan (periodik tanpa umpan balik, satu permintaan tertunda, frame 36 B, jendela 60 s).

---

## 6. Sensitivitas toleransi frame hilang K6 (untuk 5.2.6)

| Aturan | Run K6 sah | I_avg (mA) | E per tanda tangan (mJ) | CPU tidur (%) | TL (%) | Latensi (ms) |
|---|---|---|---|---|---|---|
| hilang ≤ 1 (setara < 0,25 %, aturan A/B) | 24 | 8,374 | 5,0145 | 84,44 | 99,613 | 116,24 |
| hilang < 1 % (dipakai) | 31 | 8,369 | 5,0148 | 84,47 | 99,612 | 116,31 |

Pergeseran antar-aturan:
- I_avg 0,006 mA (0,07 %);
- energi per tanda tangan 0,0003 mJ;
- CPU tidur 0,03 poin;
- latensi 0,06 ms.

Keputusan K5 lawan K6 tidak berubah. Run dengan hilang ≥ 2 sedikit berbeda dari run hilang ≤ 1 (latensi +0,27 ms,
CPU tidur +0,14 poin, I_avg −0,026 mA). Ini wajar, karena kehilangan frame berkaitan dengan tanda tangan lambat.

---

## 7. Keterbatasan baru (untuk 5.12)
1. **Skenario C diambil pada hari berbeda (27 Nov) dari A/B (18 Nov).** Perbandingan C dengan A/B dibaca secara
   deskriptif, dengan pembanding sehari (run B 27 Nov, n = 2/3) dan jembatan uji pendahuluan. Tegangan USB 5,13 V
   (18 Nov 5,22 V), sehingga metrik berbasis arus lebih tepat untuk perbandingan lintas hari.
2. **IDLE skenario C memakai IDLE 18 Nov** (tidak diukur ulang). Daya/energi dinamis C bias ±0,2 mW dan arus
   dinamis ≤ 0,3 %.
3. **Toleransi frame hilang skenario C (< 1 %) lebih longgar dari A/B (< 0,25 %)** dan ditetapkan setelah run awal
   terlihat. Sensitivitas menunjukkan tidak ada pengaruh pada kesimpulan (bagian 6).
4. **Hanya satu titik utilisasi (ρ = 1) yang diukur.** Laju di antara A, C, dan B tetap hasil model.
5. **ρ terukur terkuantisasi:** satu jendela hanya memuat sekitar 90 frame software-based, sehingga TL K5 bernilai
   97,80 % atau 98,89 % bergantung pada apakah frame ke-91 masuk jendela.
6. **Rata-rata jendela memuat jeda sebelum frame pertama** (sekitar 2 periode attacker; juga pada data A/B).
   Metrik tunak disajikan sebagai pelengkap.
7. **Overhead layanan sekitar 6,1 ms pada perangkat lunak terukur sebagai total;** komponennya tidak dipisahkan.
8. Butir 14 v4 diubah: "Penyerang dimodelkan sebagai pengirim periodik pada laju tetap (1 dan 10
   permintaan/detik, serta 1/T_sign per arsitektur); penyerang adaptif tidak diuji."

---

## 8. Keputusan yang menunggu Anda

**K1. Toleransi frame hilang skenario C.**
- Saat meminta keputusan, saya memperkirakan hanya ±1 dari 4 run lolos aturan 0,25 %, sehingga dibutuhkan ±120
  run. **Perkiraan itu terlalu pesimis** karena hanya berdasarkan 4 run pertama. Dari 33 run K2S-C, ternyata 24 run
  lolos aturan 0,25 %; mencapai 30 dengan aturan itu butuh ±41 run, atau sekitar 9 run lagi.
- Pilihan:
  - (a) tetap 1 % (n = 31), diungkap dengan sensitivitas bagian 6. **Usul saya:** hasil kedua aturan praktis
    identik dan keputusan sudah diambil dengan alasan yang tercatat.
  - (b) kembali ke 0,25 % agar satu aturan untuk semua skenario: n = 24, atau tambah ±7 run K2S-C pada hari lain,
    dengan risiko efek hari.

**K2. Posisi skenario C di naskah.**
- (a) **rumusan masalah 4 tanpa hipotesis terarah**, dianalisis deskriptif + K5 lawan K6. **Usul saya:** desain
  faktorial 2 × 2 tetap utuh, dan permintaan penguji terjawab eksplisit.
- (b) dilebur ke RM2 sebagai tingkat beban ketiga. Ini kurang cocok, karena laju C berbeda per arsitektur sehingga
  tidak masuk faktorial.

**K3. Metrik utama skenario C.**
- (a) **metrik jendela (sama dengan A/B) + tunak sebagai pelengkap.** Usul saya: konsisten dengan protokol.
- (b) tunak sebagai utama.

**K4. Koreksi "0,38 %" dan interaksi tunak pada bagian A/B yang sudah ada.**
- **Usul saya: ya**, sebagai catatan dan sensitivitas, bukan mengganti angka utama.

**K5. Gambar baru** (CPU tidur dan arus terhadap ρ; Tabel 5.13 diperluas).
- **Usul saya: satu gambar** "CPU tidur terhadap utilisasi" (enam titik). Gambar ini paling jelas menunjukkan
  lantai tidur DS28E38 dan runtuhnya perangkat lunak di ρ = 1.

**K6. Perbandingan C lawan B.**
- **Usul saya:** pakai pembanding sehari tunak (n kecil, deskriptif). Jangan pakai nilai p K3–K5/K4–K6 lintas hari
  di naskah.

---

## 9. Koreksi atas pernyataan saya selama sesi 27 Nov

| Saya sebut saat sesi | Yang benar |
|---|---|
| Overhead software B ±8 ms dan DS28E38 B ±0,5 ms | Keduanya terbawa jeda awal. Nilai tunaknya ±6,1 ms (software, sama di B dan C) dan ≈ 0 (DS28E38) |
| Arus C sedikit di bawah B | Artefak jeda awal. Pada keadaan tunak, arus C = B (software) dan −0,4 % (DS28E38) |
| Hanya ±1 dari 4 run K2S-C lolos 0,25 % | 24 dari 33 run lolos (K1) |
| ρ nyata K5 0,991 "≈ 1 karena jeda awal" | Benar arahnya. Nilai terukur per blok 0,991 dan 0,980 (kuantisasi frame ke-91) |

---

## 10. Yang tidak berubah (sudah diperiksa)
- **Data A/B 18 Nov, kalibrasi, dan semua angka laporan v4-kalibrasi tidak berubah.**
- **H1–H4 dan semua persentase penghematan v4 tidak berubah:** 82,3 % latensi, 92,6 % energi per permintaan, 58,0 %
  daya dinamis barrage.
- **Keputusan uji 18 Nov dan leave-one-block-out tidak berubah.**
- **Biner host tidak berubah.** Data C memakai biner yang sama dengan 18 Nov.

## Urutan kerja yang disarankan di Word
1. Putuskan K1–K3.
2. Bab 3: D1–D7.
3. Bab 5: F7 (sub-bab baru), lalu F6, F9, F2–F5, F10–F13.
4. Bab 6: G1–G7.
5. Bab 1 B1–B5 dan Intisari A1–A4.
6. Bab 4: E1–E3.
7. Lampiran H.
8. Lembar jawaban revisi (5.5).
