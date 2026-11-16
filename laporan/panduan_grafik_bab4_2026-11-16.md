# Panduan Grafik Inti Bab 4 (Hasil dan Pembahasan)

Tanggal: 16 November 2026
Sumber angka: `data/analisis/20261116_012530_r0.1/` 

Dokumen ini adalah lembar kerja untuk memplot. Tiap grafik punya: berkas data dan kolom yang dipakai, isi sumbu X dan Y, usulan keterangan gambar, catatan, dan tabel pendukung berisi angka yang seharusnya muncul di grafik (untuk mencocokkan hasil plot).

---

## 0. Hal yang berlaku untuk semua grafik

### 0.1 Lokasi data

| Kebutuhan | Berkas | Isi |
|---|---|---|
| Nilai per run (titik data, boxplot) | `data/analisis/20261116_012530_r0.1/runs.csv` | 140 baris, satu baris per run |
| Rerata, SD, IK 95 % per kelompok | `data/analisis/20261116_012530_r0.1/deskriptif.csv` | satu baris per kelompok per metrik |
| Tingkat layanan dan model | `data/analisis/20261116_012530_r0.1/layanan.csv` | satu baris per kelompok |
| Interaksi | `data/analisis/20261116_012530_r0.1/interaksi_dd.csv` | satu baris per metrik |
| Ketidakpastian (batang galat) | `data/analisis/20261116_012530_r0.1/ketidakpastian.csv` | kolom `U_gab` |
| Jejak arus mentah | `data/runs/<sesi>/<nnn>_<K1|K2>_<A|B>.csv` | cuplikan INA226 dan tepi pin status |
| Gambar yang sudah jadi | `data/analisis/20261116_012530_r0.1/gambar/` | 26 PNG keluaran `tools/analisis_d.py` |

### 0.2 Penyaring wajib di `runs.csv`

- Pakai hanya baris dengan `sah == True`.
- Kolom `kelompok` berisi K1, K2, K3, K4, K1-IDLE, K2-IDLE. Untuk grafik inti pakai K1 sampai K4 saja.
- Hasil penyaringan harus tepat 30 run per kelompok (total 120). Kalau jumlahnya lain, penyaringnya salah.

### 0.3 Arti kelompok

| Kelompok | Arsitektur | Skenario | Laju permintaan |
|---|---|---|---|
| K1 | Software (Mbed TLS) | Normal | 1 permintaan/s |
| K2 | DS28E38 | Normal | 1 permintaan/s |
| K3 | Software (Mbed TLS) | Barrage | 10 permintaan/s |
| K4 | DS28E38 | Barrage | 10 permintaan/s |

### 0.4 Konvensi tampilan (supaya semua gambar terbaca sebagai satu seri)

- Urutan di sumbu X selalu: K1, K2, K3, K4. Alternatif yang lebih mudah dibaca: dua kelompok batang (Normal, Barrage), tiap kelompok berisi Software lalu DS28E38.
- Warna mengikuti arsitektur, bukan kelompok: satu warna untuk Software (K1, K3), satu warna untuk DS28E38 (K2, K4). Skenario dibedakan dengan pola: Normal polos, Barrage berarsir.
- Tulis nilai rerata di atas tiap batang.
- Label sumbu selalu menyertakan satuan. Pemisah desimal koma (naskah berbahasa Indonesia).
- Sumbu Y diagram batang mulai dari nol.

---

## Gambar 4.1 Jejak arus terhadap waktu (belum ada, perlu dibuat)

**Tujuan:** menunjukkan mekanismenya secara langsung: kapan host bangun dan kapan tidur pada tiap kelompok. Grafik ini menjadi pembuka sebelum angka-angka statistik.

**Data (satu run contoh per kelompok, semuanya run sah):**

| Kelompok | Berkas |
|---|---|
| K1 | `data/runs/20261115_173019_K1/002_K1_A.csv` |
| K3 | `data/runs/20261115_173019_K1/006_K1_B.csv` |
| K2 | `data/runs/20261115_180129_K2/002_K2_A.csv` |
| K4 | `data/runs/20261115_180129_K2/004_K2_B.csv` |

**Format berkas mentah:**

| Awal baris | Kolom | Arti |
|---|---|---|
| `s` | `s,<t>,<shunt>,<bus>` | cuplikan INA226 (sekitar 496 cuplikan/s) |
| `e` | `e,<t>,<level>` | tepi pin status host: 1 = CPU aktif, 0 = CPU tidur |
| `h` | `h,<t>,>START,...` dan `h,<t>,>END,...` | awal dan akhir jendela ukur |

**Konversi:**

- Waktu (s) = (`t` - `t` pada baris `>START`) / 72.000.000
- Arus (mA) = (`shunt` + 2) x 2,5e-6 / R_shunt x 1000, dengan R_shunt = 0,100 Ohm (ganti setelah kalibrasi)
- Tegangan bus (V) = `bus` x 1,25e-3

**Bentuk:** empat panel bersusun 2 x 2 (baris: Normal, Barrage; kolom: Software, DS28E38), diagram garis.

- **Sumbu X:** waktu sejak awal jendela (s). Ambil potongan 3 s saja, misalnya 10 s sampai 13 s. Rentang X sama di keempat panel.
- **Sumbu Y:** arus host (mA). Rentang Y sama di keempat panel, misalnya 10 sampai 21 mA.
- **Tambahan:** arsir latar pada selang pin status = 1 (CPU aktif) dari baris `e`; garis putus-putus mendatar pada arus IDLE (K1 11,55 mA; K2 12,05 mA).

**Usulan keterangan:** "Gambar 4.1 Jejak arus host selama 3 s pada keempat kelompok uji. Daerah berarsir menandai CPU aktif menurut pin status; garis putus-putus adalah arus diam."

**Catatan:**

- Yang harus terlihat: K1 aktif sekitar 0,66 s lalu tidur sekitar 0,34 s tiap permintaan; K2 hanya dua pulsa pendek per permintaan (sekitar 26 ms dan 46 ms) dan tidur selama IC menghitung; K3 aktif terus tanpa jeda; K4 berselang-seling cepat antara aktif dan tidur.
- Selang cuplik 2 ms. Pulsa aktif K4 yang lebih pendek dari 2 ms (bangun karena UART) tidak terlihat di jejak arus, hanya di pin status. Sebutkan ini di pembahasan.
- Ini gambar ilustrasi satu run, bukan bukti statistik. Jangan menarik angka dari sini.

**Tabel pendukung 4.1 (angka dari run contoh, untuk mencocokkan plot):**

| Kelompok | Arus rata-rata (mA) | Arus saat tidur (mA) | Arus saat aktif (mA) | Jumlah tepi pin |
|---|---|---|---|---|
| K1 | 16,33 | 11,78 | 18,83 | 120 |
| K2 | 13,16 | 12,72 | 18,99 | 240 |
| K3 | 18,69 | 11,76 | 18,72 | 4 |
| K4 | 16,60 | 15,23 | 19,12 | 1392 |

---

## Gambar 4.2 Latensi tanda tangan

**Menjawab:** H1a (latensi DS28E38 lebih rendah).

**Data:** `deskriptif.csv`, baris `metrik == lat_mean_ms_nyata`, kolom `rerata`. Titik per run: `runs.csv` kolom `lat_mean_ms_nyata`.

**Bentuk:** diagram batang, empat batang.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** latensi tanda tangan (ms), mulai dari 0

**Usulan keterangan:** "Gambar 4.2 Rerata latensi tanda tangan ECDSA P-256 per kelompok (n = 30 run per kelompok)."

**Catatan:**

- Sebaran sangat sempit (CV sekitar 0,01 %), sehingga boxplot hanya tampak sebagai garis dan batang galat tidak terlihat. Karena itu pakai batang dengan nilai tertulis. Gambar lama `gambar/box_lat_mean_ms_nyata.png` kurang informatif untuk naskah.
- Pesan utamanya dua: DS28E38 sekitar 3,2 kali lebih cepat, dan latensi tidak berubah oleh barrage (beda K1 ke K3 hanya 0,11 ms).
- Tidak bergantung pada kalibrasi shunt.

**Tabel pendukung 4.2:**

| Kelompok | Rerata (ms) | SD (ms) | Min (ms) | Maks (ms) |
|---|---|---|---|---|
| K1 | 655,40 | 0,071 | 655,27 | 655,56 |
| K2 | 202,15 | 0,026 | 202,11 | 202,20 |
| K3 | 655,51 | 0,089 | 655,33 | 655,70 |
| K4 | 202,17 | 0,027 | 202,13 | 202,23 |

Selisih Software - DS28E38: 453,25 ms (Normal) dan 453,33 ms (Barrage), keduanya 69,16 %.

---

## Gambar 4.3 Durasi tidur CPU (S_cpu)

**Menjawab:** H1c (host lebih lama tidur dengan DS28E38), inti argumen mitigasi Denial of Sleep.

**Data:** `deskriptif.csv`, baris `metrik == S_cpu_log`. Titik per run: `runs.csv` kolom `S_cpu_log`.

**Bentuk:** diagram batang, empat batang.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** durasi tidur CPU (% dari waktu pengamatan), rentang 0 sampai 100

**Usulan keterangan:** "Gambar 4.3 Persentase waktu CPU host berada dalam mode tidur selama jendela 60 s (n = 30 run per kelompok)."

**Catatan:**

- Batang K3 hampir nol (0,38 %). Tulis nilainya supaya tidak dikira data kosong. Inilah kondisi Denial of Sleep.
- Selisih dinyatakan dalam poin persentase, bukan persen relatif.
- Tidak bergantung pada kalibrasi shunt. Gambar lama: `gambar/box_S_cpu_log.png`.

**Tabel pendukung 4.3:**

| Kelompok | S_cpu rerata (%) | Selisih DS28E38 - Software (poin) |
|---|---|---|
| K1 | 35,46 | |
| K2 | 92,99 | +57,53 (Normal) |
| K3 | 0,38 | |
| K4 | 64,72 | +64,34 (Barrage) |

---

## Gambar 4.4 Daya dinamis (P_dyn)

**Menjawab:** H1b (daya dinamis lebih rendah dengan DS28E38).

**Data:** `runs.csv` kolom `P_dyn_mW` (titik per run), atau `deskriptif.csv` baris `metrik == P_dyn_mW`. Batang galat: `ketidakpastian.csv` kolom `U_gab`.

**Bentuk:** boxplot per kelompok, atau diagram batang rerata dengan batang galat +- U_gab.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** daya dinamis (mW), yaitu daya rata-rata dikurangi daya diam arsitektur yang sama

**Usulan keterangan:** "Gambar 4.4 Daya dinamis host per kelompok (n = 30 run per kelompok). Batang galat menyatakan ketidakpastian gabungan."

**Catatan:**

- WAJIB dibuat ulang setelah kalibrasi shunt. Nilai mutlak akan bergeser, persentase penghematan hampir tidak berubah.
- K1 dan K4 hampir sama (23,2 dan 23,0 mW). Ini bukan kebetulan yang perlu disembunyikan: K4 melayani 3,3 kali lebih banyak permintaan dengan daya yang sama. Hubungkan dengan Gambar 4.5.
- Alternatif yang lebih stabil antar blok: arus dinamis `I_dyn_mA` (pergeseran antar blok 0,009 mA dibanding 0,76 mW pada P_dyn K1). Gambar lama: `gambar/box_P_dyn_mW.png`, `gambar/box_I_dyn_mA.png`.

**Tabel pendukung 4.4:**

| Kelompok | P_dyn rerata (mW) | SD (mW) | U_gab (mW) | I_dyn rerata (mA) |
|---|---|---|---|---|
| K1 | 23,25 | 0,62 | 1,42 | 4,69 |
| K2 | 5,30 | 0,10 | 0,32 | 1,02 |
| K3 | 35,11 | 0,74 | 2,09 | 7,09 |
| K4 | 23,05 | 0,14 | 1,17 | 4,46 |

Penghematan P_dyn: 17,95 mW atau 77,2 % (Normal); 12,07 mW atau 34,4 % (Barrage).

---

## Gambar 4.5 Energi per permintaan terlayani (E_lyn)

**Menjawab:** H1b dari sisi efisiensi: berapa energi yang dibayar untuk satu tanda tangan yang berhasil dilayani.

**Data:** `runs.csv` kolom `E_lyn_mJ`, atau `deskriptif.csv` baris `metrik == E_lyn_mJ`. Batang galat: `ketidakpastian.csv` kolom `U_gab`.

**Bentuk:** diagram batang rerata dengan batang galat +- U_gab (atau boxplot).

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** energi dinamis per permintaan terlayani (mJ)

**Usulan keterangan:** "Gambar 4.5 Energi dinamis per permintaan terlayani per kelompok (n = 30 run per kelompok)."

**Catatan:**

- WAJIB dibuat ulang setelah kalibrasi shunt.
- Pola yang harus terlihat: energi per layanan hampir tetap terhadap skenario dan hanya ditentukan arsitektur (Software sekitar 23 mJ, DS28E38 sekitar 5 mJ). Ini melengkapi Gambar 4.4.
- Alternatif berbasis arus: muatan per layanan `Q_lyn_mC`. Gambar lama: `gambar/box_E_lyn_mJ.png`, `gambar/box_Q_lyn_mC.png`.

**Tabel pendukung 4.5:**

| Kelompok | E_lyn rerata (mJ) | SD (mJ) | U_gab (mJ) | Q_lyn rerata (mC) |
|---|---|---|---|---|
| K1 | 23,83 | 0,63 | 1,45 | 4,81 |
| K2 | 5,42 | 0,10 | 0,32 | 1,04 |
| K3 | 23,32 | 0,49 | 1,39 | 4,71 |
| K4 | 4,68 | 0,03 | 0,24 | 0,91 |

Penghematan E_lyn: 18,41 mJ atau 77,3 % (Normal); 18,64 mJ atau 79,9 % (Barrage).

---

## Gambar 4.6 Interaksi arsitektur dengan skenario

**Menjawab:** RM3, apakah manfaat DS28E38 berubah saat diserang.

**Data:** `deskriptif.csv` (rerata empat kelompok untuk `S_cpu_log` dan `P_dyn_mW`); nilai interaksi dan IK 95 % dari `interaksi_dd.csv`.

**Bentuk:** dua panel diagram garis (plot interaksi), panel (a) S_cpu dan panel (b) P_dyn. Tiap panel berisi dua garis.

- **Sumbu X:** skenario (Normal, Barrage), dua titik
- **Sumbu Y:** (a) durasi tidur CPU (%); (b) daya dinamis (mW)
- **Garis:** satu untuk Software (K1 ke K3), satu untuk DS28E38 (K2 ke K4)

**Usulan keterangan:** "Gambar 4.6 Plot interaksi arsitektur dan skenario untuk (a) durasi tidur CPU dan (b) daya dinamis. Garis yang tidak sejajar menunjukkan adanya interaksi."

**Catatan:**

- Cara membacanya: jarak tegak antara dua garis adalah manfaat DS28E38. Di panel (a) jarak melebar saat barrage (manfaat tidur membesar); di panel (b) jarak menyempit (manfaat daya dinamis mengecil karena K4 melayani jauh lebih banyak).
- Panel (b) dibuat ulang setelah kalibrasi shunt; panel (a) tidak terpengaruh.
- Gambar lama: `gambar/interaksi_S_cpu_log.png`, `gambar/interaksi_P_dyn_mW.png`.

**Tabel pendukung 4.6 (DD = manfaat saat Barrage dikurangi manfaat saat Normal):**

| Metrik | Manfaat Normal | Manfaat Barrage | DD | IK 95 % | Tafsiran |
|---|---|---|---|---|---|
| S_cpu (poin) | 57,53 | 64,34 | +6,80 | [6,795; 6,809] | manfaat lebih besar saat barrage |
| P_dyn (mW) | 17,95 | 12,07 | -5,88 | [-6,22; -5,53] | manfaat lebih kecil saat barrage |
| I_dyn (mA) | 3,67 | 2,63 | -1,04 | [-1,05; -1,03] | manfaat lebih kecil saat barrage |
| E_lyn (mJ) | 18,41 | 18,64 | +0,23 | [-0,05; 0,52] | tidak terbukti berbeda |
| Latensi (ms) | 453,25 | 453,33 | +0,08 | [0,04; 0,13] | berbeda secara statistik, praktis sama |

---

## Gambar 4.7 Tingkat layanan terukur dan model (belum ada dalam bentuk ini)

**Menjawab:** H4, ketahanan layanan saat barrage dan kecocokan dengan model utilisasi.

**Data:** `layanan.csv`, kolom `TL_rerata` (terukur) dan `TL_model` (model).

**Bentuk:** diagram batang berpasangan, dua batang per kelompok.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** tingkat layanan (% permintaan terlayani), rentang 0 sampai 100
- **Legenda:** Terukur, Model

**Usulan keterangan:** "Gambar 4.7 Tingkat layanan terukur dibandingkan dengan model utilisasi TL = min(1, 1/(lambda x t_sign))."

**Catatan:**

- Tingkat layanan = permintaan terlayani / permintaan datang. Pada skenario Normal keduanya 100 % (tidak ada permintaan terlewat).
- K4 tetap kehilangan sekitar separuh permintaan. Tegaskan di pembahasan bahwa offloading adalah mitigasi, bukan kekebalan: batasnya waktu tanda tangan IC.
- Selisih terukur terhadap model sekitar 0,2 poin; cukup ditulis di tabel.
- Tidak bergantung pada kalibrasi shunt. Gambar lama `gambar/box_TL_pct.png` hanya berisi nilai terukur.

**Tabel pendukung 4.7:**

| Kelompok | Datang (per run) | Terlayani | Terlewat | TL terukur (%) | TL model (%) |
|---|---|---|---|---|---|
| K1 | 59 | 59 | 0 | 100,00 | 100,00 |
| K2 | 59 | 59 | 0 | 100,00 | 100,00 |
| K3 | 604 sampai 605 | 91 | 513 sampai 514 | 15,04 | 15,24 |
| K4 | 603 sampai 605 | 297 sampai 298 | 306 sampai 307 | 49,25 | 49,44 |

---

## Gambar 4.8 Throughput terukur dan batas model

**Menjawab:** H4 dari sisi kapasitas: berapa tanda tangan per detik yang sanggup dihasilkan.

**Data:** `layanan.csv`, kolom `Theta` (terukur) dan `Theta_maks_model` (batas = 1/t_sign). Titik per run: `runs.csv` kolom `Theta_sps`.

**Bentuk:** diagram batang empat kelompok, ditambah garis putus-putus mendatar untuk batas kapasitas tiap arsitektur.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** throughput (tanda tangan/s)
- **Garis acuan:** 1,53 tanda tangan/s (batas Software) dan 4,95 tanda tangan/s (batas DS28E38)

**Usulan keterangan:** "Gambar 4.8 Throughput tanda tangan per kelompok. Garis putus-putus adalah kapasitas maksimum tiap arsitektur (1/t_sign)."

**Catatan:**

- Pada skenario Normal throughput dibatasi laju permintaan (sekitar 0,98 per detik), jadi kedua arsitektur tampak sama. Perbedaan baru muncul saat barrage: keduanya jenuh di batasnya masing-masing.
- Rasio K4 terhadap K3 = 3,27 kali.
- Tidak bergantung pada kalibrasi shunt. Gambar lama: `gambar/box_Theta_sps.png` (tanpa garis batas).

**Tabel pendukung 4.8:**

| Kelompok | Throughput terukur (per s) | Batas model (per s) | Utilisasi rho |
|---|---|---|---|
| K1 | 0,976 | 1,526 | 0,64 |
| K2 | 0,979 | 4,947 | 0,20 |
| K3 | 1,506 | 1,526 | 6,56 |
| K4 | 4,927 | 4,946 | 2,02 |

---

## Tabel pendukung umum (bukan grafik)

### Tabel A. Uji hipotesis utama, Software vs DS28E38

Sumber: `banding_sw_hw.csv` (baris dengan `utama == True`). Uji Welch t, koreksi Holm atas 6 perbandingan; keenamnya menolak H0 dengan p Holm paling besar 1,2e-10.

| Skenario | Metrik | Software | DS28E38 | Selisih | IK 95 % selisih | Penghematan |
|---|---|---|---|---|---|---|
| Normal | Latensi (ms) | 655,40 | 202,15 | 453,25 | [453,22; 453,27] | 69,16 % |
| Normal | P_dyn (mW) | 23,25 | 5,30 | 17,95 | [17,73; 18,17] | 77,19 % |
| Normal | S_cpu (%) | 35,46 | 92,99 | 57,53 poin | [57,53; 57,54] | |
| Barrage | Latensi (ms) | 655,51 | 202,17 | 453,33 | [453,30; 453,36] | 69,16 % |
| Barrage | P_dyn (mW) | 35,11 | 23,05 | 12,07 | [11,81; 12,34] | 34,36 % |
| Barrage | S_cpu (%) | 0,38 | 64,72 | 64,34 poin | [64,33; 64,34] | |

### Tabel B. Pengaruh barrage pada tiap arsitektur (RM2)

Sumber: `pengaruh_barrage.csv`. Selisih = Barrage dikurangi Normal.

| Metrik | Software (K3 - K1) | DS28E38 (K4 - K2) |
|---|---|---|
| S_cpu (poin) | -35,08 | -28,27 |
| P_dyn (mW) | +11,86 (+51,0 %) | +17,74 (+334,7 %) |
| I_dyn (mA) | +2,40 (+51,3 %) | +3,45 (+338,1 %) |
| E_lyn (mJ) | -0,51 (-2,1 %) | -0,74 (-13,6 %) |
| Tingkat layanan (poin) | -84,96 | -50,75 |
| Throughput (per s) | +0,53 (+54,3 %) | +3,95 (+403,3 %) |
| Latensi (ms) | +0,11 | +0,02 |

### Tabel C. Kondisi diam (acuan P_dyn)

Sumber: `deskriptif.csv`, kelompok K1-IDLE dan K2-IDLE.

| Arsitektur | Jumlah run | Arus diam (mA) | Daya diam (mW) | Tegangan bus (V) |
|---|---|---|---|---|
| Software | 6 | 11,55 | 59,34 | 5,14 |
| DS28E38 | 4 | 12,05 | 62,48 | 5,19 |

### Tabel D. Sebaran run sah per blok

| Blok | Sesi | K1 | K2 | K3 | K4 |
|---|---|---|---|---|---|
| blok1 | 20261115_173019_K1 | 9 | | 10 | |
| blok2 | 20261115_180129_K2 | | 18 | | 19 |
| blok3 | 20261115_185637_K1 | 17 | | 18 | |
| blok3b | 20261115_204018_K1 | 4 | | 2 | |
| blok4 | 20261115_205801_K2 | | 12 | | 11 |
| Jumlah | | 30 | 30 | 30 | 30 |

---

## Ringkasan pengerjaan

| Gambar | Status | Terpengaruh kalibrasi shunt |
|---|---|---|
| 4.1 Jejak arus | belum ada | ya (skala arus) |
| 4.2 Latensi | ada sebagai boxplot, disarankan diganti batang | tidak |
| 4.3 S_cpu | ada | tidak |
| 4.4 P_dyn | ada | ya |
| 4.5 E_lyn | ada | ya |
| 4.6 Interaksi | ada | panel P_dyn saja |
| 4.7 Tingkat layanan vs model | belum ada | tidak |
| 4.8 Throughput vs batas | ada tanpa garis batas | tidak |

Urutan kerja yang disarankan: kerjakan dulu 4.2, 4.3, 4.7, 4.8 (sudah final), lalu 4.1, dan terakhir 4.4, 4.5, 4.6b setelah nilai shunt hasil kalibrasi tersedia.
