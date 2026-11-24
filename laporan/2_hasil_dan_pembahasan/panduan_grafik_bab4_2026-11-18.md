# Panduan Grafik Inti Bab 4 (Hasil dan Pembahasan), Data *Deep Sleep*

Tanggal: 18 November 2026
Sumber angka: `data/analisis/stop_final/` (keluaran `tools/3_analisis/analisis_d.py --stop`, 18 November 2026 pukul 15.31; manifest `data/tahap_stop.txt`)

Dokumen ini adalah lembar kerja untuk memplot dan menggantikan panduan grafik sebelumnya (`laporan/arsip/panduan_grafik_bab4_2026-11-16.md`) untuk keperluan skripsi. **Hanya data dan analisis varian *deep sleep* (Stop mode) yang dipakai.** Tiap grafik punya: berkas data dan kolom yang dipakai, isi sumbu X dan Y, usulan keterangan gambar, catatan, dan tabel pendukung berisi angka yang seharusnya muncul di grafik (untuk mencocokkan hasil plot). Bahasa naskah mengikuti Bab 3 hasil revisi (`laporan/3_revisi_naskah/revisi_bab3_propo_fix5_2026-11-18.md`).

Perubahan terhadap panduan sebelumnya: nomor gambar 4.1–4.8 mempertahankan peran yang sama, tetapi isinya diganti data *deep sleep*; ditambah tiga gambar baru (4.9–4.11) untuk bukti keabsahan pengukuran (keadaan bersih, frame UART, kesesuaian instrumen).

---

## 0. Hal yang berlaku untuk semua grafik

### 0.1 Lokasi data

| Kebutuhan | Berkas | Isi |
|---|---|---|
| Nilai per run (titik data, boxplot) | `data/analisis/stop_final/runs.csv` | 139 baris, satu baris per run |
| Rerata, SD, IK 95 % per kelompok | `data/analisis/stop_final/deskriptif.csv` | satu baris per kelompok per metrik |
| Tingkat layanan dan model | `data/analisis/stop_final/layanan.csv` | satu baris per kelompok |
| Interaksi | `data/analisis/stop_final/interaksi_dd.csv` | satu baris per metrik |
| Pengaruh barrage | `data/analisis/stop_final/pengaruh_barrage.csv` | selisih K3−K1 dan K4−K2 |
| Perbandingan SW vs HW | `data/analisis/stop_final/banding_sw_hw.csv` | uji Welch, Holm, gain |
| Ketidakpastian (batang galat) | `data/analisis/stop_final/ketidakpastian.csv` | kolom `U_gab` |
| Kesesuaian instrumen | `data/analisis/stop_final/bland_altman.csv`, `latensi_pin_vs_dwt.csv` | bias dan batas kesesuaian |
| Jejak arus mentah | `data/runs/<sesi>/<nnn>_<K1S|K2S>_<A|B>.csv` | cuplikan INA226 dan tepi pin status |
| Gambar yang sudah jadi | `data/analisis/stop_final/gambar/` | 24 PNG keluaran `tools/3_analisis/analisis_d.py` |

Sesi sumber (blok): blok1 `20261118_115044_K2S`, blok2 `20261118_124646_K1S`, blok3 `20261118_132320_K1S`, blok4 `20261118_140335_K2S`.

### 0.2 Penyaring wajib di `runs.csv`

- Pakai hanya baris dengan `sah == True`.
- Kolom `kelompok` berisi K1, K2, K3, K4, K1-IDLE, K2-IDLE. Untuk grafik inti pakai K1 sampai K4 saja.
- Hasil penyaringan harus tepat **30 run per kelompok (total 120)**. Berkas memuat 41 baris K4 (11 gugur karena frame hilang melebihi batas); kalau penyaring `sah` tidak dipasang, K4 akan berjumlah 41 dan itu salah.
- Kolom arsitektur di berkas bernama `K1S`/`K2S`, sedangkan kolom `kelompok` sudah K1–K4.

### 0.3 Arti kelompok

| Kelompok | Arsitektur | Skenario | Laju permintaan |
|---|---|---|---|
| K1 | Software (Mbed TLS) | Normal | 1 permintaan/s |
| K2 | DS28E38 | Normal | 1 permintaan/s |
| K3 | Software (Mbed TLS) | Barrage | 10 permintaan/s |
| K4 | DS28E38 | Barrage | 10 permintaan/s |

### 0.4 Konvensi tampilan (supaya semua gambar terbaca sebagai satu seri)

- Urutan di sumbu X selalu: K1, K2, K3, K4. Alternatif yang lebih mudah dibaca: dua kelompok batang (Normal, Barrage), tiap kelompok berisi Software lalu DS28E38.
- Warna mengikuti arsitektur, bukan kelompok: satu warna untuk Software (K1, K3), satu warna untuk DS28E38 (K2, K4). Skenario dibedakan dengan pola: Normal polos, Barrage berarsir. Pilih pasangan warna yang tetap terbedakan pada cetak hitam-putih.
- Tulis nilai rerata di atas tiap batang.
- Label sumbu selalu menyertakan satuan. Pemisah desimal koma (naskah berbahasa Indonesia).
- Sumbu Y diagram batang mulai dari nol.
- Resolusi minimal 300 dpi; ukuran huruf di gambar sama dengan naskah (10–11 pt).
- Semua gambar hanya memuat data *deep sleep*; jangan menggabungkan data dari sesi lain.

### 0.5 Definisi yang dipakai di tabel pendukung

- **Daya dinamis (P_dyn)** dan **arus dinamis (I_dyn)** dihitung terhadap IDLE arsitektur yang sama pada blok yang sama (IDLE ≈ 1,99 mA atau 10,4 mW). Karena IDLE rendah, nilai dinamis K2 kecil (2,96 mW) dan persentase naik dari K2 ke K4 besar; baca selalu bersama nilai mutlak (`P_avg_mW`).
- **Tidur CPU (S_cpu)** = waktu CPU berada dalam mode Stop; **nilai kelompok memakai `S_cpu_log`** (menurut pin status yang direkam logger), bukan angka firmware.
- Nilai mutlak mA/mW/mJ memakai shunt nominal 0,100 Ω; ulangi setelah kalibrasi.

---

## Gambar 4.1 Jejak arus terhadap waktu (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_1_jejak_arus.png` dan rincian `gambar_4_1b_jejak_arus_rinci.png`. Skrip: `python tools/3_analisis/gambar_bab4_stop.py [--shunt <nilai>]`.

**Tujuan:** menunjukkan mekanismenya secara langsung: kapan host bangun dari tidur dalam dan kapan kembali tidur pada tiap kelompok. Grafik ini menjadi pembuka sebelum angka-angka statistik.

**Data (satu run contoh per kelompok, dipilih yang arus rata-ratanya paling dekat median kelompok; semuanya run sah):**

| Kelompok | Berkas |
|---|---|
| K1 | `data/runs/20261118_124646_K1S/031_K1S_A.csv` |
| K3 | `data/runs/20261118_124646_K1S/007_K1S_B.csv` |
| K2 | `data/runs/20261118_115044_K2S/033_K2S_A.csv` |
| K4 | `data/runs/20261118_115044_K2S/024_K2S_B.csv` |

**Format berkas mentah:**

| Awal baris | Kolom | Arti |
|---|---|---|
| `s` | `s,<t>,<shunt>,<bus>` | cuplikan INA226 (sekitar 496 cuplikan/s) |
| `e` | `e,<t>,<level>` | tepi pin status host: 1 = CPU aktif, 0 = CPU tidur (Stop) |
| `h` | `h,<t>,>START,...` dan `h,<t>,>END,...` | awal dan akhir jendela ukur |

**Konversi:**

- Waktu (s) = (`t` − `t` pada baris `>START`) / 72.000.000
- Arus (mA) = (`shunt` + 2) × 2,5e-6 / R_shunt × 1000, dengan R_shunt = 0,100 Ω (ganti setelah kalibrasi)
- Tegangan bus (V) = `bus` × 1,25e-3

**Bentuk:** empat panel bersusun 2 × 2 (baris: Normal, Barrage; kolom: Software, DS28E38), diagram garis.

- **Sumbu X:** waktu sejak awal jendela (s). Ambil potongan 3 s, yaitu 10 s sampai 13 s. Rentang X sama di keempat panel.
- **Sumbu Y:** arus host (mA). Rentang Y sama di keempat panel: 0 sampai 21 mA (puncak arus K2 dan K4 mencapai ±19,5 mA saat IC dan CPU aktif bersamaan; arus tidur ±2 mA harus tampak pada skala yang sama).
- **Tambahan:** arsir latar pada selang pin status = 1 (CPU aktif) dari baris `e`; garis putus-putus mendatar pada arus diam (1,99 mA).
- **Gambar rinci terpisah (4.1b):** (a) K2 satu permintaan (10,9 s sampai 11,3 s) memperlihatkan dua pulsa aktif; (b) K4 potongan 0,5 s (10,0 s sampai 10,5 s) memperlihatkan pulsa pendek ±3,3 ms dan pulsa ±15 ms berselang-seling. Inset di dalam panel K4 dibatalkan karena menutupi jejak.

**Usulan keterangan:** "Gambar 4.1 Jejak arus host selama 3 s pada keempat kelompok uji. Daerah berarsir menandai CPU aktif menurut pin status; garis putus-putus adalah arus diam."

**Catatan:**

- Yang harus terlihat (diukur dari pin status run contoh): K1 aktif satu pulsa ±664 ms per permintaan lalu tidur dalam ±340 ms; K2 aktif dua pulsa pendek per permintaan (±6,9 ms dan ±11–12 ms) dengan tidur dalam ±107 ms di antaranya, selama IC menghitung; K3 aktif terus sepanjang jendela (hanya 4 tepi); K4 berselang-seling cepat antara pulsa pendek ±3,3 ms (penerimaan frame setelah bangun) dan pulsa ±15 ms.
- Selama IC menghitung (±107 ms), arus host berada pada plato ±5–6 mA, bukan 2 mA: CPU tidur dalam tetapi host mencatu arus komputasi IC lewat strong pull-up. Itu yang membuat arus saat tidur rata-rata K2 ±2,37 mA (0,107 s × ±5,5 mA + 0,893 s × 1,99 mA ≈ 2,37 mA) dan K4 ±5,6 mA. Bahas di teks.
- Selang cuplik INA226 ±2 ms. Pulsa ±3,3 ms pada K4 hanya terlihat sebagian di jejak arus tetapi jelas di pin status; sebutkan ini di pembahasan.
- Arus saat tidur K2 (±2,37 mA) di atas arus diam (1,99 mA) terutama karena DS28E38 menarik arus selama menghitung (±107 ms per permintaan) yang ikut terhitung sebagai waktu tidur CPU.
- Ini gambar ilustrasi satu run, bukan bukti statistik. Jangan menarik angka dari sini.

**Tabel pendukung 4.1 (angka dari run contoh, untuk mencocokkan plot):**

| Kelompok | Arus rata-rata (mA) | Arus saat tidur (mA) | Arus saat aktif (mA) | Jumlah tepi pin |
|---|---|---|---|---|
| K1 | 9,678 | 2,036 | 13,799 | 120 |
| K2 | 2,555 | 2,373 | 12,181 | 240 |
| K3 | 13,715 | 2,079 | 13,759 | 4 |
| K4 | 6,922 | 5,640 | 13,631 | 2.048 |

---

## Gambar 4.2 Latensi tanda tangan (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_2_latensi.png` (batang bernilai; menggantikan `gambar/box_lat_mean_ms_nyata.png`). Skrip: `tools/3_analisis/gambar_bab4_stop.py`.

**Menjawab:** H1a (latensi DS28E38 lebih rendah).

**Data:** `deskriptif.csv`, baris `metrik == lat_mean_ms_nyata`, kolom `rerata`. Titik per run: `runs.csv` kolom `lat_mean_ms_nyata`.

**Bentuk:** diagram batang, empat batang.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** latensi tanda tangan (ms), mulai dari 0

**Usulan keterangan:** "Gambar 4.2 Rerata latensi tanda tangan ECDSA P-256 per kelompok (n = 30 run per kelompok)."

**Catatan:**

- Sebaran sangat sempit (CV 0,01–0,07 %), sehingga boxplot hanya tampak sebagai garis dan batang galat tidak terlihat. Karena itu pakai batang dengan nilai tertulis. Gambar lama `gambar/box_lat_mean_ms_nyata.png` kurang informatif untuk naskah.
- Pesan utamanya dua: DS28E38 sekitar 5,6 kali lebih cepat (82,2 %), dan latensi tidak berubah oleh barrage (beda K1 ke K3 hanya 0,03 ms; K2 ke K4 0,07 ms).
- Latensi K2 dan K4 mencerminkan konfigurasi overdrive + polling (Bab 3.4.1 b); sebut ini di keterangan atau teks.
- Tidak bergantung pada kalibrasi shunt.

**Tabel pendukung 4.2:**

| Kelompok | Rerata (ms) | SD (ms) | Min (ms) | Maks (ms) |
|---|---|---|---|---|
| K1 | 654,88 | 0,090 | 654,63 | 655,03 |
| K2 | 116,23 | 0,057 | 116,12 | 116,37 |
| K3 | 654,91 | 0,109 | 654,76 | 655,36 |
| K4 | 116,30 | 0,087 | 116,21 | 116,46 |

Selisih Software − DS28E38: 538,65 ms (Normal) dan 538,61 ms (Barrage), keduanya 82,2 %.

---

## Gambar 4.3 Durasi tidur CPU (S_cpu) (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_3_durasi_tidur_cpu.png` (menggantikan `gambar/box_S_cpu_log.png`).

**Menjawab:** H1c (host lebih lama tidur dengan DS28E38), inti argumen mitigasi Denial of Sleep.

**Data:** `deskriptif.csv`, baris `metrik == S_cpu_log`. Titik per run: `runs.csv` kolom `S_cpu_log`.

**Bentuk:** diagram batang, empat batang.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** durasi tidur dalam CPU (% dari waktu pengamatan), rentang 0 sampai 100

**Usulan keterangan:** "Gambar 4.3 Persentase waktu CPU host berada dalam mode tidur dalam (Stop) selama jendela 60 s (n = 30 run per kelompok)."

**Catatan:**

- Batang K3 hampir nol (0,38 %). Tulis nilainya supaya tidak dikira data kosong. Inilah kondisi Denial of Sleep.
- Selisih dinyatakan dalam poin persentase, bukan persen relatif.
- Tidak bergantung pada kalibrasi shunt. Gambar lama: `gambar/box_S_cpu_log.png`.

**Tabel pendukung 4.3:**

| Kelompok | S_cpu rerata (%) | SD | Selisih DS28E38 − Software (poin) |
|---|---|---|---|
| K1 | 35,08 | 0,008 | |
| K2 | 98,17 | 0,005 | +63,08 (Normal) |
| K3 | 0,377 | 0,001 | |
| K4 | 83,97 | 0,028 | +83,59 (Barrage) |

---

## Gambar 4.4 Daya dinamis (P_dyn) (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_4_daya_dinamis.png` (batang rerata dengan batang galat ± U_gab; menggantikan `gambar/box_P_dyn_mW.png`). Dibuat ulang setelah kalibrasi.

**Menjawab:** H1b (daya dinamis lebih rendah dengan DS28E38).

**Data:** `runs.csv` kolom `P_dyn_mW` (titik per run), atau `deskriptif.csv` baris `metrik == P_dyn_mW`. Batang galat: `ketidakpastian.csv` kolom `U_gab`.

**Bentuk:** boxplot per kelompok, atau diagram batang rerata dengan batang galat ± U_gab.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** daya dinamis (mW), yaitu daya rata-rata dikurangi daya diam arsitektur yang sama

**Usulan keterangan:** "Gambar 4.4 Daya dinamis host per kelompok (n = 30 run per kelompok). Batang galat menyatakan ketidakpastian gabungan."

**Catatan:**

- WAJIB dibuat ulang setelah kalibrasi shunt. Nilai mutlak akan bergeser; persentase penghematan hampir tidak berubah.
- Batang galat didominasi komponen kalibrasi shunt (±5 %), jadi gambar ini lebih informatif sebagai perbandingan relatif antar-kelompok.
- K4 (25,7 mW) lebih kecil dari K3 (61,2 mW) walaupun K4 melayani 5,6 kali lebih banyak permintaan; hubungkan dengan Gambar 4.5 dan 4.7.
- Alternatif yang lebih stabil antar blok: arus dinamis `I_dyn_mA`. Gambar lama: `gambar/box_P_dyn_mW.png`, `gambar/box_I_dyn_mA.png`.

**Tabel pendukung 4.4:**

| Kelompok | P_dyn rerata (mW) | SD (mW) | U_gab (mW) | I_dyn rerata (mA) | P_avg rerata (mW) |
|---|---|---|---|---|---|
| K1 | 40,10 | 0,042 | 2,01 | 7,69 | 50,50 |
| K2 | 2,96 | 0,018 | 0,15 | 0,566 | 13,33 |
| K3 | 61,18 | 0,051 | 3,06 | 11,73 | 71,57 |
| K4 | 25,69 | 0,038 | 1,29 | 4,93 | 36,06 |

Penghematan P_dyn: 37,14 mW atau 92,6 % (Normal); 35,48 mW atau 58,0 % (Barrage).

---

## Gambar 4.5 Energi per permintaan terlayani (E_lyn) (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_5_energi_per_permintaan.png` (menggantikan `gambar/box_E_lyn_mJ.png`). Dibuat ulang setelah kalibrasi.

**Menjawab:** H1b dari sisi efisiensi: berapa energi yang dibayar untuk satu tanda tangan yang berhasil dilayani.

**Data:** `runs.csv` kolom `E_lyn_mJ`, atau `deskriptif.csv` baris `metrik == E_lyn_mJ`. Batang galat: `ketidakpastian.csv` kolom `U_gab`.

**Bentuk:** diagram batang rerata dengan batang galat ± U_gab (atau boxplot).

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** energi dinamis per permintaan terlayani (mJ)

**Usulan keterangan:** "Gambar 4.5 Energi dinamis per permintaan terlayani per kelompok (n = 30 run per kelompok)."

**Catatan:**

- WAJIB dibuat ulang setelah kalibrasi shunt.
- Pola yang harus terlihat: energi per layanan hampir tetap terhadap skenario dan hanya ditentukan arsitektur (Software sekitar 41 mJ, DS28E38 sekitar 3 mJ). Ini melengkapi Gambar 4.4: daya dinamis naik pada barrage karena lebih banyak pekerjaan, bukan karena tiap pekerjaan lebih boros.
- Alternatif berbasis arus: muatan per layanan `Q_lyn_mC`. Gambar lama: `gambar/box_E_lyn_mJ.png`, `gambar/box_Q_lyn_mC.png`.

**Tabel pendukung 4.5:**

| Kelompok | E_lyn rerata (mJ) | SD (mJ) | U_gab (mJ) | Q_lyn rerata (mC) |
|---|---|---|---|---|
| K1 | 41,03 | 0,044 | 2,05 | 7,87 |
| K2 | 3,01 | 0,019 | 0,15 | 0,576 |
| K3 | 40,59 | 0,035 | 2,03 | 7,78 |
| K4 | 3,00 | 0,004 | 0,15 | 0,575 |

Penghematan E_lyn: 38,02 mJ atau 92,7 % (Normal); 37,59 mJ atau 92,6 % (Barrage).

---

## Gambar 4.6 Interaksi arsitektur dengan skenario (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_6_interaksi.png` (dua panel; nilai DD dan IK 95 % tertulis di tiap panel; menggantikan `gambar/interaksi_*.png`). Panel (b) dibuat ulang setelah kalibrasi.

**Menjawab:** RM3, apakah manfaat DS28E38 berubah saat diserang.

**Data:** `deskriptif.csv` (rerata empat kelompok untuk `S_cpu_log` dan `P_dyn_mW`); nilai interaksi dan IK 95 % dari `interaksi_dd.csv`.

**Bentuk:** dua panel diagram garis (plot interaksi), panel (a) S_cpu dan panel (b) P_dyn. Tiap panel berisi dua garis.

- **Sumbu X:** skenario (Normal, Barrage), dua titik
- **Sumbu Y:** (a) durasi tidur dalam CPU (%); (b) daya dinamis (mW)
- **Garis:** satu untuk Software (K1 ke K3), satu untuk DS28E38 (K2 ke K4)

**Usulan keterangan:** "Gambar 4.6 Plot interaksi arsitektur dan skenario untuk (a) durasi tidur dalam CPU dan (b) daya dinamis. Garis yang tidak sejajar menunjukkan adanya interaksi."

**Catatan:**

- Cara membacanya: jarak tegak antara dua garis adalah manfaat DS28E38. Di panel (a) jarak melebar saat barrage (manfaat tidur membesar); di panel (b) jarak menyempit sedikit (manfaat daya dinamis mengecil karena K4 melayani jauh lebih banyak).
- Panel (b) dibuat ulang setelah kalibrasi shunt; panel (a) tidak terpengaruh.
- Gambar lama: `gambar/interaksi_S_cpu_log.png`, `gambar/interaksi_P_dyn_mW.png`.

**Tabel pendukung 4.6 (DD = manfaat saat Barrage dikurangi manfaat saat Normal):**

| Metrik | Manfaat Normal | Manfaat Barrage | DD | IK 95 % | Tafsiran |
|---|---|---|---|---|---|
| S_cpu (poin) | 63,08 | 83,59 | +20,51 | [20,50; 20,52] | manfaat lebih besar saat barrage |
| P_dyn (mW) | 37,14 | 35,48 | −1,66 | [−1,69; −1,64] | manfaat lebih kecil saat barrage |
| I_dyn (mA) | 7,12 | 6,80 | −0,32 | [−0,33; −0,32] | manfaat lebih kecil saat barrage |
| E_lyn (mJ) | 38,02 | 37,59 | −0,43 | [−0,45; −0,41] | sedikit lebih kecil (±1,1 % dari manfaatnya) |
| Latensi (ms) | 538,65 | 538,61 | −0,04 | [−0,10; 0,02] | tidak terbukti berbeda |

---

## Gambar 4.7 Tingkat layanan terukur dan model (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_7_tingkat_layanan_vs_model.png`. Skrip: `tools/3_analisis/gambar_bab4_stop.py`. Terukur = batang penuh; model = batang berbingkai; Barrage berarsir.

**Menjawab:** H4, ketahanan layanan saat barrage dan kecocokan dengan model utilisasi.

**Data:** `layanan.csv`, kolom `TL_rerata` (terukur) dan `TL_model` (model).

**Bentuk:** diagram batang berpasangan, dua batang per kelompok.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** tingkat layanan (% permintaan terlayani), rentang 0 sampai 100
- **Legenda:** Terukur, Model

**Usulan keterangan:** "Gambar 4.7 Tingkat layanan terukur dibandingkan dengan model utilisasi TL = min(1, 1/(λ × t_sign))."

**Catatan:**

- Tingkat layanan = permintaan terlayani / permintaan datang. Pada skenario Normal keduanya 100 % (tidak ada permintaan terlewat).
- K4 melayani 85,5 % dan kehilangan 14,5 % permintaan karena beban 10 permintaan/detik sedikit melampaui kapasitas 8,6 tanda tangan/detik (ρ = 1,17). Tegaskan di pembahasan bahwa offloading adalah mitigasi, bukan kekebalan: batasnya waktu tanda tangan IC.
- Selisih terukur terhadap model 0,20 poin (K3) dan 0,33 poin (K4); cukup ditulis di tabel.
- Tidak bergantung pada kalibrasi shunt. Gambar lama `gambar/box_TL_pct.png` hanya berisi nilai terukur.

**Tabel pendukung 4.7:**

| Kelompok | Datang (per run) | Terlayani | Terlewat | TL terukur (%) | TL model (%) | ρ |
|---|---|---|---|---|---|---|
| K1 | 59 | 59 | 0 | 100,00 | 100,00 | 0,64 |
| K2 | 59 | 59 | 0 | 100,00 | 100,00 | 0,11 |
| K3 | 605 | 91 | 514 | 15,04 | 15,24 | 6,56 |
| K4 | 601 sampai 603 | 514 sampai 515 | 86 sampai 88 | 85,50 | 85,83 | 1,17 |

---

## Gambar 4.8 Throughput terukur dan batas model (SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_8_throughput_vs_batas.png`. Dua garis putus-putus penuh menandai kapasitas (Software 1,53/s biru, DS28E38 8,60/s oranye). Skrip: `tools/3_analisis/gambar_bab4_stop.py`.

**Menjawab:** H4 dari sisi kapasitas: berapa tanda tangan per detik yang sanggup dihasilkan.

**Data:** `layanan.csv`, kolom `Theta` (terukur) dan `Theta_maks_model` (batas = 1/t_sign). Titik per run: `runs.csv` kolom `Theta_sps`.

**Bentuk:** diagram batang empat kelompok, ditambah garis putus-putus mendatar untuk batas kapasitas tiap arsitektur.

- **Sumbu X:** kelompok (K1, K2, K3, K4)
- **Sumbu Y:** throughput (tanda tangan/s)
- **Garis acuan:** 1,53 tanda tangan/s (batas Software) dan 8,60 tanda tangan/s (batas DS28E38)

**Usulan keterangan:** "Gambar 4.8 Throughput tanda tangan per kelompok. Garis putus-putus adalah kapasitas maksimum tiap arsitektur (1/t_sign)."

**Catatan:**

- Pada skenario Normal throughput dibatasi laju permintaan (sekitar 0,98 per detik), jadi kedua arsitektur tampak sama. Perbedaan baru muncul saat barrage: keduanya menyentuh batasnya masing-masing (K3 1,507 dari 1,527; K4 8,565 dari 8,598).
- Rasio K4 terhadap K3 = 5,68 kali.
- Skala Y harus muat sampai ±9; batang K1–K3 akan tampak pendek, itu wajar.
- Tidak bergantung pada kalibrasi shunt. Gambar lama: `gambar/box_Theta_sps.png` (tanpa garis batas).

**Tabel pendukung 4.8:**

| Kelompok | Throughput terukur (per s) | Batas model (per s) | Utilisasi ρ |
|---|---|---|---|
| K1 | 0,977 | 1,527 | 0,64 |
| K2 | 0,983 | 8,604 | 0,11 |
| K3 | 1,507 | 1,527 | 6,56 |
| K4 | 8,565 | 8,598 | 1,17 |

---

## Gambar 4.9 Arus diam dan kestabilan antar-blok (BARU, SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_9_arus_diam_antar_blok.png`. Skrip: `tools/3_analisis/gambar_bab4_stop.py`.

**Menjawab:** keabsahan pengukuran (Bab 3.5.4 dan 3.6.4): papan berada dalam keadaan bersih dan tidak bergeser antar-blok.

**Data:** `runs.csv`, baris `kelompok` ∈ {K1-IDLE, K2-IDLE}, kolom `I_avg_mA`, `blok`, `urut`. Delapan titik (dua per blok: awal dan akhir).

**Bentuk:** diagram titik-garis, dua titik per blok dihubungkan garis.

- **Sumbu X:** blok (1 K2S, 2 K1S, 3 K1S, 4 K2S), tiap blok dengan titik "awal" dan "akhir"
- **Sumbu Y:** arus diam (mA), rentang sempit 1,93 sampai 2,05 supaya perbedaan 0,01 mA terlihat dan pita ±0,05 mA tampak
- **Garis acuan:** pita ±0,05 mA di sekitar rerata (batas keabsahan blok)

**Usulan keterangan:** "Gambar 4.9 Arus diam host pada pengukuran IDLE di awal dan akhir tiap blok. Pergeseran terbesar 0,004 mA, jauh di bawah batas keabsahan 0,05 mA."

**Catatan:**

- Pesan utamanya: arus diam 1,99 mA konsisten pada kedua arsitektur dan semua blok, sehingga selisih yang diamati berasal dari prosesor dan chip, bukan dari drift papan.
- Skala Y yang dipersempit harus dinyatakan di keterangan atau sumbu agar tidak menyesatkan.
- Tidak bergantung pada kalibrasi shunt dalam bentuk selisih; nilai mutlak mengikuti kalibrasi.

**Tabel pendukung 4.9:**

| Blok | Firmware | I_IDLE awal (mA) | I_IDLE akhir (mA) | Selisih (mA) | V awal / akhir (V) |
|---|---|---|---|---|---|
| 1 | K2S | 1,990 | 1,990 | +0,001 | 5,207 / 5,217 |
| 2 | K1S | 1,990 | 1,992 | +0,002 | 5,218 / 5,225 |
| 3 | K1S | 1,992 | 1,991 | −0,001 | 5,226 / 5,218 |
| 4 | K2S | 1,989 | 1,984 | −0,004 | 5,214 / 5,217 |

Rerata IDLE: K1S 1,991 mA (10,40 mW), K2S 1,988 mA (10,37 mW).

---

## Gambar 4.10 Frame permintaan hilang pada K4 (BARU, SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_10_frame_hilang_K4.png`. Skrip: `tools/3_analisis/gambar_bab4_stop.py`.

**Menjawab:** batas keabsahan pada Bab 3.5.1 butir 4 dan keterbatasan 1.3 butir 11.

**Data:** `runs.csv`, baris `kelompok == K4` (semua 41 run, termasuk gugur), kolom `rx_lost`, `arrived`, `sah`.

**Bentuk:** diagram batang frekuensi: sumbu X jumlah frame hilang per run (0, 1, 2, 3, 4), sumbu Y jumlah run; batang run sah dan run gugur dengan warna berbeda; garis tegak putus-putus pada batas 0,25 % (±1,5 frame dari ±602 permintaan).

**Usulan keterangan:** "Gambar 4.10 Distribusi jumlah frame permintaan yang hilang per run pada K4 (41 run). Run dengan kehilangan di bawah 0,25 % dianalisis; yang melampaui batas digugurkan."

**Catatan:**

- Frame hilang terjadi ketika frame mulai tepat saat CPU memulihkan jam dari Stop (penerima belum menyala).
- Frame rusak 0 pada semua run. Bangun palsu 0 pada semua run sah.
- K1, K2, K3: frame hilang dan rusak 0.
- Gambar ini melaporkan keterbatasan secara terbuka; jangan dihilangkan.

**Tabel pendukung 4.10:**

| Frame hilang per run | Jumlah run | Status |
|---|---|---|
| 0 | 17 | sah |
| 1 | 13 | sah (< 0,25 %) |
| 2 | 6 | gugur |
| 3 | 3 | gugur |
| 4 | 2 | gugur |
| Jumlah | 41 | 30 sah, 11 gugur |

Total frame hilang pada 30 run sah K4: 13; maksimum per run 0,17 % dari yang tiba.

---

## Gambar 4.11 Kesesuaian dua cara mengukur tidur CPU (BARU, SUDAH DIBUAT)

**Berkas:** `data/analisis/stop_final/gambar_bab4/gambar_4_11_bland_altman_scpu.png` (warna per kelompok; menggantikan `gambar/ba_S_cpu.png`).

**Menjawab:** Bab 3.6.4 (2): S_cpu menurut pin status (logger) terhadap S_cpu menurut firmware.

**Data:** `runs.csv` kolom `S_cpu_log` dan `S_cpu_host` (120 run sah K1–K4); ringkasan di `bland_altman.csv`. Gambar sudah ada: `gambar/ba_S_cpu.png`.

**Bentuk:** plot Bland-Altman: sumbu X rerata kedua cara (%), sumbu Y selisih (pin − firmware, poin persentase); garis bias dan dua garis batas kesesuaian 95 %.

**Usulan keterangan:** "Gambar 4.11 Plot Bland-Altman persentase tidur CPU: pin status (logger) terhadap firmware (n = 120 run)."

**Catatan:** bias −0,014 poin, batas kesesuaian [−0,055; +0,026]. Pesan: dua cara mengukur hampir identik. Skala Y sangat sempit (±0,06); tuliskan.

**Tabel pendukung 4.11:**

| Pasangan | n | Bias | SD selisih | Batas bawah | Batas atas |
|---|---|---|---|---|---|
| S_cpu pin − firmware (poin) | 120 | −0,0142 | 0,0207 | −0,0548 | +0,0263 |

---

## Tabel pendukung umum (bukan grafik)

### Tabel A. Uji hipotesis utama, Software vs DS28E38

Sumber: `banding_sw_hw.csv` (baris dengan `utama == True`). Uji Welch t, koreksi Holm atas 6 perbandingan; keenamnya menolak H0 (p terkoreksi paling besar 6,9e-103; nilai di berkas tertulis 0 karena di bawah presisi). Sebaran kedua kelompok tidak tumpang tindih pada satu pun perbandingan.

| Skenario | Metrik | Software | DS28E38 | Selisih | IK 95 % selisih | Penghematan |
|---|---|---|---|---|---|---|
| Normal | Latensi (ms) | 654,88 | 116,23 | 538,65 | [538,61; 538,69] | 82,25 % |
| Normal | P_dyn (mW) | 40,10 | 2,96 | 37,14 | [37,13; 37,16] | 92,62 % |
| Normal | S_cpu (%) | 35,08 | 98,17 | 63,08 poin | [63,08; 63,09] | |
| Barrage | Latensi (ms) | 654,91 | 116,30 | 538,61 | [538,56; 538,66] | 82,24 % |
| Barrage | P_dyn (mW) | 61,18 | 25,69 | 35,48 | [35,46; 35,50] | 58,00 % |
| Barrage | S_cpu (%) | 0,38 | 83,97 | 83,59 poin | [83,58; 83,60] | |

### Tabel B. Pengaruh barrage pada tiap arsitektur (RM2)

Sumber: `pengaruh_barrage.csv`. Selisih = Barrage dikurangi Normal.

| Metrik | Software (K3 − K1) | DS28E38 (K4 − K2) |
|---|---|---|
| S_cpu (poin) | −34,71 | −14,20 |
| P_dyn (mW) | +21,07 (+52,5 %) | +22,74 (+768,3 %) |
| I_dyn (mA) | +4,04 (+52,5 %) | +4,36 (+770,5 %) |
| E_lyn (mJ) | −0,44 (−1,1 %) | −0,009 (−0,3 %) |
| Tingkat layanan (poin) | −84,96 | −14,50 |
| Throughput (per s) | +0,53 (+54,2 %) | +7,58 (+771,0 %) |
| Latensi (ms) | +0,03 | +0,07 |

### Tabel C. Kondisi diam (acuan P_dyn)

Sumber: `deskriptif.csv`, kelompok K1-IDLE dan K2-IDLE.

| Arsitektur | Jumlah run | Arus diam (mA) | Daya diam (mW) | Tegangan bus (V) |
|---|---|---|---|---|
| Software (K1S) | 4 | 1,991 | 10,40 | 5,222 |
| DS28E38 (K2S) | 4 | 1,988 | 10,37 | 5,214 |

### Tabel D. Sebaran run sah per blok

| Blok | Sesi | K1 | K2 | K3 | K4 |
|---|---|---|---|---|---|
| blok1 | 20261118_115044_K2S | | 15 | | 14 |
| blok2 | 20261118_124646_K1S | 15 | | 15 | |
| blok3 | 20261118_132320_K1S | 15 | | 15 | |
| blok4 | 20261118_140335_K2S | | 15 | | 16 |
| Jumlah | | 30 | 30 | 30 | 30 |

### Tabel E. Kesesuaian latensi dengan episode pin status (skenario Normal)

Sumber: `latensi_pin_vs_dwt.csv` (1.770 tanda tangan per arsitektur).

| Arsitektur | Cara ukur firmware | Bias pin − firmware (ms) | SD selisih (ms) |
|---|---|---|---|
| Software (K1S) | pencacah DWT | +9,29 | 0,06 |
| DS28E38 (K2S) | jam RTC | +3,28 | 0,26 |

Bias tetap ditafsirkan sebagai kerja CPU di luar jendela ukur firmware (pengumpulan sumber acak dan pemulihan jam).

---

## Ringkasan pengerjaan

| Gambar | Status | Terpengaruh kalibrasi shunt |
|---|---|---|
| 4.1 Jejak arus (+ 4.1b rinci) | **dibuat** (`gambar_bab4/`) | ya (skala arus); jalankan ulang skrip dengan `--shunt` |
| 4.2 Latensi | **dibuat** (batang bernilai, `gambar_bab4/`) | tidak |
| 4.3 S_cpu | **dibuat** (`gambar_bab4/`) | tidak |
| 4.4 P_dyn | **dibuat** (`gambar_bab4/`) | ya; skrip dengan `--shunt` |
| 4.5 E_lyn | **dibuat** (`gambar_bab4/`) | ya; skrip dengan `--shunt` |
| 4.6 Interaksi | **dibuat** (`gambar_bab4/`) | panel P_dyn saja |
| 4.7 Tingkat layanan vs model | **dibuat** (`gambar_bab4/`) | tidak |
| 4.8 Throughput vs batas | **dibuat** (dengan garis kapasitas, `gambar_bab4/`) | tidak |
| 4.9 Arus diam antar-blok | **dibuat** (`gambar_bab4/`) | selisih tidak, nilai mutlak ya |
| 4.10 Frame hilang K4 | **dibuat** (`gambar_bab4/`) | tidak |
| 4.11 Bland-Altman S_cpu | **dibuat** (`gambar_bab4/`) | tidak |

**Semua gambar inti (4.1–4.11) kini seragam** (warna per arsitektur, Barrage berarsir, desimal koma, 300 dpi) dan dibuat satu skrip: `python tools/3_analisis/gambar_bab4_stop.py [--shunt <nilai>]` → `data/analisis/stop_final/gambar_bab4/`. Gambar keluaran `analisis_d.py` di `gambar/` (boxplot, plot interaksi, drift) tetap ada dan hanya dipakai sebagai lampiran: `drift_*.png`, `box_I_avg_mA`, `box_P_avg_mW`, `box_E_mJ`, `box_Q_lyn_mC`.

Setelah kalibrasi shunt selesai: jalankan ulang skrip dengan `--shunt <nilai>` (memperbarui 4.1, 4.1b), jalankan ulang `python tools/3_analisis/analisis_d.py --manifest data/tahap_stop.txt --stop --rshunt <nilai> --keluar data/analisis/stop_final` agar `ketidakpastian.csv` dan `deskriptif.csv` ikut berubah, lalu jalankan skrip gambar lagi (4.4, 4.5, 4.6b, 4.9 membaca berkas itu).
