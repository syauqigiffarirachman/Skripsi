# Laporan perubahan SKRIPSI_FULL_Syauqi_v4 (20 Nov) setelah kalibrasi INA226 (21 November 2026)

Dibandingkan: `paper/SKRIPSI_FULL_Syauqi_v4_2026-11-20.pdf` (113 hlm PDF) terhadap data kalibrasi dan analisis ulang 21 November 2026.
Letak ditulis **hal. cetak (PDF hal.)**: halaman cetak = halaman PDF − 13 (Bab 1 mulai hal. 1 = PDF 14). Subbab ditulis dengan nomornya agar mudah dicari di Word.

Sumber angka baru: `data/analisis/stop_final/` (dibuat ulang 21 Nov; salinan hasil lama ada di `data/analisis/stop_final_LAMA_rshunt0.100_offset-2.0/`), `data/kalibrasi/kalibrasi_final_2026-11-21.md`, `data/kalibrasi/catatan_multimeter_2026-11-21.md`, `data/kalibrasi/catatan_pullup_2026-11-21.md`.

---

## 0. Ringkasan

**Parameter kalibrasi final**

| Parameter | Nilai | Dasar |
|---|---|---|
| Hambatan shunt efektif R_eff | **0,0821 Ω** (nominal modul 0,100 Ω; faktor koreksi arus ×1,218) | rerata 6 selisih ΔI = V_bus/R_beban (1 kΩ ×3, 2,2 kΩ ×3) |
| Offset nol | **−2,83 satuan** (2,5 µV/satuan) | awal −2,820, akhir −2,833 (bus 5,23–5,25 V) |
| u_kal | **1,5 %** (menggantikan asumsi 5 %) | metode selisih 1,2 %; pembanding multimeter 0,0833 Ω menyimpang 1,4 % |
| Pull-up 1-Wire PA15–3V3 | **4,66 ± 0,12 kΩ** (dengan atau tanpa DS9121) | ohmmeter CD800a, rentang 40 kΩ |
| Multimeter | SANWA CD800a, DCA 40 mA, resolusi 0,01 mA, akurasi ±(2,2 % + 5 digit) | katalog |

**Aturan konversi cepat** (untuk angka lama yang diukur dengan shunt 0,100 Ω dan offset −2,0, termasuk uji pengembangan Bab 4):
- arus mutlak: **I_baru = 1,218 × I_lama + 0,025 mA**;
- selisih arus, daya dinamis, energi dinamis, muatan dinamis: **× 1,218** (offset terhapus; daya dan energi dihitung langsung dari data, bukan dari konversi);
- persentase penghematan, latensi, durasi tidur, tingkat layanan, throughput, nilai p: **tidak berubah**.

**Yang berubah**: semua nilai mutlak mA/mW/mJ/mC (≈ ×1,218), ketidakpastian mutlak (5 % → 1,5 %), lantai arus diam (1,99 → ≈ 2,45 mA), proyeksi umur baterai, tabel kalibrasi/penyimpangan yang masih placeholder, kalimat "kalibrasi belum dilakukan" di lima tempat, nilai pull-up.
**Yang tidak berubah**: kesimpulan, semua persentase penghematan (92,6 %, 58,0 %, 82,3 %), IK 95 % persentase, uji hipotesis, Intisari/Abstract (tidak memuat angka mutlak).

Ringkasan jumlah butir: Bab 1 (1: A1), Bab 3 (15: B1–B15), Bab 4 (9: C1–C9), Bab 5 (31: D1–D31, ditambah tabel pengganti D4b, D17, D21), Bab 6 (5: E1–E5), Lampiran, Pustaka, Lambang (5: F1–F5), gambar (6 diganti + 1 opsional), temuan tambahan di luar kalibrasi (6: J1–J6).

---

## A. Bab 1

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| A1 | 1.3 butir 7 "Lingkup sistem dan catu daya", hal. 5 (PDF 18), baris 3–4 | "arus diam papan sekitar 1,99 mA" | "arus diam papan sekitar 2,45 mA" (kalimat selanjutnya "≈14 µA tipikal" tetap) |

---

## B. Bab 3

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| B1 | **Tabel 3.1**, baris 3 "Kalibrasi dan uji pendahuluan", kolom indikator, hal. 23 (PDF 36) | "Bias dan batas kesesuaian INA226 terhadap multimeter diperoleh; latensi uji pendahuluan (~654,9 ms dan ~116,2 ms) menetapkan laju uji" | "Hambatan shunt efektif dan offset INA226 diperoleh dengan metode selisih dan dibandingkan dengan multimeter (u_kal 1,5 %); latensi uji pendahuluan (~654,9 ms dan ~116,2 ms) menetapkan laju uji" |
| B2 | **3.2.2 butir 3** variabel kontrol, hal. 24 (PDF 37) | "antarmuka 1-Wire pull-up 4,7 kΩ" | "pull-up 1-Wire 4,66 ± 0,12 kΩ (terukur)" |
| B3 | **Tabel 3.3 baris 1** (papan host), hal. 27 (PDF 40) | "jalur 1-Wire PA15 dengan pull-up dalam 4,7 kΩ" | "… pull-up 4,66 ± 0,12 kΩ (terukur, tanpa dan dengan papan DS9121 terpasang)" |
| B4 | **Tabel 3.3 baris 5** (INA226), hal. 27 | "shunt bawaan berkode R100 (0,100 Ω) dipasang high-side pada VBUS" | "shunt bawaan modul berkode R100 dipasang high-side pada VBUS; hambatan efektif terukur 0,0821 Ω (Sub-bab 3.4.10), bukan 0,100 Ω" |
| B5 | **Tabel 3.3 baris 7** (multimeter), hal. 27 | "Arus DC rentang mA dan tegangan DC rentang mV" / "Kalibrasi dan validasi arus INA226" | "SANWA CD800a, 4000 hitungan; arus DC rentang 40 mA (resolusi 0,01 mA, akurasi ±(2,2 % + 5 digit)); tegangan DC ±(0,7 % + 3 digit); resistansi ±(1,5 % + 5 digit) (SANWA, t.t.)". Katalog hanya mencantumkan "akurasi terbaik" per fungsi, bukan per rentang; periksa manual lengkap bahwa rentang 40 mA memakai nilai itu / "Pembanding independen kalibrasi arus INA226 dan pengukur pull-up" |
| B6 | **Tabel 3.3 baris 8** (peralatan pendukung), hal. 27 | "… resistor seri, kabel penghubung" | tambah "resistor beban kalibrasi 1 kΩ dan 2,2 kΩ (toleransi 1 %, 1/4 W)" |
| B7 | **3.3.2**, hal. 29 (PDF 42), kalimat "Pada mode tidur dalam, arus diam papan…" | "terukur sekitar 1,99 mA (sekitar 10,4 mW pada 5,2 V) … Nilai ini sama pada kedua arsitektur (selisih < 0,003 mA)" | "terukur sekitar 2,45 mA (sekitar 12,8 mW pada 5,22 V) … (selisih ≈ 0,003 mA)" |
| B8 | **Tabel 3.5 butir 7** (Host PA15 – DS28E38), hal. 30 (PDF 43) | "pull-up 4,7 kΩ dan strong pull-up" | "pull-up 4,66 kΩ dan strong pull-up" |
| B9 | **3.3.3 b**, hal. 32 (PDF 45), kalimat pertama | "pull-up diam 4,7 kΩ ke 3,3 V" | "pull-up diam 4,66 ± 0,12 kΩ ke 3,3 V (terukur)". Opsional tambah: "Pad VCC/CEXT pada papan DS9121 hanya satu pin dan tidak dipakai; DS28E38 beroperasi parasit melalui IO dan GND." |
| B10 | **3.3.3 d**, hal. 32 (PDF 45), kalimat debugger | "dari ±1,94 mA menjadi ±2,52 mA" | "dari ±2,39 mA menjadi ±3,09 mA" |
| B11 | **3.4.3 butir 4**, hal. 37 (PDF 50) | "run IDLE pembuka (arus diam ±1,99 mA)" | "(arus diam ±2,45 mA)" |
| B12 | **3.4.5**, hal. 38 (PDF 51–52), setelah Persamaan 3.3 | (belum ada rumus konversi cuplikan mentah) | Tambah kalimat dan persamaan: "Arus tiap cuplikan dihitung dari pembacaan shunt mentah X_k: I_k = (X_k − X_0) · 2,5 µV / R_eff (3.20), dengan X_0 = −2,83 satuan (offset nol) dan R_eff = 0,0821 Ω (Sub-bab 3.4.10)." |
| B13 | **3.4.8** pasangan (3), hal. 41–42 (PDF 54–55) | "(3) arus rata-rata INA226 terhadap multimeter pada beban tetap" | "(3) arus rata-rata INA226 terhadap multimeter SANWA CD800a pada tiga beban tetap (tanpa tambahan, +2,2 kΩ, +1 kΩ); karena hanya tiga pasangan, hasilnya dilaporkan sebagai bias dan simpangan, bukan batas kesesuaian yang dapat digeneralisasi" |
| B14 | **3.4.9**, hal. 42 (PDF 55), kalimat "Bila kalibrasi shunt terhadap multimeter belum tersedia, u_kal ditetapkan sebesar 5 %…" | seluruh kalimat | ganti: "u_kal ditetapkan 1,5 % berdasarkan kalibrasi pada Sub-bab 3.4.10 (selisih metode selisih dan pembanding multimeter 1,4 %), dan dinyatakan pada tabel hasil." Pada kalimat berikutnya "u_kal saling menghapus … shunt yang sama" tetap benar untuk faktor gain. |
| B15 | **BARU: Sub-bab 3.4.10 Kalibrasi INA226** (di akhir Bab 3, sesudah 3.4.9, hal. 42) | – | Tambah subbab (draf di bagian G). |

---

## C. Bab 4

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| C1 | **4.2.1**, hal. 44 (PDF 57), paragraf 1 | "resistor pull-up 4,7 kΩ ke 3,3 V yang terpasang tetap" | "resistor pull-up (terukur 4,66 ± 0,12 kΩ) ke 3,3 V" |
| C2 | **4.2.1**, hal. 44 (PDF 57), paragraf 2 | "LED inilah yang mendominasi arus diam papan sekitar 1,99 mA" | "… sekitar 2,45 mA" |
| C3 | **4.2.2**, hal. 44 (PDF 57), "modul INA226 berkode shunt R100" | – | tambah "(hambatan efektif terukur 0,0821 Ω; Sub-bab 3.4.10)" |
| C4 | **4.2.3**, hal. 45 (PDF 58), "shunt modul yang nilai efektifnya diragukan diganti dengan resistor shunt 1 %" | "diragukan" | "shunt modul yang nilai efektifnya terukur 0,0821 Ω, bukan 0,100 Ω, diganti dengan resistor shunt 1 % yang nilainya diketahui". Catat juga: dengan shunt 0,1 Ω ± 1 % pada PCB, kalibrasi perlu diulang. |
| C5 | **4.6**, hal. 53 (PDF 66), kalimat terakhir paragraf 2 | "Nilai hambatan shunt menjadi parameter skrip sehingga nilai mutlak dapat dihitung ulang setelah kalibrasi tanpa mengulang pengukuran." | "Seluruh nilai mutlak dalam Bab 5 dihitung ulang dari data mentah dengan R_eff = 0,0821 Ω dan offset −2,83 satuan hasil kalibrasi (Sub-bab 3.4.10); skrip kalibrasi_shunt.py menyimpan data kalibrasi di data/kalibrasi/." |
| C6 | **4.6**, hal. 53, nama berkas salah (butir lama K di laporan 20 Nov, belum diperbaiki) | "ujigalat_stop.py", "dsod.py" | "uji_galat_stop.py", "ds_od.py" |
| C7 | **Tabel 4.5**, hal. 55 (PDF 68), baris S0 dan S1 | "1,985 mA (14.922 cuplikan) tanpa ST-Link; 2,567 mA dengan ST-Link"; "arus tidur 1,988 mA" | "2,443 mA …; 3,152 mA …"; "arus tidur 2,447 mA". Tambah catatan tabel: "Nilai dikonversi dari pengukuran pengembangan (R = 0,100 Ω, offset −2,0) dengan I = 1,218·I_lama + 0,025 mA." |
| C8 | **Tabel 4.6** baris "Arus IDLE dalam keadaan bersih", hal. 55 (PDF 68) | "1,988 mA" / "1,991 mA" | "2,447 mA" / "2,450 mA" |
| C9 | **Tabel 4.7 butir 12**, hal. 57 (PDF 70) | "Arus diam berubah dari ±1,94 mA menjadi ±2,52 mA" | "dari ±2,39 mA menjadi ±3,09 mA" |

---

## D. Bab 5

### D-1. Pengantar dan kalibrasi

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| D1 | **Paragraf pembuka Bab 5**, hal. 58 (PDF 71), kalimat 3–5 | "Nilai mutlak arus, daya, dan energi masih dihitung dengan hambatan shunt nominal modul INA226 (0,100 Ω) karena kalibrasi shunt terhadap acuan presisi belum dilakukan (Sub-bab 5.2.4). Kesalahan kalibrasi bersifat faktor pengali yang sama bagi semua kelompok, …" | "Nilai mutlak arus, daya, dan energi dihitung dengan hambatan shunt efektif 0,0821 Ω dan offset −2,83 satuan hasil kalibrasi pada 21 November 2026 (Sub-bab 5.2.2), dengan ketidakpastian kalibrasi 1,5 %; dengan shunt nominal 0,100 Ω nilai arus akan terbaca ±17 % terlalu kecil. Kesalahan kalibrasi bersifat faktor pengali yang sama bagi semua kelompok, sehingga persentase penghematan tidak terpengaruh, …" (sisa kalimat tetap) |
| D2 | **Tabel 5.3**, baris "Kalibrasi shunt dan Bland–Altman INA226 vs multimeter (Bab 3)", hal. 60 (PDF 73) | kolom pelaksanaan "Dilakukan setelah pengambilan data [diisi setelah pengukuran]"; dampak "Sebelum hasil tersedia, ukal diasumsikan 5 %; nilai mutlak sementara" | pelaksanaan: "Dilakukan 21 Nov 2026 (tiga hari setelah data, 18 Nov) pada beban 12–21 mA; metode selisih dengan resistor 1 kΩ dan 2,2 kΩ, pembanding multimeter pada tiga beban; kelompok arus Stop (±2 mA) tidak dikalibrasi"; dampak: "R_eff = 0,0821 Ω, u_kal = 1,5 %; seluruh nilai mutlak dihitung ulang; linearitas hingga ±2 mA diasumsikan" |
| D3 | **Tabel 5.3**, tambah baris (opsional) | – | "Pull-up 1-Wire (Bab 3: dugaan 4,7 kΩ) \| terukur 4,66 ± 0,12 kΩ \| nilai terukur menggantikan dugaan" |
| D4 | **5.2.2**, hal. 63 (PDF 76), **Tabel 5.4** baris terakhir dan paragraf sesudahnya | baris "INA226 − multimeter (arus, beban tetap) […] […] […] [diisi setelah pengukuran]"; paragraf "Kesesuaian arus INA226 terhadap multimeter … [diisi setelah pengukuran; …]" | **Hapus baris itu dari Tabel 5.4** (kolomnya ms, bukan mA) dan ganti paragraf dengan teks D4a di bawah, ditambah **Tabel baru (5.5)** "Kalibrasi INA226 terhadap multimeter" (D4b). Tabel lain akan bergeser nomornya otomatis (Update Field). |

**D4a. Teks pengganti paragraf (taruh sesudah Tabel 5.4 di 5.2.2):**

> Hambatan shunt efektif modul INA226 ditentukan dengan metode selisih (Sub-bab 3.4.10) dan diperiksa terhadap multimeter pada kelompok arus terjaga (12–21 mA). Hasil enam selisih (resistor 1 kΩ dan 2,2 kΩ, masing-masing tiga putaran) memberi R_eff = 0,0821 Ω, dengan sebaran antar-putaran 0,25 % untuk 1 kΩ (0,0825 Ω) dan 2,30 % untuk 2,2 kΩ (0,0817 Ω). Kriteria sebaran < 1 % per resistor hanya terpenuhi oleh 1 kΩ; sebaran 2,2 kΩ lebih besar karena selisih pembacaan hanya sekitar 78 satuan sehingga galat ±1 satuan setara ±1,3 %, sejalan dengan simpangan antar-baseline sebesar 0,9 satuan. Selisih antar-resistor 0,9 % memenuhi kriteria 2 %. Regresi arus absolut terhadap multimeter melalui titik nol memberi R_eff = 0,0833 Ω (residu ≤ 0,3 %), berbeda 1,4 % dari hasil metode selisih. Offset nol diukur −2,820 satuan (awal) dan −2,833 satuan (akhir), selisih 0,013 satuan. Dengan R_eff dan offset tersebut, INA226 membaca 1,1–1,6 % lebih tinggi daripada multimeter (Tabel 5.5), konsisten dengan ketidakpastian multimeter (±2,2 % + 5 digit). Pembacaan memakai hambatan nominal 0,100 Ω akan 17 % terlalu rendah pada ketiga beban.

**D4b. Tabel baru "Kalibrasi INA226 terhadap multimeter SANWA CD800a (rentang 40 mA, rerata tiga pembacaan)":**

| Keadaan beban | Multimeter (mA) | INA226 @ 0,100 Ω (mA) | INA226 @ 0,0821 Ω (mA) | Selisih INA226 − multimeter (0,0821 Ω) |
|---|---|---|---|---|
| Tanpa beban tambahan | 15,38 | 12,77 (−17,0 %) | 15,56 | +0,18 mA (+1,1 %) |
| + 2,2 kΩ | 17,68 | 14,72 (−16,7 %) | 17,93 | +0,25 mA (+1,4 %) |
| + 1 kΩ | 20,51 | 17,10 (−16,6 %) | 20,83 | +0,32 mA (+1,6 %) |

Catatan di bawah tabel: bias rerata +0,25 mA, simpangan 0,07 mA, rentang indikatif +0,11 s.d. +0,39 mA (n = 3, hanya indikatif). Ketidakpastian multimeter ±(2,2 % + 0,05 mA).

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| D5 | **5.2.3**, hal. 63 (PDF 76), paragraf 1 | "Arus diam berada pada 1,984–1,992 mA dengan pergeseran terbesar 0,004 mA, jauh di bawah batas 0,05 mA" | "2,442–2,451 mA … 0,005 mA" (blok 4: 2,448 → 2,442 mA = 0,0054) |
| D6 | 5.2.3, hal. 63–64, kalimat berikutnya | "sama dengan arus Stop host yang berdiri sendiri pada uji dasar (1,985 mA)" | "(2,443 mA)" |
| D7 | **5.2.2**, hal. 63 (PDF 76), kalimat model duty cycle | "selisih maksimum 0,008 mA" | "selisih maksimum 0,010 mA" |
| D8 | **5.2.3**, pencilan, hal. 64 (PDF 77) | "satu run dengan arus dinamis 11,733 mA dan muatan 7,787 mC" | "arus dinamis 14,291 mA dan muatan 9,485 mC" (penyimpangan < 0,1 % terhadap median tetap benar) |
| D9 | **Tabel 5.5** (LOBO), hal. 64 | ΔP normal 92,60–92,64; ΔP barrage 57,95–58,02; ΔT normal 82,24–82,25; ΔT barrage 82,24–82,25; ΔS normal 63,08–63,09; ΔS barrage 83,57–83,61 | dari `leave_one_block_out.csv` baru: ΔP normal 92,60–92,64; ΔP barrage **57,99–58,01**; ΔT normal **82,25**; ΔT barrage 82,24–82,25; ΔS normal **63,08**; ΔS barrage 83,57–83,61. Perubahan kecil (≤ 0,04 poin) dan tidak mengubah kesimpulan; periksa angka lama di naskah terhadap CSV bila ragu |
| D10 | **5.2.4**, hal. 64–65 (PDF 77–78) | "didominasi asumsi ketidakpastian kalibrasi shunt 5 % karena kalibrasi terhadap multimeter belum dilakukan"; "Rasio terkecil … adalah 20,0" | "didominasi ketidakpastian kalibrasi shunt 1,5 % (Sub-bab 5.2.2)"; "Rasio terkecil … adalah 66,6" |
| D11 | **Tabel 5.6**, hal. 65 (PDF 78) | Pdyn/Ereq "5,0–5,1 %", Idyn/Qreq "5,0–5,1 %", komponen "u_kal (asumsi 5 %)" | Pdyn/Ereq **1,5–1,8 %**; Idyn/Qreq **1,5–1,7 %**; komponen dominan "u_kal (1,5 %) dan u_idle pada K2/K4". Latensi 0,01–0,04 % tetap |
| D12 | **Tabel 5.8** (sensitivitas ambang), hal. 66 (PDF 79) | kolom daya dinamis 25,70 / 25,69 / 25,69 / 25,68; energi per layanan 3,0000 / 3,0000 / 3,0000 / 3,0002 | daya dinamis **31,31 / 31,30 / 31,29 / 31,28 mW**; energi per layanan **3,6536 / 3,6541 / 3,6540 / 3,6543 mJ**; kolom lain (latensi, durasi tidur, TL) tetap |
| D13 | **5.2.6**, paragraf sesudah Tabel 5.8, hal. 66 | "0,025 mW pada daya dinamis (0,10 %)" | "0,030 mW" (persentase 0,10 % tetap). Kalimat K3 vs K4 (58,00 %/58,02 %, 92,61 %, p) tidak berubah |

### D-2. Hasil

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| D14 | **5.3**, caption **Gambar 5.4**, hal. 67 (PDF 80) | "garis putus-putus: arus diam 1,99 mA" | "… 2,45 mA" |
| D15 | 5.3, paragraf sesudah Gambar 5.4, hal. 67 (PDF 80) | "arus papan sekitar 5–6 mA karena host menyuplai arus komputasi DS28E38" | "sekitar 6–7 mA" |
| D16 | 5.3, hal. 68 (PDF 81), kalimat sebelum Tabel 5.9 | "Daya diam papan pada kedua arsitektur adalah 10,4 mW (1,99 mA)." | "12,8 mW (2,45 mA)." |
| D17 | **Tabel 5.9**, hal. 68 (PDF 81) | seluruh nilai mutlak | lihat tabel D17 di bawah |
| D18 | **5.6.1**, hal. 70 (PDF 83) | "turun dari 40,10 mW (software-based) menjadi 2,96 mW"; "turun dari 61,18 mW menjadi 25,70 mW" | "48,85 mW … 3,60 mW"; "74,52 mW … 31,30 mW". Persentase 92,62 % [92,61; 92,64], 58,00 % [57,97; 58,02], 92,64 %, 57,99 % **tetap** |
| D19 | **5.6.2**, hal. 71 (PDF 84) | "41,03 mJ (normal) dan 40,59 mJ (barrage) … 3,01 mJ dan 3,00 mJ"; "(7,87 dan 7,78 mC terhadap 0,575 mC)" | "49,98 mJ … 49,44 mJ … 3,67 mJ dan 3,65 mJ"; "(9,58 dan 9,48 mC terhadap 0,70 mC)". 92,67 % (13,6 kali) dan 92,61 % (13,5 kali) tetap |
| D20 | **5.6.3**, hal. 72 (PDF 85) | "daya diam papan 10,4 mW … 50,50 mW menjadi 13,33 mW (hemat 73,61 %) … 71,57 mW menjadi 36,06 mW (hemat 49,62 %)" | "12,8 mW … 61,64 mW menjadi 16,36 mW (hemat 73,45 %) … 87,31 mW menjadi 44,06 mW (hemat 49,54 %)" (dua persentase bergeser 0,1 % karena offset) |
| D21 | **Tabel 5.10** dan teks 5.6.4, hal. 72–73 (PDF 85–86) | lihat tabel D21 di bawah | lihat di bawah |
| D22 | **Tabel 5.11**, hal. 73 (PDF 86) | "Pdyn 40,10 → 61,18 mW (+52,55 %) \| 2,96 → 25,70 mW (+768 %)"; "Ereq 41,03 → 40,59 mJ (−1,1 %) \| 3,01 → 3,00 mJ (−0,3 %)" | "48,85 → 74,52 mW (+52,55 %) \| 3,60 → 31,30 mW (+768 %)"; "49,98 → 49,44 mJ (−1,1 %) \| 3,67 → 3,65 mJ (−0,3 %)" |
| D23 | **5.8**, teks, hal. 74 (PDF 87) | "keunggulan daya dinamis mengecil sebesar 1,66 mW" | "2,02 mW" |
| D24 | **Tabel 5.12**, hal. 75 (PDF 88) | Pdyn 37,14 / 35,48 / −1,66 [−1,69; −1,64]; Idyn 7,12 / 6,80 / −0,32; Ereq 38,02 / 37,59 / −0,43 [−0,45; −0,41] | Pdyn **45,24 / 43,22 / −2,02 [−2,06; −1,99]**; Idyn **8,68 / 8,28 / −0,39 [−0,40; −0,39]**; Ereq **46,31 / 45,79 / −0,52 [−0,55; −0,50]**. Baris S_cpu, latensi, TL tidak berubah |
| D25 | **Tabel 5.14**, hal. 77 (PDF 90), kolom Welch t baris Pdyn | normal "4.410 (39,4)"; barrage "3.067 (53,4)" | "4.403 (39,4)"; "3.063 (53,4)". Selisih, IK, p (< 10⁻¹⁰⁰), "7 × 10⁻¹⁰³" **tetap** |
| D26 | **5.11.1**, hal. 79 (PDF 92), paragraf 2 | "sekitar 3–4 mA di atas arus diam … arus rata-rata saat CPU tidur pada K4 (5,63 mA) jauh di atas arus diam (1,99 mA)" | "sekitar 4–5 mA … (6,89 mA) … (2,45 mA)". Kalimat "lebih kecil dari arus strong pull-up maksimum sekitar 10 mA" tetap benar |
| D27 | **5.11.8**, hal. 83 (PDF 96) | "LED daya saja sudah menarik sekitar 1,99 mA"; "1,984–1,992 mA … pergeseran terbesar 0,004 mA"; "satu sesi debugger … sekitar 0,58 mA … melebihi arus dinamis K2 (0,566 mA)" | "2,45 mA"; "2,442–2,451 mA … 0,005 mA"; "sekitar 0,71 mA … melebihi arus dinamis K2 (0,689 mA)" (pernyataan tetap benar) |

**Tabel D17 (pengganti isi Tabel 5.9, rerata dan sd, n = 30):**

| Metrik | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| Arus dinamis Idyn (mA) | 9,37 (0,004) | 0,689 (0,004) | 14,28 (0,003) | 6,00 (0,009) |
| Muatan per permintaan Qreq (mC) | 9,58 (0,004) | 0,701 (0,004) | 9,48 (0,003) | 0,701 (0,0008) |
| Daya rata-rata Pavg (mW) | 61,64 (0,05) | 16,36 (0,02) | 87,31 (0,06) | 44,06 (0,05) |
| Arus rata-rata Iavg (mA) | 11,82 (0,004) | 3,136 (0,004) | 16,73 (0,004) | 8,447 (0,010) |
| Energi jendela 60 s (mJ) | 3.721 (3,2) | 981,8 (1,2) | 5.272 (3,9) | 2.647 (3,3) |
| Arus saat CPU tidur (mA) | 2,51 | 2,92 | 2,56 | 6,89 |
| Tegangan bus (V) | 5,219 | 5,218 | 5,218 | 5,216 (tetap) |

**Tabel D21 (pengganti Tabel 5.10 dekomposisi energi, kondisi normal):**

| Kelompok | Fase | Durasi (ms/tanda tangan) | Arus rerata (mA) | Energi dinamis (mJ) | Porsi |
|---|---|---|---|---|---|
| K1 | CPU aktif (hitung perangkat lunak) | 664,1 | 16,86 | 49,91 | 99,9 % |
| K1 | Tidur di luar episode | 359,0 | 2,49 | 0,064 | 0,1 % |
| K2 | CPU aktif (kirim perintah dan baca hasil) | 18,6 | 15,87 | 1,30 | 35,5 % |
| K2 | IC menghitung (CPU tidur) | 100,9 | 6,80 | 2,29 | 62,4 % |
| K2 | Tidur di luar episode | 897,5 | 2,46 | 0,075 | 2,0 % |

Jumlah per kelompok: K1 49,98 mJ, K2 3,67 mJ (sama dengan energi per permintaan utama, selisih 0,00 %). Teks 5.6.4:
- "selama sekitar 664 ms pada 13,8 mA" → "pada 16,9 mA";
- "arus papan 5,56 mA atau sekitar 3,6 mA di atas arus diam" → "6,80 mA atau sekitar 4,3 mA di atas arus diam";
- "Angka 3,6 mA adalah batas atas bagi IC" → "Angka 4,3 mA …";
- "Nilai mutlak mJ mengikuti kalibrasi shunt (Sub-bab 5.2.4)" → "(Sub-bab 5.2.2; u_kal 1,5 %)";
- "62 %", "36 kali", "13,6 kali", "2,5 kali lebih rendah" (16,86/6,80 = 2,5) **tetap**.

### D-3. Umur baterai dan keterbatasan

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| D28 | **Tabel 5.17**, hal. 84 (PDF 97) | K1 41,03 / 325 / 9,68 / 103 / 130; K2 3,01 / 4.426 / 2,55 / 392 / 1.767; K3 40,59 / 328 / 13,72 / 73 / 85; K4 3,00 / 4.440 / 6,91 / 145 / 203 | K1 **49,98 / 267 / 11,82 / 85 / 107**; K2 **3,67 / 3.634 / 3,14 / 319 / 1.451**; K3 **49,44 / 269 / 16,73 / 60 / 70**; K4 **3,65 / 3.645 / 8,45 / 118 / 167** (tingkat layanan tetap) |
| D29 | **5.11.9**, paragraf sesudah Tabel 5.17, hal. 84 | "103 menjadi sekitar 392 jam (3,8 kali) dengan lantai papan 1,99 mA"; "umur 29 % … (103 menjadi 73 jam)"; "63 % pada DS28E38 (392 menjadi 145 jam)"; "Lantai papan … 1,99 mA" | "85 menjadi sekitar 319 jam (3,8 kali) dengan lantai papan 2,45 mA"; "29 % … (85 menjadi 60 jam)"; "63 % … (319 menjadi 118 jam)"; "Lantai papan … 2,45 mA". "13,6 kali lebih banyak tanda tangan per baterai" tetap. Kalimat "Nilai mutlak proyeksi mengikuti kalibrasi shunt (Sub-bab 5.2.4)" → "(Sub-bab 5.2.2)". Asumsi baterai 1.000 mAh, 3,7 V tetap |
| D30 | **5.12 butir 1**, hal. 84 (PDF 97) | "Kalibrasi shunt INA226 terhadap acuan presisi dan pembanding multimeter belum dilakukan, sehingga nilai mutlak arus, daya, dan energi masih sementara. Persentase penghematan tidak terpengaruh." | "Kalibrasi shunt INA226 dilakukan pada arus 12–21 mA (kelompok terjaga), tiga hari setelah pengambilan data; linearitas hingga arus Stop (±2 mA) diasumsikan, dan stabilitas shunt antara 18 dan 21 November tidak diperiksa. Sebaran R_eff untuk resistor 2,2 kΩ (2,3 %) tidak memenuhi kriteria 1 %; pembanding multimeter hanya tiga titik. Ketidakpastian kalibrasi 1,5 %. Persentase penghematan tidak terpengaruh." |
| D31 | **5.12 butir 5**, hal. 85 (PDF 98) | "Arus diam papan sekitar 1,99 mA" | "sekitar 2,45 mA" |

---

## E. Bab 6

| # | Letak | Tertulis di v4 | Ganti menjadi |
|---|---|---|---|
| E1 | **6.1** paragraf RM1, hal. 87 (PDF 100) | "turun dari sekitar 41 mJ menjadi sekitar 3,0 mJ" | "turun dari sekitar 50 mJ menjadi sekitar 3,7 mJ" |
| E2 | 6.1 paragraf RM1, kalimat berikutnya | "Nilai mutlak energi masih menunggu kalibrasi shunt, tetapi persentase penghematannya tidak terpengaruh." | "Nilai mutlak energi dihitung dengan shunt terkalibrasi (0,0821 Ω; ketidakpastian 1,5 %), dan persentase penghematannya tidak terpengaruh kalibrasi." |
| E3 | 6.1, paragraf prosedur, hal. 88 (PDF 101) | "menjaga arus diam stabil pada 1,984–1,992 mA di seluruh blok" | "2,442–2,451 mA" |
| E4 | 6.1, paragraf kekurangan, hal. 88 | "yaitu kalibrasi shunt yang belum dilakukan, pengujian hanya pada satu papan…" | "yaitu kalibrasi shunt yang hanya mencakup arus 12–21 mA, pengujian hanya pada satu papan…" |
| E5 | **6.2 saran 1**, hal. 88 (PDF 101) | "Melakukan kalibrasi shunt INA226 terhadap acuan presisi dan analisis Bland–Altman terhadap multimeter terkalibrasi, lalu menghitung ulang nilai mutlak arus, daya, dan energi dari data mentah yang telah disimpan." | "Memperluas kalibrasi shunt INA226 ke arus rendah (±2 mA, keadaan Stop) dengan multimeter beresolusi lebih halus atau resistor shunt presisi, memeriksa kestabilan shunt antar-hari, dan menghitung ulang nilai mutlak dari data mentah yang telah disimpan." |

---

## F. Lampiran, Daftar Pustaka, Daftar Lambang

| # | Letak | Perubahan |
|---|---|---|
| F1 | **Lampiran B, Tabel B.1**, hal. 96 (PDF 109): judul "Kronologi Kegiatan 10–18 November 2026" dan baris 18 Nov malam, kolom status "Data akhir memenuhi standar; kalibrasi shunt tersisa" | judul → "10–21 November 2026"; status → "Data akhir memenuhi standar"; **tambah baris** "21 Nov \| Kalibrasi INA226 (offset, metode selisih, pembanding multimeter), pengukuran pull-up, analisis ulang nilai mutlak \| R_eff 0,0821 Ω; u_kal 1,5 %" |
| F2 | **Lampiran D, Tabel D.1**, baris `tools/kalibrasi_shunt.py`, hal. 98 (PDF 111) | deskripsi → "Perekaman dan perhitungan kalibrasi shunt INA226 (offset, metode selisih); data di data/kalibrasi/" |
| F3 | **Daftar Pustaka**, hal. 90+ | tambah: SANWA Electric Instrument Co., Ltd. (t.t.). *CD800a Digital Multimeter* [katalog]. Tokyo. (tahun terbit pada katalog tidak tercantum; isi bila ada; dirujuk dari Tabel 3.3 dan Sub-bab 3.4.10) |
| F4 | **Daftar Lambang**, hal. vii–viii (PDF 8–9) | tambah "R_eff \| Hambatan shunt efektif INA226 \| Ω" dan "X_0 \| Offset nol pembacaan shunt \| satuan (2,5 µV)" |
| F5 | **Lampiran C** teks pengantar, hal. 96 (PDF 109) | tidak wajib diubah; bila ingin konsisten, tambah satu kalimat: "Resistor shunt R4 pada PCB logger 0,1 Ω 1 % menggantikan shunt modul yang terukur 0,0821 Ω; kalibrasi perlu diulang setelah fabrikasi." |

---

## G. Draf teks baru

### G1. Sub-bab 3.4.10 Kalibrasi INA226 (tambahkan di akhir Bab 3, sesudah 3.4.9)

> Hambatan shunt modul INA226 yang berkode R100 belum terbukti bernilai 0,100 Ω, padahal seluruh nilai mutlak arus, daya, dan energi bergantung padanya. Kalibrasi dilakukan pada 21 November 2026, setelah pengambilan data utama (18 November), dengan host berisi firmware hardware offloading dalam keadaan bersih (daya dimatikan penuh, tanpa sesi debugger) dan terjaga di luar jendela pengamatan, sehingga arus papan sekitar 15 mA.
>
> Metode yang dipakai adalah metode selisih. Resistor beban R_b (1 kΩ dan 2,2 kΩ, toleransi 1 %, 1/4 W) dipasang antara terminal IN− sensor dan GND, sehingga arus tambahan ΔI = V_bus / R_b melewati shunt dan terukur oleh INA226. Tegangan V_bus dibaca INA226 sendiri pada sisi beban, dan nilai R_b diambil dari nilai nominal karena ketelitian nominal 1 % lebih baik daripada pengukuran resistansi dengan multimeter yang tersedia (±1,5 % + 5 digit). Setiap resistor diapit dua rekaman baseline tanpa resistor (urutan B–R–B) agar drift terhapus, dan setiap rekaman berdurasi 30 detik dengan jeda 15 detik sesudah perubahan beban. Hambatan efektif dihitung sebagai
>
> R_eff = ΔX · 2,5 µV / ΔI (3.20)
>
> dengan ΔX selisih rerata pembacaan shunt (satuan 2,5 µV) antara rekaman beban dan baseline. Karena ΔX adalah selisih, offset terhapus. Offset nol X_0 diukur terpisah pada beban nol (IN− dilepas, V_bus tetap ada) di awal dan di akhir sesi, 60 detik tiap pengukuran. Arus tiap cuplikan kemudian dihitung dengan Persamaan 3.20 (I_k = (X_k − X_0) · 2,5 µV / R_eff).
>
> Sebagai pemeriksaan independen, arus absolut pada tiga beban (tanpa resistor, +2,2 kΩ, +1 kΩ) dibandingkan dengan multimeter SANWA CD800a rentang 40 mA (resolusi 0,01 mA; akurasi ±(2,2 % + 5 digit) menurut katalog), dengan rerata tiga pembacaan. Kriteria penerimaan yang ditetapkan sebelum pengukuran: (1) sebaran R_eff antar-putaran untuk setiap resistor kurang dari 1 %; (2) selisih R_eff antar-resistor kurang dari 2 %; (3) selisih R_eff antara arus terjaga dan arus Stop kurang dari 2 %. Ketidakpastian kalibrasi u_kal dihitung dari toleransi resistor (1,0 %), ketidakpastian V_bus INA226 (0,5 %, asumsi konservatif), dan sebaran antar-putaran, lalu ditetapkan 1,5 % agar mencakup selisih terhadap pembanding multimeter. Kriteria 3 tidak dinilai karena kelompok arus Stop tidak dikalibrasi (Sub-bab 5.12).

### G2. Pengganti kalimat di Bab 5 pembuka (D1), sudah tertulis di tabel D di atas.

---

## H. Gambar yang harus diganti atau dibuat ulang

Berkas sumber: `data/analisis/stop_final/gambar_bab4/` (dibuat ulang 21 Nov dengan shunt 0,0821 Ω; ukuran dan gaya sama) dan `data/analisis/stop_final/gambar/`. Gambar 5.x di naskah berurutan berbeda dengan nama berkas 4.x.

**Pemeriksaan visual 21 Nov (saya melihat gambarnya).** Regenerasi pertama ternyata cacat karena skrip gambar mengunci sumbu dan teks untuk skala nominal; sudah diperbaiki di `tools/gambar_bab4_stop.py` dan semua gambar di bawah dibuat ulang sesudahnya:
- Gambar 5.3 (arus diam): batas sumbu tetap 1,93–2,05 mA akan membuat data 2,44–2,45 mA di luar bingkai. Kini sumbu mengikuti rerata IDLE.
- Gambar 5.4/5.5 (jejak arus): batas atas tetap 21 mA memotong puncak K2/K4 (23,8 mA terkalibrasi). Kini 26 mA (mengikuti puncak data); offset pada jejak sebelumnya masih −2,0 tertanam, kini −2,83 sama dengan analisis.
- Gambar 5.8/5.9 (daya dan energi): batas atas tetap 78 mW dan 50 mJ (batang K1 energi 49,98 + galat melewati 50) dan teks "kalibrasi shunt ±5 %" tercetak di gambar; kini sumbu mengikuti data dan teks "±1,5 %".
- Gambar 5.10 (interaksi): batas daya dinamis tetap 74 mW (K3 = 74,52) kini mengikuti data.
Gambar 5.1, 5.2, 5.6, 5.7, 5.11, 5.12 tidak berubah.

| Gambar naskah | Berkas pengganti | Alasan |
|---|---|---|
| **5.3** (arus diam awal/akhir blok), hal. 64 | `gambar_bab4/gambar_4_9_arus_diam_antar_blok.png` | sumbu mA berubah (≈ 1,99 → 2,45 mA) |
| **5.4** (jejak arus 3 detik), hal. 67 | `gambar_bab4/gambar_4_1_jejak_arus.png` | sumbu arus, garis arus diam |
| **5.5** (rincian jejak K2 dan K4), hal. 68 | `gambar_bab4/gambar_4_1b_jejak_arus_rinci.png` | sumbu arus |
| **5.8** (daya dinamis), hal. 71 | `gambar_bab4/gambar_4_4_daya_dinamis.png` | nilai mW dan batang galat (U_gab 5 % → 1,5 %) |
| **5.9** (energi per permintaan), hal. 72 | `gambar_bab4/gambar_4_5_energi_per_permintaan.png` | nilai mJ dan batang galat |
| **5.10** (plot interaksi), hal. 75 | `gambar_bab4/gambar_4_6_interaksi.png` | panel daya dinamis (mW) berubah; panel durasi tidur sama |
| (opsional, baru) dekomposisi energi | `gambar_bab4/gambar_4_12_dekomposisi_energi.png` | memperkuat Tabel 5.10; v4 hanya punya tabel |

Gambar yang **tidak perlu diganti** (isi sama dengan v4; sudah saya bandingkan secara visual dengan halaman PDF v4): 5.1 (frame hilang), 5.2 (Bland–Altman durasi tidur), 5.6 (latensi), 5.7 (durasi tidur), 5.11 (tingkat layanan vs model), 5.12 (throughput).

**Semua gambar Bab 5 sudah diekspor dengan nomor naskah ke `data/analisis/stop_final/gambar_naskah/`** (`Gambar_5.<n>_<isi>.png` + `README.md`), oleh `tools/ekspor_gambar_naskah.py`, yang menolak menyalin bila gambar lebih tua dari `runs.csv` atau kalibrasi analisis berbeda dari `kalibrasi_param.py`. Gunakan folder itu untuk mengganti gambar di Word (wajib: 5.3, 5.4, 5.5, 5.8, 5.9, 5.10; opsional: dekomposisi energi).

**Diagram di bab lain (Gambar 2.1, 3.1–3.5): tidak memuat angka yang berubah akibat kalibrasi** (saya periksa halaman PDF v4: tidak ada nilai arus, daya, energi, shunt, atau lantai 1,99 mA), jadi tidak perlu diganti karena kalibrasi. Bab 4 tidak memuat gambar. Tetapi saya melihat beberapa isi diagram yang tidak sesuai varian Stop. Ini masalah lama (butir E4 daftar kekurangan 18 Nov), bukan akibat kalibrasi, dan diagramnya tidak dihasilkan skrip sehingga harus Anda gambar ulang:
| Gambar | Isi di v4 | Seharusnya (data final) |
|---|---|---|
| 2.1 (hal. 19) | kotak "CPU tidur (WFI) selama IC bekerja" | "CPU tidur dalam (Stop)" |
| 3.2 (hal. 29) | host "tidur (WFI)"; logger "kristal 48 MHz"; kotak "Multimeter dan osiloskop" | "tidur dalam (Stop)"; logger **72 MHz** (host yang 48 MHz); multimeter saja (osiloskop dihapus dari Tabel 3.3/3.4); INA226 alamat 0x44 sudah benar |
| 3.4 (hal. 34) | host "bangun dari WFI"; hardware "≈ 202 ms per tanda tangan, CPU aktif ≈ 72 ms, kapasitas ≈ 4,95/detik, utilisasi ≈ 2,0" | Stop; DS28E38 **≈ 116 ms**, CPU aktif ≈ 19 ms, kapasitas **≈ 8,60/detik**, utilisasi **≈ 1,17** (teks 3.4.1 sudah benar, hanya gambar yang lama) |
| 3.5 (hal. 36) | "CPU tidur (WFI)", "kembali WFI", "catat DWT" | "tidur dalam (Stop)"; latensi dicatat DWT (software) atau RTC (hardware offloading) |
| 3.1 (hal. 22), 3.3 (hal. 31) | tidak ada angka atau istilah yang bertentangan | tetap |

---

## I. Yang tidak berubah (sudah diperiksa)

Semua nilai latensi (654,9/116,2 ms) dan persentasenya (82,25 %, 82,24 %); durasi tidur dan selisihnya (63,08 dan 83,59 poin; ΔΔ +20,51); tingkat layanan, throughput, utilisasi (Tabel 5.13); nilai p, statistik Welch untuk latensi dan durasi tidur; Bland–Altman latensi (Tabel 5.4 baris K1 dan K2); Tabel 5.2, 5.7, 5.15, 5.16 (rasio 17,7 % waktu dan 7,3 % energi dinamis, "1/13,6" tidak berubah); uji jalur galat; seluruh Bab 1–2 kecuali A1; Intisari dan Abstract (tidak memuat angka mutlak arus/daya/energi, cukup persentase).

---

## J. Temuan tambahan di luar kalibrasi (saya temukan saat membaca v4)

| # | Letak | Catatan |
|---|---|---|
| J1 | **Tabel 5.6**, baris Durasi tidur, hal. 65 | v4 menulis "≤ 0,03 %", sedangkan keluaran analisis memberi U_gab relatif 0,002–0,25 % (maksimum pada K3, durasi tidur 0,38 %). Berlaku juga pada hasil lama. Periksa dan sesuaikan. |
| J2 | **Tabel 4.7 butir 15**, hal. 57 (PDF 70) | masih memuat hipotesis lama "penerima masih mati sekitar 0,3–0,5 ms saat jam dipulihkan", sedangkan 5.2.1 dan 5.12 butir 4 sudah memuat diagnosis (frame hilang = tanda tangan 210 ms akibat alarm RTC bertepatan). Selaraskan. |
| J3 | **Demo sidang**: arus K2S.A demo 3,12 mA vs skripsi 2,55 mA | kalibrasi **tidak** menjelaskannya, karena demo (`demo_sidang.py`) juga memakai shunt 0,100 Ω (demo terkoreksi ≈ 3,8 mA vs skripsi terkoreksi 3,14 mA). Selisih ±22 % tetap belum dijelaskan (M10). Tidak perlu masuk naskah; siapkan jawaban sidang. |
| J4 | **Lampiran B** | rangkaian "10–18 November" belum mencatat kerja 19–21 November (demo, perbaikan naskah, kalibrasi); cukup baris 21 Nov (F1). |
| J5 | Penomoran tabel | menambah tabel kalibrasi (D4b) menggeser nomor Tabel 5.5 dst. Gunakan Insert Caption dan Update Field agar daftar tabel dan rujukan silang (misalnya "Tabel 5.14") ikut berubah; periksa rujukan manual "Tabel 5.x" di teks. |
| J6 | `tools/analisis_d.py` mode `--stop`, baris 580-583 | `bland_altman.csv` hanya berisi baris S_cpu; baris latensi pin vs DWT/RTC tidak terbentuk karena `pins["arch"]` berisi `K1S/K2S` sedangkan pencocokan memakai `K1/K2`. Angka Tabel 5.4 di v4 (K1 +9,29 ms sd 0,06; K2 +3,28 ms sd 0,26) **benar** (cocok dengan hitungan ulang dari `latensi_pin_vs_dwt.csv`), tetapi tidak dihasilkan otomatis. Tidak memengaruhi naskah; perbaikan kode opsional. |

---

## K. Yang masih menunggu Anda

1. Tahun/keterangan katalog SANWA CD800a (F3), dan pemeriksaan manual lengkap bahwa akurasi ±(2,2 % + 5 digit) berlaku untuk rentang 40 mA (katalog hanya memuat akurasi terbaik per fungsi). Bila rentang 40 mA kurang teliti, u_kal 1,5 % tetap menutup karena nilai utama berasal dari metode selisih (resistor 1 %).
2. Keputusan memakai Tabel 5.5 baru atau menyederhanakan (D4b).
3. Posisi pad VCC/CEXT papan DS9121 (hanya satu pin; apakah terhubung atau dibiarkan) bila ingin ditulis di B9.
4. Konfirmasi osiloskop tidak dipakai (sudah dihapus dari Tabel 3.3/3.4 di v4).
5. Nama pembimbing/penguji dan Prakata (halaman pengesahan), Update Field daftar isi/tabel/gambar (belum terisi di v4).
6. C2 (angka paper pembanding di Tabel 2.1 dan 5.16) dan E4 (gambar ulang Gambar 2.1, 3.1, 3.2, 3.4) dari daftar kekurangan lama.

## Urutan kerja yang disarankan di Word
1. Bab 3: B1–B15 (kalibrasi, 3.4.10) → 2. Tabel 5.3 (D2) → 3. 5.2.2 (D4) → 4. angka mutlak Bab 5 dari D14 sampai D29 → 5. gambar (H) → 6. Bab 6 (E) → 7. Bab 4 dan Bab 1 → 8. Lampiran/Pustaka/Lambang (F) → 9. Update Field semua → 10. cek ulang `Ctrl+F` untuk "1,99", "10,4 mW", "40,10", "41,03", "5 %", "asumsi 5", "belum dilakukan", "4,7 kΩ".

Kata kunci pencarian yang harus nol hasilnya (kecuali di kutipan sejarah) setelah revisi: `1,99 mA`, `1,984`, `1,992`, `1,985`, `10,4 mW`, `40,10`, `41,03`, `40,59`, `61,18`, `25,70`, `2,96 mW`, `3,01 mJ`, `3,00 mJ`, `asumsi 5`, `belum dilakukan`, `4,7 kΩ` (kecuali pull-up I²C modul INA226 dan PCB logger, yang memang 4,7 kΩ).