# Revisi Bab 3 (PROPO_BAB1-3_FIX_5 → versi data *deep sleep*)

Tanggal: 18 November 2026
Acuan: `paper/PROPO_BAB1-3_FIX_5.pdf`, Bab 3 (halaman 20–36). Dasar revisi: data final varian *deep sleep* (Stop mode) `data/tahap_stop.txt`, analisis `data/analisis/stop_final/`, logbook 17–18 November 2026, dan `laporan/laporan_rekap_logbook_pendukung_bab4_2026-11-18.md`.

Aturan penulisan revisi ini: **hanya data dan analisis varian *deep sleep* yang disebut sebagai hasil.** Teks pengganti tidak memuat perbandingan dengan data pengembangan awal (Sleep/WFI). Angka yang dipakai berasal dari uji pendahuluan pada firmware final varian *deep sleep*.

Format: tiap butir menyebut **lokasi**, **teks FIX_5** (diringkas), **teks pengganti** (siap tempel; bahasa baku seperti naskah), dan **alasan/sumber**. Nilai arus, daya, dan energi mutlak masih memakai shunt nominal 0,100 Ω dan diperbarui setelah kalibrasi (butir 33). Persentase, latensi, S_CPU, dan TL tidak berubah oleh kalibrasi.

---

## Ringkasan perubahan

| No | Sub-bab | Jenis | Isi singkat |
|---|---|---|---|
| 1 | 3.1 par. 1 | Ubah | Desain dan blok: urutan ABBA K2S–K1S–K1S–K2S, prosedur keadaan bersih (bukan lagi "ST-Link dilepas") |
| 2 | 3.1 butir metrik 3 | Ubah | "mode sleep" menjadi mode tidur dalam (Stop) |
| 3 | 3.1 par. tahapan | Ubah | Tahap pengembangan varian *deep sleep* dan uji dasar |
| 4 | Gambar 3.1 | Ubah | Diagram alir: tambah tahap uji dasar Stop, overdrive, uji jalur galat |
| 5 | 3.1 indikator capaian | Ubah | Latensi uji pendahuluan, ketahanan, galat |
| 6 | Tabel 3.1 | Ubah | Host (RTC/LSE, XC6219, BOOT0), attacker (10 kΩ), ST-Link, osiloskop |
| 7 | Tabel 3.2 | Ubah | Mbed TLS konfigurasi 1c, driver 1-Wire overdrive, skrip pendukung, hapus Jamovi |
| 8 | 3.3 kategori data | Ubah | Latensi (DWT/RTC), durasi tidur (Stop), frame UART |
| 9 | 3.3 variabel | Ubah | Variabel kontrol: Stop, overdrive, keadaan bersih, konfigurasi software |
| 10 | 3.4 par. lantai arus | Tambah | Lantai papan 1,99 mA dan LED daya |
| 11 | 3.4.1 a | **Tulis ulang** | Sub-sistem host: Stop, RTC, bangun lewat sinkron bit |
| 12 | 3.4.1 b | Ubah | DS28E38: overdrive, polling, tunggu dalam Stop |
| 13 | 3.4.1 c | **Tulis ulang** | Debugging: tanpa sesi debugger, POR penuh, larik 600 |
| 14 | 3.4.1 d | Ubah | Pin status PB1: 0 saat Stop |
| 15 | 3.4.1 e | Ubah | Logger/attacker: resistor seri 10 kΩ |
| 16 | Tabel 3.3 | Ubah | Resistor 10 kΩ pada dua jalur, kristal LSE, konfirmasi pull-up |
| 17 | 3.5 par. 1 | Ubah | Alur run dengan Stop |
| 18 | 3.5.1 (1) | **Tulis ulang** | Latensi: DWT (K1) dan RTC (K2) |
| 19 | 3.5.1 (2) | Ubah | Jendela T_obs dan koreksi jam |
| 20 | 3.5.1 (3) | **Tulis ulang** | Durasi tidur dari RTC |
| 21 | 3.5.1 (4) | Ubah | Tambah frame UART rusak/hilang dan toleransi |
| 22 | 3.5.2 | Ubah | Kapasitas dan utilisasi dengan latensi final |
| 23 | 3.5.3 + Tabel 3.4 | Ubah | Blok, jumlah run, aturan gugur |
| 24 | 3.5.4 (baru) | Tambah | Prosedur keadaan bersih dan pemrograman |
| 25 | 3.5.5 (baru) | Tambah | Uji ketahanan dan jalur galat |
| 26 | 3.6 par. pembuka | Ubah | Tambah kriteria keabsahan data |
| 27 | 3.6.3 | Ubah kecil | Hipotesis dan H4 tetap; tambah catatan keabsahan |
| 28 | 3.6.4 | Ubah | Pasangan Bland-Altman dengan RTC |
| 29 | 3.6.5 | Ubah kecil | Ketidakpastian kalibrasi: nyatakan statusnya |
| 30 | Bab 4 (Jadwal) | Catatan | Tambah kegiatan |
| 31 | Daftar pustaka | Tambah | Referensi Stop, RTC, osilator |
| 32 | Lintas bab | Catatan | Konsistensi dengan Bab 1–2 |
| 33 | Periksa | Daftar | Hal yang harus Anda pastikan sebelum menempel |

---

## 1. Sub-bab 3.1, paragraf pertama (desain dan blok)

**FIX_5:** "...Setiap unit eksperimen berupa satu run berdurasi 60 detik dengan 30 kali repetisi per kelompok (total 120 run). Untuk mencegah pengaruh pemasangan ST-Link terhadap pembacaan arus, run disusun dalam empat blok berselang-seling menurut arsitektur (Sub-bab 3.5.3) dengan urutan skenario diacak di dalam tiap blok..."

**Pengganti:**
> Penelitian ini menerapkan eksperimen kuantitatif komparatif dengan rancangan faktorial 2 × 2 di Laboratorium Elektronika dan Instrumentasi, Jurusan Ilmu Komputer dan Elektronika (DIKE), FMIPA, Universitas Gadjah Mada (UGM), Yogyakarta. Dua faktor yang diuji adalah arsitektur keamanan (software-based ECDSA P-256 pada STM32F103 dan hardware offloading ke IC DS28E38) serta skenario beban (kondisi normal dan barrage attack), yang membentuk empat kelompok uji (K1–K4). Pada seluruh kelompok, CPU host memasuki mode tidur dalam (Stop mode) di antara kejadian. Setiap unit eksperimen berupa satu run berdurasi 60 detik dengan 30 kali repetisi sah per kelompok (total 120 run sah). Karena mode Stop sangat peka terhadap keadaan debugger dan sisa daya pada jalur sinyal, arus tidur hanya sah diukur pada papan dalam keadaan bersih, yaitu setelah daya papan dimatikan penuh (power-on reset) dan tanpa sesi debugger sama sekali (Sub-bab 3.5.4). Karena itu run disusun dalam empat blok berselang-seling menurut arsitektur (Sub-bab 3.5.3), dengan urutan skenario diacak di dalam tiap blok agar efek drift suhu ruang maupun instrumen tidak terkonsentrasi pada salah satu arsitektur. Empat metrik utama yang diukur pada setiap run adalah:

**Alasan/sumber:** logbook 18 Nov (debug power: lantai 1,94 → 2,52 mA setelah satu sesi OpenOCD); rekap 18 Nov Bagian 7.5. Jumlah run sah tetap 120 (30 × 4); jumlah run total lebih besar karena run gugur (Butir 23).

---

## 2. Sub-bab 3.1, butir metrik ke-3

**FIX_5:** "3. Persentase durasi CPU pada mode sleep."

**Pengganti:**
> 3. Persentase durasi CPU pada mode tidur dalam (Stop mode).

---

## 3. Sub-bab 3.1, paragraf tahapan penelitian

**FIX_5:** "...Tahap berikutnya meliputi perancangan papan prototipe STM32F103-DS28E38, pengembangan firmware software-based dan hardware offloading, perakitan sistem ukur INA226 logger, serta penyusunan attacker node. Setelah kalibrasi instrumen dan uji pendahuluan untuk menetapkan laju uji (Sub-bab 3.5.2), dilakukan pengambilan data utama sebanyak 120 run..."

**Pengganti:**
> Tahap berikutnya meliputi perancangan papan prototipe STM32F103-DS28E38, pengembangan firmware software-based dan hardware offloading dengan mode tidur dalam, perakitan sistem ukur INA226 logger, serta penyusunan attacker node. Pengembangan mode tidur dalam dilakukan bertahap dan setiap tahap diuji sampai stabil sebelum fungsi berikutnya ditambahkan: uji osilator jam RTC (LSE), uji arus Stop minimal, uji bangun oleh alarm RTC, uji bangun oleh permintaan attacker node tanpa mengubah attacker node, pembuatan driver 1-Wire berkecepatan tinggi (overdrive) untuk DS28E38, serta integrasi ke kedua firmware. Setelah kalibrasi instrumen dan uji pendahuluan untuk menetapkan laju uji (Sub-bab 3.5.2), dilakukan pengambilan data utama sebanyak 120 run sah (30 run per kelompok K1–K4), disusul uji ketahanan dan uji jalur galat (Sub-bab 3.5.5). Data yang tervalidasi kemudian diolah melalui statistik deskriptif, gain analysis, uji beda statistik, dan analisis kesesuaian instrumen untuk menarik kesimpulan penelitian.

**Alasan/sumber:** logbook 17–18 Nov; langkah 1–12 pada `docs/rencana_deep_sleep.md`.

---

## 4. Gambar 3.1 (Diagram Alir Tahapan Penelitian)

**FIX_5:** diagram alir tahapan umum (studi literatur → perancangan → kalibrasi/uji pendahuluan → pengambilan data → analisis).

**Perubahan yang perlu digambar ulang:**
1. Setelah blok "Pengembangan firmware" tambahkan blok **"Uji dasar mode tidur dalam"** (osilator RTC, arus Stop minimal, alarm RTC, bangun oleh permintaan) dan **"Uji overdrive DS28E38"**.
2. Setelah "Pengambilan data" tambahkan blok **"Uji ketahanan dan jalur galat"**.
3. Pada blok "Pengambilan data" tulis "4 blok ABBA, keadaan bersih (POR penuh, tanpa debugger)".

**Alasan:** diagram harus mencerminkan tahapan yang benar-benar dikerjakan (logbook 17–18 Nov).

---

## 5. Sub-bab 3.1, indikator capaian terukur

**FIX_5:** butir (3) "latensi uji pendahuluan (~655,5 ms dan ~202,2 ms) dipakai untuk menetapkan dua laju uji"; butir (4) "120 run valid ... pergeseran IDLE awal-akhir setiap blok tidak melebihi 0,05 mA."

**Pengganti (butir 2–5 ditulis ulang):**
> (2) Perancangan dan perakitan: prototipe, logger, dan attacker node berfungsi, kedua arsitektur menghasilkan tanda tangan ECDSA P-256 sebesar 64 byte, dan host dapat tidur dalam (Stop) serta bangun oleh alarm RTC maupun permintaan attacker node tanpa kehilangan frame pada laju 1 dan 10 permintaan/detik. (3) Kalibrasi dan uji pendahuluan: bias dan batas kesesuaian INA226 terhadap multimeter diperoleh (Sub-bab 3.6.4), dan latensi uji pendahuluan (~654,9 ms untuk software-based dan ~116,2 ms untuk hardware offloading) dipakai untuk menetapkan dua laju uji (Sub-bab 3.5.2). (4) Pengambilan data: 120 run sah, yaitu 30 run per kelompok K1–K4, dengan pergeseran IDLE awal-akhir setiap blok tidak melebihi 0,05 mA dan tanpa frame permintaan rusak. (5) Ketahanan: tidak ada kegagalan sistem (crash atau macet) selama seluruh run, dan host bereaksi benar pada uji jalur galat (Sub-bab 3.5.5). (6) Analisis: nilai p terkoreksi Holm, selisih beserta interval kepercayaan 95% bootstrap, dan ketidakpastian gabungan Ugab dilaporkan untuk setiap metrik utama, dan tingkat layanan terukur berbeda tidak lebih dari 1 poin persentase dari model utilisasi (Sub-bab 3.6.3 butir 5).

**Alasan/sumber:** `ringkasan.md` stop_final (latensi 654,88 dan 116,23 ms; IDLE drift 0,004 mA); logbook 18 Nov (S2: 59/59 dan 602/602).

---

## 6. Tabel 3.1, perangkat keras

**Baris 1 (Papan prototipe host)**

FIX_5: "...clock inti 48 MHz (HSI-PLL), input USB Type-C 5 V, header SWD (UPLOAD), jumper BOOT, dua LED indikator, serta jalur 1-Wire pada pin PA15 (pull-up 4,7 kΩ ke 3,3 V)." Fungsi: "...pencacah DWT, dan manajemen mode tidur."

**Pengganti spesifikasi:**
> Papan prototipe PCB dengan modul STM32 (STM32F103C8T6, ARM Cortex-M3 32-bit, 64 KB Flash, 20 KB SRAM, regulator 3,3 V XC6219B332MR), clock inti 48 MHz (HSI-PLL; setelah bangun dari Stop prosesor berjalan pada HSI 8 MHz lalu PLL dipulihkan), kristal jam 32,768 kHz pada PC14/PC15 untuk RTC, input USB Type-C 5 V (hanya daya), header SWD (UPLOAD), jumper BOOT (dipakai hanya untuk pemulihan), LED daya, serta jalur 1-Wire pada pin PA15 (pull-up 4,7 kΩ ke 3,3 V).

**Pengganti fungsi:**
> Unit uji utama yang menjalankan firmware, ECDSA perangkat lunak, komunikasi 1-Wire, pencacah DWT (saat terjaga), jam RTC, dan manajemen mode tidur dalam (Stop).

**Baris 3 (Attacker node):** ganti "melalui resistor seri 1 kΩ" menjadi "melalui resistor seri 10 kΩ".

**Baris 6 (Logger):** tambahkan di spesifikasi "...meneruskan perintah mulai ke host via USART1 (resistor seri 10 kΩ pada jalur ke host)...".

**Baris 8 (Osiloskop):** pertahankan hanya bila benar dipakai. **Periksa** (Butir 33): apakah osiloskop dipakai pada sesi validasi; catatan logbook yang saya baca tidak menyebut pemakaiannya. Bila tidak dipakai, hapus baris ini atau tulis "tidak dipakai pada pengambilan data".

**Baris 9 (Peralatan pendukung):** tambahkan "ST-Link V2 hanya untuk pemrograman dan pemulihan, tidak dioperasikan selama pengukuran".

**Alasan/sumber:** `docs/pinout_arsitektur_pengujian.md` (bagian 2, 2b), logbook 17 Nov (resistor seri 10 kΩ, regulator XC6219B332MR, kristal Epson MC-306).

---

## 7. Tabel 3.2, perangkat lunak

| Baris | FIX_5 | Pengganti |
|---|---|---|
| 1 | PlatformIO ... OpenOCD | PlatformIO (VS Code), core STM32duino, GCC ARM, OpenOCD. **Tambahkan:** "Skrip pemrograman host yang sedang tidur dan skrip pemulihan lewat bootloader (BOOT0)." |
| 2 | Mbed TLS 3.6.7 ... nonce hedged (HMAC-DRBG) | Mbed TLS 3.6.7 (Mbed TLS Contributors, 2026) dengan tabel percepatan comb dimatikan (konfigurasi 1c) dan fitur nonce hedged (HMAC-DRBG); pilihan konfigurasi ini memengaruhi latensi perangkat lunak dan dinyatakan sebagai batasan (Sub-bab 1.3). |
| 3 | Driver 1-Wire DS28E38 | Driver 1-Wire DS28E38 buatan sendiri (bit-banging pada PA15) dengan mode overdrive dan penantian hasil berbasis polling; mengeksekusi perintah Compute and Read Page Authentication. |
| 4 | Firmware logger | (tidak berubah) |
| 5 | Firmware attacker node | (tidak berubah) |
| 6 | Skrip Python (pyserial) | Skrip Python (pyserial): kendali otomatisasi pengujian, penyimpanan CSV per run, validasi keutuhan data, dan uji jalur galat. |
| 7 | Python (SciPy, statsmodels) / Jamovi | Python (NumPy, pandas, SciPy, statsmodels): uji beda statistik (Welch t, Mann-Whitney U), interval kepercayaan bootstrap, dan analisis Bland-Altman. **Hapus "Jamovi"** bila tidak dipakai (analisis dikerjakan penuh dengan Python). |
| 8 | DS28E38 Evaluation Kit | (tidak berubah) |

**Alasan:** konfigurasi 1c belum disebut di FIX_5 padahal menentukan besar keunggulan (latensi 654,9 ms); driver overdrive adalah komponen baru; Jamovi tidak digunakan.

---

## 8. Sub-bab 3.3, kategori data

**Butir 1 (latensi), FIX_5:** "...diukur menggunakan pencacah DWT CPU dan durasi episode pin status pada logger dipakai sebagai pembanding independen pada skenario normal."

**Pengganti:**
> 1. Data latensi pemrosesan (signing latency): Durasi (ms) sejak permintaan mulai diproses hingga tanda tangan ECDSA P-256 (r, s) tersedia di memori host. Pada perlakuan software-based durasi diukur dengan pencacah siklus DWT CPU; pada perlakuan hardware offloading, karena pencacah DWT berhenti selama CPU berada dalam Stop, durasi diukur dengan jam RTC berkristal 32,768 kHz (resolusi 30,5 µs). Durasi episode pin status pada logger dipakai sebagai pembanding independen pada skenario normal.

**Butir 3 (durasi tidur), FIX_5:** "Proporsi waktu (%) CPU berada pada status tidur (instruksi WFI)..."

**Pengganti:**
> 3. Data persentase durasi tidur dalam CPU: Proporsi waktu (%) CPU berada pada status Stop terhadap total jendela pengamatan, dihitung dari jam RTC dan divalidasi terhadap siklus kerja pin status yang direkam logger.

**Butir 4 (tingkat layanan):** tambahkan di akhir: "Jumlah frame permintaan yang rusak atau hilang pada jalur UART antara attacker node dan host juga dicatat sebagai syarat keabsahan run."

---

## 9. Sub-bab 3.3, variabel penelitian

**Variabel kontrol, FIX_5:** "Masukan 32 byte konstan, clock STM32F103 48 MHz, catu daya USB 5 V, antarmuka 1-Wire pull-up 4,7 kΩ, kebijakan WFI, serta keadaan LED indikator yang sama pada semua run."

**Pengganti:**
> 3. Variabel kontrol: Masukan 32 byte konstan, clock STM32F103 48 MHz (setelah pemulihan dari Stop), catu daya USB 5 V, antarmuka 1-Wire pull-up 4,7 kΩ pada kecepatan overdrive, kebijakan mode tidur dalam (Stop) dengan sumber bangun yang sama pada kedua arsitektur, konfigurasi Mbed TLS yang sama (tabel comb dimatikan), keadaan papan bersih (daya dimatikan penuh dan tanpa sesi debugger), serta keadaan LED indikator yang sama pada semua run.

**Variabel terikat:** tambahkan "jumlah frame permintaan rusak dan hilang" sebagai variabel pemeriksaan keabsahan (bukan variabel hipotesis).

---

## 10. Sub-bab 3.4, paragraf nilai mutlak daya

**FIX_5:** "Karena pengukuran dilakukan pada input USB Type-C (5 V), nilai daya mencakup komponen filter, regulator modul Blue Pill, LED, dan seluruh beban papan..."

**Pengganti (tambahan di akhir paragraf):**
> Pada mode tidur dalam, arus diam papan terukur ±1,99 mA (±10,4 mW pada 5,2 V) dan didominasi oleh LED daya papan, bukan oleh mikrokontroler yang sedang Stop (puluhan mikroampere). Nilai ini sama pada kedua arsitektur (selisih < 0,003 mA) dan sama dengan arus diam host yang berdiri sendiri, sehingga penyerang, logger, dan sensor tidak menyumbang arus pada jalur ukur. LED daya sengaja dipertahankan agar beban tetap papan sama pada semua run; konsekuensinya, nilai mutlak mencerminkan daya tingkat papan, bukan konsumsi cip.

**Alasan/sumber:** logbook 17 Nov (S0: 1,985 mA, LED daya merah ≈ 1,9 mA, keputusan peneliti), `pemeriksaan.md` (IDLE 1,984–1,992 mA).

---

## 11. Sub-bab 3.4.1 a (sub-sistem host) — tulis ulang

**FIX_5:** "Dioperasikan pada 48 MHz. CPU masuk mode sleep (WFI) di antara permintaan dan hanya dibangunkan oleh interupsi timer atau DMA UART2. Selama DS28E38 memproses tanda tangan, CPU menunggu dalam WFI menggunakan interupsi timer. Pin PA15 dikonfigurasi untuk 1-Wire dengan menonaktifkan fungsi JTAG (SWD tetap aktif)."

**Pengganti:**
> a. Sub-sistem mikrokontroler host (STM32F103): Dioperasikan pada 48 MHz saat terjaga. Di antara permintaan CPU masuk mode tidur dalam (Stop): jam utama, bus, dan hampir seluruh periferal mati, kecuali jam RTC berkristal 32,768 kHz. Dua sumber bangun dipakai, keduanya sebagai event EXTI: alarm RTC (penanda akhir jendela 60 detik dan, pada perlakuan hardware offloading, penanda selesainya komputasi IC) dan tepi turun pada pin PA3 (tanda awal frame permintaan dari attacker node). Karena UART tidak aktif selama Stop, awal frame yang membangunkan CPU sudah terlewat; host mengenali pola tepi byte pertama frame (0xA5) lalu menyalakan penerima sekitar 17 µs sebelum byte kedua sehingga frame diterima utuh tanpa mengubah attacker node. Setelah bangun, CPU berjalan pada HSI 8 MHz dan PLL 48 MHz dipulihkan. Pencacah DWT berhenti selama Stop, sehingga lama tidur dan durasi jendela dihitung dari RTC. Pin PA15 dikonfigurasi untuk 1-Wire dengan menonaktifkan fungsi JTAG (SWD tetap terkonfigurasi). Seluruh pin yang tidak dipakai diatur sebagai analog dan USB dimatikan untuk menekan arus bocor.

**Alasan/sumber:** `src/host/deepsleep/stop_common.h`, logbook 18 Nov (S2: 59/59, 602/602).

---

## 12. Sub-bab 3.4.1 b (DS28E38)

**FIX_5:** "...strong pull-up aktif melalui pin PA15 (mode push-pull tinggi setelah byte release) untuk memenuhi kebutuhan tegangan ≥ 2,8 V dan arus ~10 mA selama komputasi... Perintah Compute and Read Page Authentication (A5h) menerima challenge 32 byte dan menghasilkan tanda tangan 64 byte (r, s)."

**Pengganti (tambahkan sesudah kalimat terakhir):**
> Komunikasi dengan IC dilakukan pada kecepatan overdrive: perintah Overdrive Skip ROM dikirim pada kecepatan standar, kemudian seluruh transaksi berikutnya memakai pewaktuan overdrive (titik pembacaan bit 2,5 µs setelah tepi turun) dengan bit-banging pada PA15. Komputasi tanda tangan di dalam IC berlangsung sekitar 106,6 ms (rentang 105,9–107,3 ms). Selama itu CPU tidur dalam (Stop) dan dibangunkan oleh alarm RTC sekitar 100 ms setelah perintah dikirim; hasil kemudian dinantikan dengan polling singkat sebelum dibaca. Waktu total satu tanda tangan sekitar 116 ms (kirim ±3,4 ms, komputasi IC ±106,6 ms, baca ±5,6 ms).

**Alasan/sumber:** logbook 18 Nov (karakterisasi overdrive: 115,6 ms, 269 dari 269 tanda tangan valid, busy 105,9–107,3 ms); `src/host/deepsleep/ds28e38_od.h`.

---

## 13. Sub-bab 3.4.1 c (debugging) — tulis ulang

**FIX_5:** "ST-Link dilepas selama pengukuran. Selain itu, register DBGMCU_CR selalu dinolkan setelah pemrograman agar tidak menahan clock inti tetap aktif saat WFI. Data latensi tiap permintaan disimpan sementara di larik RAM (400 × 4 byte)..."

**Pengganti:**
> c. Sub-sistem debugging: ST-Link dipakai hanya untuk pemrograman dan pemulihan, dan tidak dioperasikan selama pengukuran. Pengamatan menunjukkan bahwa satu sesi debugger saja menyalakan bagian debug pada cip dan menaikkan arus diam (dari ±1,94 mA menjadi ±2,52 mA) serta mengubah perilaku bangun dari Stop sampai daya papan dimatikan penuh. Karena itu setiap pemrograman diikuti prosedur keadaan bersih (Sub-bab 3.5.4). Register DBGMCU_CR dinolkan oleh firmware setelah jendela aman 5 detik setelah boot agar tidak menahan clock inti tetap aktif saat Stop. Selama Stop jalur SWD tidak dapat tersambung; pemulihan papan yang terkunci memakai jumper BOOT. Data latensi tiap permintaan disimpan sementara di larik RAM (600 × 4 byte) selama jendela 60 detik dan dikirim ke logger setelah jendela berakhir agar transmisi serial tidak mengganggu pengukuran.

**Alasan/sumber:** logbook 17 Nov (lantai 1,985 vs 2,567 mA), 18 Nov (debug power); `HL_LAT_MAX = 600` di `stop_link.h` (K2 melayani ±515 tanda tangan per jendela).

---

## 14. Sub-bab 3.4.1 d (pin status)

**FIX_5:** "Pin GPIO status diatur berlogika 1 saat CPU aktif dan 0 saat WFI..."

**Pengganti:**
> d. Sub-sistem manajemen daya dan titik ukur: Pin GPIO status (PB1) diatur berlogika 1 saat CPU aktif dan 0 saat CPU memasuki Stop, untuk validasi silang durasi tidur dan latensi oleh logger, sementara seluruh LED indikator dijaga dalam kondisi tetap.

---

## 15. Sub-bab 3.4.1 e (logger dan attacker)

**FIX_5:** "...resistor seri ~1 kΩ sebagai pembatas arus proteksi..."

**Pengganti:** ganti "~1 kΩ" menjadi "10 kΩ". Tambahkan: "Resistor seri 10 kΩ juga dipasang pada jalur logger ke host (USART1) untuk membatasi arus balik antar-papan."

**Alasan:** logbook 17 Nov (resistor seri 10 kΩ pada kedua jalur sinyal untuk membatasi daya balik lewat dioda proteksi pin).

---

## 16. Tabel 3.3 (pengkabelan)

| Baris | Perubahan |
|---|---|
| 1 | Tambahkan: "melalui resistor seri 10 kΩ" (host PA9 → logger PA10 bila dipasang pada sisi ini; sesuaikan dengan bangku) |
| 2 | Tambahkan: "melalui resistor seri 10 kΩ" (logger PA9 → host PA10) |
| 8 | Nilai "pull-up 4,7 kΩ" **perlu diverifikasi** (Butir 33): catatan bangku menandainya sebagai dugaan |
| 9 | Ganti "R ~1 kΩ" → "R 10 kΩ" |
| baru | "Host PC14/PC15 — kristal 32,768 kHz — Sumber jam RTC (jangan disentuh probe)" |
| baru | "Host BOOT0 — jumper (GND saat normal, 3V3 saat pemulihan)" |

**Alasan:** `docs/pinout_arsitektur_pengujian.md` bagian 2 dan 2b.

---

## 17. Sub-bab 3.5, paragraf pembuka alur run

**FIX_5:** "...Selama pengamatan, CPU berada pada mode WFI dan hanya bangun ketika interupsi permintaan tiba pada UART2 untuk menaikkan pin status..."

**Pengganti:**
> ...Setelah papan membalas tanda mulai, PC mengaktifkan attacker node sesuai laju skenario selama jendela 60 detik. Selama pengamatan, CPU berada dalam mode tidur dalam (Stop) dan hanya bangun karena dua hal: tepi turun pada PA3 saat frame permintaan tiba (penerima UART dinyalakan melalui pengenalan pola byte pertama), atau alarm RTC (selesainya komputasi DS28E38 atau akhir jendela). Setelah bangun, CPU menaikkan pin status, memulihkan jam, menerima frame, mengeksekusi penandatanganan ECDSA (CPU atau DS28E38), mencatat durasi, dan kembali ke Stop, sementara logger merekam data INA226 serta sinyal pin status. Setiap blok pengukuran diapit oleh run tanpa permintaan (IDLE) selama 60 detik untuk mengukur daya diam Pidle (dengan DS28E38 tetap terpasang dan tercatu) guna mengoreksi pergeseran baseline antar-blok.

---

## 18. Sub-bab 3.5.1 butir 1 (latensi) — tulis ulang

**FIX_5:** "Register CYCCNT pada unit DWT Cortex-M3 diaktifkan melalui bit TRCENA dan CYCCNTENA... Tsign = (Nend − Nstart)/fCPU × 1000 (ms) dengan fCPU = 48 MHz. Nilai Tsign dikoreksi dengan rasio durasi jendela logger terhadap durasi jendela papan."

**Pengganti:**
> 1. Pengukuran latensi pemrosesan:
>    a. Perlakuan software-based memakai Data Watchpoint and Trace (DWT). Register CYCCNT pada unit DWT Cortex-M3 diaktifkan melalui bit TRCENA dan CYCCNTENA (ARM Limited, 2006). Nilai CYCCNT dibaca tepat sebelum (Nstart) dan sesudah (Nend) pembuatan tanda tangan, kemudian dihitung Tsign = (Nend − Nstart) / fCPU × 1000 (ms). Karena frekuensi inti sebenarnya sedikit berbeda dari nominal, fCPU diukur terhadap RTC selama 2 detik setelah setiap jendela dan dipakai pada perhitungan.
>    b. Perlakuan hardware offloading melibatkan tidur dalam selama IC bekerja, sedangkan CYCCNT berhenti pada kondisi tersebut. Karena itu Tsign dihitung dari jam RTC: Tsign = (Fakhir − Fawal) / 32768 × 1000 (ms), dengan F pembacaan RTC beresolusi 1/32768 detik (30,5 µs; dua register CNT dan DIV).
>    c. Nilai Tsign dikoreksi dengan rasio durasi jendela logger terhadap durasi jendela papan. Durasi episode pin status yang ditangkap logger disimpan pada seluruh run sebagai pembanding independen pada skenario normal.

**Alasan/sumber:** `stop_window.h` (`measure_hclk`, `STOP_LAT_RTC`), logbook 18 Nov (DWT berhenti di Stop).

---

## 19. Sub-bab 3.5.1 butir 2 (arus, daya, energi)

**FIX_5:** "...dengan Tobs lama jendela menurut logger (~60,3-60,5 detik) dan Nreq jumlah tanda tangan selesai."

**Pengganti:** ganti "(~60,3–60,5 detik)" dengan "(60,00–60,43 detik)". Rumus lain tidak berubah.

**Catatan:** kalimat "Pembacaan arus INA226 dikalibrasi terhadap multimeter pada kondisi arus tunak (Tabel 3.1 no. 7)..." hanya boleh dipertahankan bila kalibrasi sudah dilakukan; lihat Butir 33.

---

## 20. Sub-bab 3.5.1 butir 3 (durasi tidur) — tulis ulang

**FIX_5:** "Durasi tidur diakumulasikan dari selisih cap waktu timer perangkat keras sebelum dan sesudah instruksi WFI (Tsleep)..."

**Pengganti:**
> 3. Pengukuran persentase durasi tidur dalam CPU: Durasi tidur diakumulasikan dari selisih pembacaan jam RTC sebelum perintah masuk Stop dan sesudah bangun, dikurangi waktu terjaga sejak bangun sampai pembacaan (Tsleep), lalu divalidasi terhadap siklus kerja pin status pada logger:
>    SCPU = Tsleep / Tobs × 100%.
> Durasi jendela Tobs dihitung dari RTC dan dikoreksi terhadap jam kristal logger. Konsistensi data diperiksa menggunakan model duty cycle Iavg = Iaktif · D + Itidur · (1 − D) dengan D = 1 − SCPU/100.

**Alasan:** TIM2/TIM3/TIM4 dan SysTick berhenti di Stop; RTC adalah satu-satunya jam yang berjalan. Hasil: S_CPU logger vs firmware selisih ≤ 0,08 poin (`pemeriksaan.md`).

---

## 21. Sub-bab 3.5.1 butir 4 (tingkat layanan)

**Tambahan di akhir butir:**
> Selain itu firmware mencatat jumlah frame permintaan yang rusak (checksum atau penanda awal salah) dan yang hilang (celah nomor urut) pada jalur UART. Frame rusak tidak diperbolehkan pada run sah. Pada skenario barrage, frame yang hilang diperbolehkan kurang dari 0,25% dari jumlah permintaan yang tiba pada run tersebut; batas ini ditetapkan karena frame yang mulai tepat saat CPU memulihkan jam dari Stop tidak dapat diterima oleh penerima yang belum menyala. Jumlah frame hilang tetap dilaporkan.

**Alasan:** keputusan peneliti 18 Nov (toleransi < 0,25%); logbook 18 Nov bagian 8.2.

---

## 22. Sub-bab 3.5.2 (skenario pengujian)

**FIX_5:** "Berdasarkan latensi uji pendahuluan (~655,5 ms ... dan ~202,2 ms ...), kapasitas layanan maksimum ... ~1,53 dan ~4,95 permintaan/detik... (utilisasi ρ = 0,66 dan 0,20)... (utilisasi ρ = 6,6 dan 2,0)."

**Pengganti:**
> Berdasarkan latensi uji pendahuluan (~654,9 ms pada software-based dan ~116,2 ms pada hardware offloading), kapasitas layanan maksimum masing-masing arsitektur adalah ~1,53 dan ~8,60 permintaan/detik. Oleh karena itu, ditetapkan dua laju uji:
> 1. Skenario A: Kondisi normal (1 permintaan/detik). Berada di bawah kapasitas kedua arsitektur (utilisasi ρ = 0,65 dan 0,12) sehingga seluruh permintaan yang tiba (~59 frame per jendela 60 detik) terlayani setara. Prosedur diulang 30 kali per arsitektur.
> 2. Skenario B: Kondisi barrage attack (10 permintaan/detik). Melampaui kapasitas kedua arsitektur (utilisasi ρ = 6,6 dan 1,2); pada hardware offloading kelebihan beban hanya sekitar 16%. Permintaan yang datang saat CPU atau DS28E38 masih sibuk digabung menjadi maksimal satu permintaan tertunda (pending) dan selebihnya tercatat terlewat. Prosedur diulang 30 kali per arsitektur.

**Alasan/sumber:** `ringkasan.md` stop_final Bagian 6 (kapasitas 1,527 dan 8,60; TL model 15,24% dan 85,83%).

---

## 23. Sub-bab 3.5.3 dan Tabel 3.4

**FIX_5:** "Pengambilan data disusun dalam empat blok yang berselang-seling menurut arsitektur (Blok 1: software-based, Blok 2: hardware offloading, Blok 3: software-based, Blok 4: hardware offloading) sehingga pemrograman ulang firmware hanya dilakukan tiga kali... setiap run yang mengalami kehilangan baris data, galat checksum/I²C, dan frame rusak digugurkan dan diganti dengan run baru pada blok berikutnya."

**Pengganti:**
> Pengambilan data disusun dalam empat blok yang berselang-seling menurut arsitektur dengan urutan hardware offloading, software-based, software-based, hardware offloading (Blok 1: K2, Blok 2: K1, Blok 3: K1, Blok 4: K2; pola ABBA), sehingga pemrograman ulang firmware hanya dilakukan dua kali dan pergeseran antar-sesi terbagi pada kedua arsitektur. Setiap pemrograman ulang diikuti prosedur keadaan bersih (Sub-bab 3.5.4). Urutan skenario normal dan serangan di dalam tiap blok diacak dan setiap blok diapit run IDLE. Setiap blok software-based memuat 15 run skenario normal dan 15 run barrage; setiap blok hardware offloading memuat 15 run skenario normal dan cadangan run barrage (18 pada Blok 1 dan 23 pada Blok 4) karena sebagian run barrage dapat melampaui batas frame hilang (Sub-bab 3.5.1 butir 4). Setiap run diperiksa keutuhannya; run yang mengalami kehilangan baris data, galat checksum/I²C, frame rusak, atau frame hilang melebihi batas digugurkan dan tidak dianalisis. Run diambil sampai setiap kelompok memiliki tepat 30 run sah.

**Tabel 3.4:** pertahankan, tambahkan baris di bawah tabel: "Catatan: jumlah run yang dijalankan lebih besar daripada 120 karena run gugur dan run IDLE pembuka/penutup tiap blok."

**Fakta untuk Bab 4 (jangan ditulis di Bab 3):** total 139 run dijalankan di sesi resmi, 11 gugur (semuanya barrage hardware offloading), 8 IDLE.

---

## 24. Sub-bab 3.5.4 (BARU): Prosedur keadaan bersih dan pemrograman

**Sisipkan setelah 3.5.3:**
> 3.5.4 Prosedur Keadaan Bersih dan Pemrograman
> Arus tidur dalam mode Stop sangat peka terhadap keadaan debugger dan sisa daya pada jalur sinyal. Sesi debugger menyalakan bagian debug cip yang tetap menyala sampai daya putus, dan papan yang masih menerima daya lewat kabel sinyal tidak mengalami power-on reset yang sebenarnya. Oleh karena itu setiap blok pengukuran diawali prosedur berikut:
> 1. Firmware dibangun dan diprogram melalui ST-Link. Karena CPU yang sedang Stop tidak dapat dijangkau debugger, pemrograman dilakukan dengan skrip yang menangkap CPU saat terjaga (dibantu permintaan attacker node agar CPU sering bangun).
> 2. Daya papan host dimatikan penuh: USB-C, ST-Link, seluruh kabel sinyal dan GND dicabut, ditunggu 10 detik, lalu GND dipasang lebih dulu, kemudian kabel sinyal, ST-Link, dan USB-C terakhir.
> 3. Selama dan sesudah blok tidak dilakukan satu pun sesi debugger. Seluruh perintah berjalan melalui jalur kendali logger.
> 4. Keadaan bersih diverifikasi oleh run IDLE pembuka (arus diam ±1,99 mA) dan run IDLE penutup blok; blok dinyatakan sah bila pergeseran IDLE awal-akhir tidak melebihi 0,05 mA.
> Setelah firmware dimuat, firmware menunggu jendela aman 5 detik sebelum memasuki Stop pertama dan membiarkan jalur SWD tetap terkonfigurasi. Bila papan terkunci, pemulihan dilakukan dengan jumper BOOT.

**Alasan/sumber:** logbook 17 Nov (POR bersih), 18 Nov (debug power, bagian 8.1), rekap 18 Nov Bagian 7.5.

---

## 25. Sub-bab 3.5.5 (BARU): Uji ketahanan dan jalur galat

**Sisipkan setelah 3.5.4:**
> 3.5.5 Uji Ketahanan dan Jalur Galat
> Selain data utama, sistem diuji ketahanannya agar data tidak berasal dari perilaku yang rapuh. Ketahanan dinilai dari tidak adanya kegagalan sistem (crash atau macet) selama seluruh run. Jalur galat diuji pada perlakuan hardware offloading karena memuat kedua sumber bangun (alarm RTC dan permintaan attacker node): (1) attacker node berhenti total di tengah jendela; (2) attacker node dimatikan lalu dinyalakan ulang; (3) attacker node dinyalakan dan dimatikan berulang setiap 1,5 detik; (4) IC DS28E38 dilepas ketika run berlangsung; (5) IC dipasang kembali tanpa memulai ulang host; (6) IC tidak ada saat host dinyalakan. Kriteria lulus: jendela pengamatan tetap utuh, SCPU menurut logger dan firmware berbeda tidak lebih dari 0,5 poin persentase, tidak ada frame rusak, kegagalan tanda tangan tercatat sebagai gagal (bukan sebagai sukses), host pulih tanpa memulai ulang, dan host yang tidak menemukan IC melaporkan kondisi "belum siap" sehingga skrip kendali menolak merekam data.

**Alasan/sumber:** `tools/uji_galat_stop.py`; rekap 18 Nov Bagian 7.7 (7 dari 7 lulus).

**Tambahan pada 3.5.5 (verifikasi keabsahan tanda tangan):**
> Keabsahan kriptografis tanda tangan tidak diperiksa pada setiap run pengukuran, karena firmware hanya melaporkan keberhasilan penandatanganan agar tidak membebani jendela ukur. Keabsahannya diverifikasi terpisah pada tahap pengembangan dengan menurunkan ulang kunci publik dan memverifikasi setiap tanda tangan ECDSA P-256 (r, s) di komputer terhadap ringkasan pesan yang ditandatangani, termasuk pemeriksaan bahwa tanda tangan yang sengaja dirusak ditolak. Pada perlakuan hardware offloading dengan kecepatan overdrive, 269 dari 269 tanda tangan berturut-turut valid. Blok kripto perlakuan software-based pada firmware mode tidur dalam adalah salinan harfiah blok yang telah diverifikasi pada tahap pengembangan.

**Alasan/sumber:** `tools/k1_check.py`, `tools/k2_check.py`, `tools/ds_od.py`; logbook 18 Nov Bagian 1.3. Pernyataan ini juga perlu disebut sebagai batasan di Bab 1.3 butir 2.

---

## 26. Sub-bab 3.6, paragraf pembuka

**Tambahkan di akhir paragraf pertama:**
> Run yang tidak memenuhi kriteria keabsahan (Sub-bab 3.5.1 butir 4 dan 3.5.3) tidak dianalisis. Kriteria keabsahan ditetapkan sebelum analisis dan tidak diubah sesudah melihat hasil, kecuali batas frame hilang pada skenario barrage perlakuan hardware offloading yang ditetapkan peneliti sebagai toleransi kurang dari 0,25% setelah penyebab kehilangan frame diselidiki (frame yang mulai tepat saat CPU memulihkan jam dari Stop).

**Catatan kejujuran:** kalimat terakhir itu menjelaskan bahwa kriteria berubah sesudah pengamatan. Itu benar dan sebaiknya dinyatakan, agar penguji tidak menduga ada penyaringan data.

---

## 27. Sub-bab 3.6.3 (uji beda)

Hipotesis H0a–H0c, prosedur 1–6, dan kriteria H4 **tidak berubah**. Hanya tambahkan pada butir 5 (H4) bahwa tingkat layanan K4 dibaca bersama frame hilang: "Frame yang hilang pada jalur UART (kurang dari 0,25%) tidak dihitung sebagai permintaan yang tiba."

**Periksa:** pada analisis aktual, frame hilang tidak masuk ke N_dtg karena tidak pernah diterima host. Pernyataan di atas benar untuk hasil.

---

## 28. Sub-bab 3.6.4 (Bland-Altman)

**FIX_5:** "(1) latensi DWT terhadap durasi episode pin status per tanda tangan pada skenario normal..."

**Pengganti butir (1):**
> (1) latensi terhadap durasi episode pin status per tanda tangan pada skenario normal (DWT untuk software-based dan RTC untuk hardware offloading), dengan bias tetap ditafsirkan sebagai kerja CPU di luar jendela ukur (pengumpulan sumber acak dan pemulihan jam);

Butir (2) tetap (SCPU firmware terhadap siklus kerja pin). Butir (3) tetap, bergantung pada kalibrasi (Butir 33).

**Fakta untuk Bab 4:** bias pin − pengukuran firmware: K1 +9,29 ms (sd 0,06), K2 +3,28 ms (sd 0,26); bias S_CPU −0,014 poin.

---

## 29. Sub-bab 3.6.5 (ketidakpastian)

Struktur empat komponen tidak berubah. Tambahkan satu kalimat:
> Bila kalibrasi shunt terhadap multimeter belum tersedia, ukal ditetapkan sebesar 5% sebagai asumsi konservatif dan dinyatakan demikian pada tabel hasil.

(Hanya dipakai bila Butir 33 no. 1 pilihan B.)

---

## 30. Bab 4 (Jadwal Penelitian)

Bab ini di luar permintaan, tetapi Tabel 4.1 perlu tambahan baris kegiatan "Pengembangan dan uji mode tidur dalam" (November) agar sejalan dengan tahapan di Sub-bab 3.1. Jadwal aktual: 17 November (uji dasar), 18 November (integrasi, pengambilan data final, uji galat).

---

## 31. Daftar Pustaka: tambahan

Tambahkan (lengkapi tahun/revisi sesuai dokumen yang Anda unduh; saya tidak dapat memastikan nomor revisi dari repositori):
1. STMicroelectronics. (tahun). *RM0008: STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx, and STM32F107xx advanced Arm-based 32-bit MCUs* (Reference manual). → untuk mode Stop, RTC, dan EXTI event.
2. STMicroelectronics. (tahun). *STM32F103x8/B datasheet*. → arus Stop dan waktu bangun.
3. STMicroelectronics. (tahun). *AN2867: Oscillator design guide for STM8AF/AL/S and STM32 microcontrollers*. → osilator LSE 32,768 kHz.
4. Maxim Integrated. (2020). Datasheet DS28E38 (sudah ada) → pewaktuan 1-Wire overdrive; cantumkan juga di Sub-bab 3.4.1 b.

---

## 32. Konsistensi lintas bab (akan dibahas pada revisi Bab 2 dan Bab 1)

1. Bab 1.3 butir 6 ("hanya mode Sleep (WFI)... tanpa menguji mode Stop") harus diubah (mode Stop adalah mode uji).
2. Bab 2.5.1 butir 3 ("mode WFI", "~202 ms", "~72 ms aktif") harus diubah.
3. Intisari: "dengan CPU tidur (WFI)" dan "durasi tidur CPU dengan sinyal pin status" harus diubah.
4. Bab 2 perlu subbab baru tentang mode tidur STM32F103 (Sleep, Stop, Standby).

---

## 33. Hal yang harus Anda pastikan sebelum menempel

| No | Pertanyaan | Dampak bila tidak |
|---|---|---|
| 1 | **Kalibrasi shunt INA226 terhadap multimeter** sudah/belum dilakukan? FIX_5 menyebut hasil kalibrasi sebagai dasar u_kal dan indikator capaian (3). Di repositori kalibrasi belum selesai. **Pilihan A:** lakukan kalibrasi sebelum penulisan skripsi (data mentah tidak perlu diambil ulang). **Pilihan B:** hapus klaim kalibrasi dari 3.1 (3), 3.5.1 (2), 3.6.4 (3) dan pakai u_kal = 5% sebagai asumsi (Butir 29) | Klaim kalibrasi tanpa dasar dapat dipertanyakan penguji |
| 2 | Osiloskop GW Instek GDS-2000A (Tabel 3.1 no. 8) benar dipakai? | Peralatan yang tidak dipakai sebaiknya dihapus |
| 3 | Pull-up 1-Wire = 4,7 kΩ (Tabel 3.1, 3.3) sudah diukur? (catatan bangku menandainya sebagai dugaan, ±0,7 mA) | Spesifikasi komponen harus benar |
| 4 | Jamovi dipakai atau tidak? | Hapus bila tidak |
| 5 | Resistor seri 10 kΩ: pada jalur host PA9 → logger PA10 juga? (catatan 17 Nov menyebut logger PA9 → host PA10 dan attacker PA9 → host PA3) | Tabel 3.3 harus akurat |
| 6 | Gambar 3.1, 3.2, 3.3, 3.4: gambar ulang sesuai Butir 4, 15–16, 17 | Gambar harus sesuai teks |
| 7 | Nomor revisi/tahun dokumen ST pada Daftar Pustaka (Butir 31) | Sitasi lengkap |

---

## Lampiran: angka yang dipakai dan sumbernya

| Angka di teks pengganti | Sumber |
|---|---|
| Latensi 654,88 / 116,23 ms | `data/analisis/stop_final/ringkasan.md` Bagian 1 |
| Kapasitas 1,53 dan 8,60/detik; ρ; TL model 15,24 dan 85,83% | `ringkasan.md` Bagian 6 |
| Arus diam 1,99 mA; IDLE 1,984–1,992 mA; drift ≤ 0,004 mA | `pemeriksaan.md` Bagian 3 |
| Tobs 60,00–60,43 s | `runs.csv` kolom `T_obs_log_s` (60,0 s pada K2S, ±60,4 s pada K1S) |
| Overdrive 115,6 ms; busy 105,9–107,3 ms; 269/269 valid | logbook 18 Nov Bagian 1.3 |
| Debug power: 1,94 → 2,52 mA | logbook 18 Nov Bagian 7–8, rekap 18 Nov Bagian 7.5 |
| 139 run, 11 gugur, 30 sah per kelompok | `pemeriksaan.md` Bagian 1–2 |
| Uji galat 7 dari 7 | rekap 18 Nov Bagian 7.7 |
