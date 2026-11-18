# Revisi Bab 1 dan Intisari (PROPO_BAB1-3_FIX_5 → versi data *deep sleep*)

Tanggal: 18 November 2026
Acuan: `paper/PROPO_BAB1-3_FIX_5.pdf`, Intisari (halaman 5) dan Bab 1 (halaman 6–9). Revisi ini mengikuti revisi Bab 3 (`laporan/revisi_bab3_propo_fix5_2026-11-18.md`) dan Bab 2 (`laporan/revisi_bab2_propo_fix5_2026-11-18.md`).

Aturan: Bab 1 menyatakan masalah, batasan, dan tujuan. Perubahan utamanya hanya satu hal besar: **mode hemat daya yang diuji kini mode Stop (tidur dalam), bukan mode Sleep (WFI).** Butir ini harus tampak konsisten di Intisari, 1.1, 1.3, dan 1.4. Tidak ada perbandingan dengan data pengembangan awal.

Format: **lokasi**, **teks FIX_5**, **teks pengganti** (siap tempel), **alasan**.

---

## Ringkasan perubahan

| No | Bagian | Jenis | Isi singkat |
|---|---|---|---|
| 1 | Intisari | Ubah | WFI → Stop; DWT/RTC; overdrive; 30 run sah |
| 2 | Intisari (opsional) | Tambah | Kalimat hasil (hanya bila dokumen menjadi skripsi) |
| 3 | 1.1 par. 4 | Ubah | Rujukan race-to-sleep dengan tidur dalam |
| 4 | 1.1 (baru) | Tambah | Alasan memilih mode tidur dalam |
| 5 | 1.2 | Ubah kecil | RM2 menyebut tidur dalam |
| 6 | 1.3 butir 2 | Ubah | Konfigurasi Mbed TLS dan verifikasi tanda tangan |
| 7 | 1.3 butir 6 | **Tulis ulang** | Mode daya: Stop, bukan Sleep |
| 8 | 1.3 butir 7 | Ubah | Lantai papan dan LED daya |
| 9 | 1.3 butir 10–12 (baru) | Tambah | Overdrive, frame UART, keadaan bersih |
| 10 | 1.4 | Ubah kecil | Tujuan menyebut tidur dalam |
| 11 | 1.5 | Tambah kecil | Manfaat prosedur pengukuran |
| 12 | Lintas bab | Catatan | Konsistensi |
| 13 | Periksa | Daftar | Hal yang harus Anda pastikan |

---

## 1. Intisari

**FIX_5, paragraf 1:** "...Penelitian ini bertujuan mengevaluasi pendelegasian (hardware offloading) pembuatan tanda tangan ECDSA P-256 dari mikrokontroler STM32F103 ke IC DS28E38 melalui bus 1-Wire, dengan CPU tidur (WFI) selama IC bekerja."

**FIX_5, paragraf 2:** "...masing-masing 30 run berdurasi 60 detik. Latensi pembuatan tanda tangan diukur dengan DWT CYCCNT, arus dan tegangan dengan sensor INA226, serta durasi tidur CPU dengan sinyal pin status..."

**Pengganti paragraf 1 dan 2:**
> Mikrokontroler berdaya rendah yang membuat tanda tangan digital ECDSA secara perangkat lunak rentan terhadap serangan Denial of Sleep (DoSl) jenis barrage attack, karena setiap permintaan autentikasi menahan CPU dalam keadaan aktif dan menguras baterai. Penelitian ini bertujuan mengevaluasi pendelegasian (hardware offloading) pembuatan tanda tangan ECDSA P-256 dari mikrokontroler STM32F103 ke IC DS28E38 melalui bus 1-Wire berkecepatan overdrive, dengan CPU tidur dalam (Stop mode) selama IC bekerja.
>
> Penelitian dirancang sebagai eksperimen faktorial 2 × 2 yang membandingkan arsitektur perangkat lunak (Mbed TLS) dan hardware offloading pada kondisi normal (1 permintaan/detik) dan barrage attack (10 permintaan/detik), masing-masing 30 run sah berdurasi 60 detik. Latensi pembuatan tanda tangan diukur dengan DWT CYCCNT (perangkat lunak) dan jam RTC (hardware offloading), arus dan tegangan dengan sensor INA226, serta durasi tidur dalam CPU dengan jam RTC yang divalidasi sinyal pin status. Data dianalisis dengan uji-t Welch terkoreksi Holm, interval kepercayaan bootstrap persentil, analisis Bland-Altman, dan analisis ketidakpastian pengukuran. Luaran yang diharapkan adalah selisih terukur latensi, energi per permintaan, durasi tidur dalam CPU, dan tingkat layanan antara kedua arsitektur.

**Alasan:** konsisten dengan Bab 3 (Butir 1, 11, 18, 20).

**Kata kunci:** tambahkan "mode tidur dalam (Stop)" bila ruang memungkinkan.

---

## 2. Intisari, kalimat hasil (OPSIONAL)

Hanya bila dokumen ini sudah menjadi laporan skripsi (bukan usulan), tambahkan sebelum "Kata kunci":
> Hasil pada 120 run sah menunjukkan bahwa hardware offloading mempersingkat latensi pembuatan tanda tangan sebesar 82,3% (dari 654,9 ms menjadi 116,2 ms), menurunkan daya dinamis sebesar 92,6% pada kondisi normal dan 58,0% pada barrage attack, serta menambah persentase durasi tidur dalam CPU sebesar 63,1 poin pada kondisi normal dan 83,6 poin pada barrage attack. Pada barrage attack, perangkat berbasis perangkat lunak melayani 15,0% permintaan dengan durasi tidur 0,38%, sedangkan hardware offloading melayani 85,5% permintaan dengan durasi tidur 84,0%.

**Sumber:** `data/analisis/stop_final/ringkasan.md`. Nilai mutlak daya masih memakai shunt nominal; persentase di atas tidak terpengaruh kalibrasi.

---

## 3. Sub-bab 1.1, paragraf keempat (kalimat race-to-sleep)

**FIX_5:** "Karena Noseda dkk. (2022) tidak menggunakan mode sleep selama secure element bekerja, efektivitas offloading ke DS28E38 pada STM32F103 dalam menerapkan strategi race-to-sleep yakni menyelesaikan tugas secepat mungkin lalu segera kembali tidur (Chiang dkk., 2021) perlu diuji secara empiris."

**Pengganti:**
> Karena Noseda dkk. (2022) tidak menggunakan mode sleep selama secure element bekerja, efektivitas offloading ke DS28E38 pada STM32F103 dalam menerapkan strategi race-to-sleep yakni menyelesaikan tugas secepat mungkin lalu segera kembali tidur (Chiang dkk., 2021), dengan mode tidur dalam selama IC bekerja, perlu diuji secara empiris.

---

## 4. Sub-bab 1.1, paragraf baru (alasan memilih tidur dalam)

**Sisipkan sebagai paragraf terakhir 1.1, sebelum kalimat "Target penelitian ini...":**
> Perangkat ujung bertenaga baterai umumnya memakai mode tidur dalam, yaitu mode yang mematikan jam utama dan hampir seluruh periferal sehingga arus diamnya jauh di bawah mode tidur ringan yang hanya menghentikan inti prosesor (STMicroelectronics, [tahun a]). Kedalaman tidur ini menentukan besar kerugian energi setiap kali sebuah permintaan membangunkan perangkat, sehingga dampak serangan penguras baterai paling relevan dinilai pada mode tidur dalam. Karena itu penelitian ini menguji perangkat dalam mode Stop.

**Alasan:** menjelaskan pilihan Stop tanpa membandingkan dengan data lain. Sitasi mengikuti Bab 2.1.2 ([tahun a] = RM0008).

---

## 5. Sub-bab 1.2 (rumusan masalah)

**FIX_5, butir 2:** "...menurunkan persentase durasi tidur CPU, menaikkan daya dinamis, dan menurunkan tingkat layanan..."

**Pengganti (ganti satu frasa):** "...menurunkan persentase durasi tidur dalam CPU, menaikkan daya dinamis, dan menurunkan tingkat layanan..."

Butir 1 dan 3 tidak berubah.

---

## 6. Sub-bab 1.3 butir 2 (fungsi kriptografi)

**FIX_5:** "Fungsi kriptografi: Pengujian dibatasi pada pembuatan tanda tangan digital (signing) ECDSA kurva NIST P-256 dengan masukan konstan 32 byte. Verifikasi tanda tangan, pembangkitan kunci, dan hashing tidak dibandingkan. Pembanding perangkat lunak dibatasi pada pustaka Mbed TLS."

**Pengganti:**
> 2. Fungsi kriptografi: Pengujian dibatasi pada pembuatan tanda tangan digital (signing) ECDSA kurva NIST P-256 dengan masukan konstan 32 byte. Verifikasi tanda tangan, pembangkitan kunci, dan hashing tidak dibandingkan. Pembanding perangkat lunak dibatasi pada pustaka Mbed TLS dengan satu konfigurasi, yaitu tabel percepatan comb dimatikan, sehingga besar selisih terhadap hardware offloading bergantung pada konfigurasi ini. Keabsahan kriptografis tanda tangan diverifikasi di komputer pada tahap pengembangan, tidak pada setiap run pengukuran.

**Alasan:** konfigurasi software menentukan latensi (654,9 ms); batasan ini tidak ada di FIX_5. Verifikasi per run tidak dilakukan (lihat Bab 3 Butir 25).

---

## 7. Sub-bab 1.3 butir 6 (mode daya) — tulis ulang

**FIX_5:** "Mode daya: Mode hemat daya yang diuji hanya mode Sleep (instruksi WFI), tanpa menguji mode Stop maupun Standby. Persentase durasi tidur merujuk pada waktu tidur inti CPU, bukan seluruh papan."

**Pengganti:**
> 6. Mode daya: Mode hemat daya yang diuji hanya mode tidur dalam (Stop) dengan regulator tegangan dalam mode daya rendah dan alarm RTC sebagai pembangun; mode Sleep dan Standby tidak diuji. Persentase durasi tidur merujuk pada waktu CPU berada dalam Stop, bukan seluruh papan, dan arus diam papan didominasi beban tetap papan (Butir 7).

---

## 8. Sub-bab 1.3 butir 7 (lingkup sistem dan catu daya)

**FIX_5:** "...sehingga nilai daya mencakup seluruh komponen papan (STM32F103, DS28E38, regulator, dan indikator)."

**Tambahan di akhir butir:**
> Pada mode tidur dalam, arus diam papan sekitar 1,99 mA dan didominasi oleh LED daya papan yang sengaja dipertahankan, sehingga nilai mutlak mencerminkan daya tingkat papan dan bukan konsumsi cip (yang pada mode Stop berada pada orde puluhan mikroampere).

---

## 9. Sub-bab 1.3, butir baru

**Tambahkan sebagai butir 10–12 (setelah butir 9):**
> 10. Implementasi hardware offloading: IC DS28E38 diakses melalui driver 1-Wire buatan sendiri pada kecepatan overdrive dengan penantian hasil berbasis polling. Latensi tanda tangan perlakuan ini bergantung pada konfigurasi tersebut dan tidak mewakili seluruh cara akses (misalnya kecepatan standar dengan penantian tetap).
> 11. Jalur permintaan: Penerimaan frame permintaan oleh CPU yang sedang tidur dalam dilakukan dengan mengenali pola byte pertama frame tanpa mengubah attacker node. Pada barrage attack, frame yang mulai tepat ketika CPU memulihkan jam dari Stop dapat hilang; kehilangan ini dibatasi kurang dari 0,25% dari permintaan yang tiba per run dan dilaporkan.
> 12. Keadaan pengukuran: Arus tidur dalam hanya sah diukur pada papan dalam keadaan bersih (daya dimatikan penuh dan tanpa sesi debugger), sehingga setiap pemrograman ulang firmware diikuti prosedur tersebut.

**Alasan:** ketiganya menyatakan syarat dan batas yang memengaruhi tafsir hasil; mencegah pembaca menganggap latensi K2 berlaku untuk semua akses DS28E38.

---

## 10. Sub-bab 1.4 (tujuan)

**FIX_5, butir 2:** "Mengukur pengaruh barrage attack terhadap persentase durasi tidur CPU, daya dinamis, dan tingkat layanan pada kedua arsitektur."

**Pengganti:** "Mengukur pengaruh barrage attack terhadap persentase durasi tidur dalam CPU, daya dinamis, dan tingkat layanan pada kedua arsitektur."

Butir 1 dan 3 tidak berubah.

---

## 11. Sub-bab 1.5 (manfaat), tambahan

**Tambahkan di butir 1 (Bagi Peneliti) atau butir 3 (Bagi Industri):**
> ...serta prosedur pengukuran arus tidur dalam pada mikrokontroler (keadaan bersih tanpa debugger dan dengan jam RTC sebagai basis waktu) yang dapat diterapkan kembali pada penelitian serupa.

---

## 12. Konsistensi lintas bab

| Hal | Bab 1 | Bab 2 | Bab 3 |
|---|---|---|---|
| Mode hemat daya | tidur dalam (Stop) | 2.1.2 (baru) | 3.1, 3.4.1 a, 3.5, 3.5.1 (3) |
| Konfigurasi software | 1.3 butir 2 | — | Tabel 3.2 baris 2 |
| Overdrive | 1.3 butir 10 | 2.3.2 | 3.4.1 b |
| Latensi uji pendahuluan | — | 2.5.1 butir 3 (116 ms) | 3.5.2 (654,9 dan 116,2 ms) |
| Keadaan bersih | 1.3 butir 12 | 2.1.2 | 3.4.1 c, 3.5.4 |
| Toleransi frame hilang | 1.3 butir 11 | — | 3.5.1 butir 4, 3.5.3 |
| Verifikasi tanda tangan | 1.3 butir 2 | — | 3.5.5 |

---

## 13. Hal yang harus Anda pastikan

| No | Pertanyaan |
|---|---|
| 1 | Apakah dokumen ini tetap berbentuk **usulan** (proposal) atau sudah menjadi **skripsi**? Menentukan apakah Butir 2 (kalimat hasil) dipakai dan apakah "Luaran yang diharapkan" diubah menjadi hasil. |
| 2 | Apakah Anda ingin memasukkan "mode tidur dalam" ke Judul? Judul sekarang tidak menyebut mode tidur dan masih benar; mengubahnya tidak wajib. |
| 3 | Apakah klaim "regulator dalam mode daya rendah" (Butir 7) sesuai konfigurasi akhir? Firmware akhir menyetel LPDS (regulator daya rendah) pada saat Stop; konfirmasi bila Anda tidak ingin menyebut detail ini di Bab 1. |
| 4 | Apakah Anda ingin menyebut data pengembangan awal (mode Sleep) di Bab 1 sebagai latar? Revisi ini sengaja tidak menyebutnya sesuai arahan Anda. |
