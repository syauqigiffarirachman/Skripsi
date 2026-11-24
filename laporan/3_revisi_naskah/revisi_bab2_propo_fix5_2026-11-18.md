# Revisi Bab 2 (PROPO_BAB1-3_FIX_5 → versi data *deep sleep*)

Tanggal: 18 November 2026
Acuan: `paper/PROPO_BAB1-3_FIX_5.pdf`, Bab 2 (halaman 10–19). Revisi ini mengikuti revisi Bab 3 (`laporan/3_revisi_naskah/revisi_bab3_propo_fix5_2026-11-18.md`) agar istilah, angka, dan pilihan teknis seragam.

Aturan: Bab 2 membekali pembaca dengan teori dan literatur yang dipakai Bab 3 dan Bab 4. Karena penelitian kini memakai mode tidur dalam (Stop), Bab 2 perlu memuat teori mode tidur STM32F103 dan jam RTC, yang belum ada di FIX_5. Tidak ada perbandingan dengan data pengembangan awal; hanya hasil literatur dan teori.

Format: **lokasi**, **teks FIX_5**, **teks pengganti** (siap tempel), **alasan**.

---

## Ringkasan perubahan

| No | Sub-bab | Jenis | Isi singkat |
|---|---|---|---|
| 1 | 2.1 par. 2 | Ubah kecil | "mode tidur" menjadi mode tidur dalam |
| 2 | 2.1.1 | Tambah | Perilaku jam 48 MHz dan bangun dari Stop |
| 3 | 2.1.2 (BARU) | **Tambah** | Mode hemat daya STM32F103 (Sleep, Stop, Standby), RTC/LSE, pengaruh debugger |
| 4 | 2.1.2 → 2.1.3 | Renomor | Urgensi autentikasi menjadi 2.1.3 |
| 5 | 2.2 | Tambah kecil | Kaitan DoSl dengan kedalaman tidur |
| 6 | 2.3.1 | Ubah kecil | Latar durasi komputasi dan kedalaman tidur |
| 7 | 2.3.2 | Tambah | Overdrive 1-Wire dan durasi komputasi IC terukur |
| 8 | Tabel 2.1 baris 7 | Ubah | "Penelitian ini": mode Stop |
| 9 | 2.5.1 butir 3 | **Tulis ulang** | Race-to-sleep dengan Stop, angka waktu aktif CPU |
| 10 | 2.5.1 (baru) | Tambah | Kontribusi: prosedur keadaan bersih dan bangun tanpa mengubah attacker |
| 11 | 2.6 + Gambar 2.1 | Ubah | Kerangka berpikir: tidur dalam |
| 12 | 2.7 hipotesis | Ubah kecil | H3 menyebut tidur dalam |
| 13 | Daftar isi, gambar, pustaka | Perbaiki | Nomor sub-bab, referensi baru |
| 14 | Periksa | Daftar | Hal yang harus Anda pastikan |

---

## 1. Sub-bab 2.1, paragraf kedua (sifat berdaya rendah)

**FIX_5:** "...sifat berdaya rendah dalam penelitian ini merujuk pada skenario simpul bertenaga baterai dan pemanfaatan mode tidur, bukan nilai arus statis mutlak cip."

**Pengganti:**
> ...sifat berdaya rendah dalam penelitian ini merujuk pada skenario simpul bertenaga baterai dan pemanfaatan mode tidur dalam (Stop mode, Sub-bab 2.1.2), bukan nilai arus statis mutlak cip maupun papan.

---

## 2. Sub-bab 2.1.1 (arsitektur host), tambahan di akhir paragraf pertama

**FIX_5:** "...mikrokontroler dioperasikan pada frekuensi 48 MHz menggunakan osilator internal (HSI) melalui PLL untuk menekan arus aktif sesuai kebutuhan sistem berdaya rendah."

**Tambahan:**
> Pada mode tidur dalam, jam utama dan PLL berhenti; setelah bangun, prosesor berjalan pada osilator internal 8 MHz sehingga PLL harus dinyalakan kembali sebelum frekuensi 48 MHz dicapai (STMicroelectronics, [tahun]).

---

## 3. Sub-bab 2.1.2 (BARU): Mode Hemat Daya pada STM32F103

**Sisipkan setelah 2.1.1, sebelum "Urgensi Autentikasi..." (yang menjadi 2.1.3):**

> **2.1.2 Mode Hemat Daya pada STM32F103**
> STM32F103 menyediakan tiga mode hemat daya (STMicroelectronics, [tahun a]). Pada mode Sleep, hanya jam inti CPU yang dihentikan, sedangkan jam bus dan periferal tetap berjalan sehingga CPU dapat dibangunkan oleh interupsi apa pun dengan latensi sangat singkat; arus diamnya tetap bergantung pada seluruh periferal yang menyala. Pada mode Stop, seluruh jam pada domain 1,8 V dihentikan, PLL serta osilator internal dan eksternal dimatikan, sedangkan isi SRAM dan register dipertahankan; regulator tegangan dapat dibiarkan normal atau dipindah ke mode daya rendah. CPU keluar dari Stop melalui salah satu jalur EXTI (dikonfigurasi sebagai interupsi atau sebagai event) atau melalui alarm jam RTC, dan setelah keluar prosesor berjalan pada osilator internal 8 MHz. Pada mode Standby, regulator 1,8 V dimatikan dan isi SRAM hilang kecuali pada domain cadangan; mode ini tidak dipakai dalam penelitian ini.
>
> Pada tingkat cip, arus mode Stop berada pada orde puluhan mikroampere, sedangkan arus mode aktif pada 48 MHz berada pada orde puluhan miliampere (STMicroelectronics, [tahun b]). Selisih sebesar ini menjadikan Stop relevan bagi strategi race-to-sleep (Chiang dkk., 2021) pada simpul bertenaga baterai, karena CPU dapat tidur dalam selama menunggu IC keamanan menyelesaikan komputasi. Konsekuensinya, selama Stop periferal seperti UART tidak dapat menerima data dan pencacah siklus DWT berhenti. Jam RTC berkristal 32,768 kHz berada pada domain cadangan, tetap berjalan selama Stop, dan dapat dipakai sebagai alarm pembangun serta basis waktu untuk mengukur lama tidur (STMicroelectronics, [tahun a]; STMicroelectronics, [tahun c]).
>
> Keadaan debugger memengaruhi arus mode Stop. Debugger dapat menahan jam inti tetap berjalan selama Stop melalui bit konfigurasi debug, dan sesi debugger menyalakan bagian debug pada cip. Pada penelitian ini pengamatan awal menunjukkan bahwa satu sesi debugger menaikkan arus diam papan dan mengubah perilaku bangun dari Stop sampai daya papan dimatikan penuh (Sub-bab 3.4.1 c). Karena itu pengukuran arus tidur dalam dilakukan pada papan dalam keadaan bersih (Sub-bab 3.5.4).

**Catatan sumber:** [tahun a] = RM0008 (power control, RTC, EXTI); [tahun b] = datasheet STM32F103x8/B; [tahun c] = AN2867 (osilator). Isi paragraf pertama mengikuti RM0008; periksa kembali istilah persis (mis. nama bit debug DBG_STOP) pada dokumen yang Anda unduh. Klaim terakhir (debugger) adalah temuan penelitian ini, bukan literatur; tempatkan rujukan ke Bab 3.4.1 c.

**Alasan:** FIX_5 tidak memuat teori mode tidur padahal seluruh penelitian kini bergantung pada mode Stop; Bab 3 mereferensikannya.

---

## 4. Renomor 2.1.2 → 2.1.3

"2.1.2 Urgensi Autentikasi dan Integritas Data Lapisan Fisik" menjadi **2.1.3**. Perbarui Daftar Isi, sitasi silang, dan nomor halaman.

---

## 5. Sub-bab 2.2 (ancaman), tambahan di akhir paragraf pertama

**FIX_5:** "Jika setiap permintaan memicu komputasi ECDSA berbasis perangkat lunak, waktu aktif CPU diperkirakan meningkat tajam dan mempercepat habisnya baterai (Agung dkk., 2024)."

**Tambahan:**
> Dampak ini bergantung pada kedalaman mode tidur yang dipakai perangkat: semakin dalam mode tidurnya, semakin besar selisih daya antara keadaan tertidur dan keadaan terjaga, sehingga setiap permintaan yang membangunkan perangkat menimbulkan kerugian energi yang makin besar (Pirretti dkk., 2006; Kuaban dkk., 2023).

**Periksa sumber:** pastikan kalimat ini didukung Pirretti dkk. dan Kuaban dkk. Bila tidak eksplisit di sumber, tulis sebagai penjelasan peneliti tanpa sitasi, atau tambahkan satu rujukan yang benar-benar menyatakannya.

---

## 6. Sub-bab 2.3.1, kalimat terakhir

**FIX_5:** "Hubungan kuantitatif antara durasi eksekusi ECDSA pada STM32F103 dan fraksi waktu tidur di bawah serangan menjadi fokus utama pengukuran penelitian ini."

**Pengganti:**
> Hubungan kuantitatif antara durasi eksekusi ECDSA pada STM32F103 dan fraksi waktu tidur dalam (Stop) di bawah serangan menjadi fokus utama pengukuran penelitian ini.

---

## 7. Sub-bab 2.3.2 (alur penandatanganan DS28E38)

**FIX_5:** "...mengaktifkan strong pull-up selama jeda komputasi internal IC (hingga 130 ms dengan arus tarik sekitar 10 mA pada pin IO) sebelum membaca hasil tanda tangan ECDSA sebesar 64 byte..."

**Tambahan sesudah kalimat itu:**
> IC mendukung kecepatan komunikasi standar dan overdrive pada bus 1-Wire (Maxim Integrated, 2020). Pada kecepatan overdrive waktu kirim perintah dan baca hasil jauh lebih singkat sehingga waktu total satu tanda tangan terutama ditentukan oleh komputasi internal IC. Dalam penelitian ini komputasi tersebut terukur sekitar 106,6 ms (rentang 105,9–107,3 ms), lebih singkat dari batas atas spesifikasi 130 ms.

**Catatan:** "mendukung overdrive" harus tertera di datasheet DS28E38 (periksa halaman). Penelitian ini sudah mempraktikkannya (komunikasi berhasil pada overdrive, 269 dari 269 tanda tangan valid), tetapi sitasinya harus datasheet.

---

## 8. Tabel 2.1 baris 7 ("Penelitian ini (2026)")

**FIX_5:** "Komparasi ECDSA P-256 software (Mbed TLS) vs. hardware offloading (DS28E38) | Latensi, daya dinamis, energi/permintaan, durasi tidur CPU, tingkat layanan | Mengukur dampak offloading pada STM32F103 dengan CPU masuk mode tidur selama DS28E38 bekerja pada kondisi normal dan barrage attack. Keterbatasan: 1 papan & 1 IC, 2 laju beban, catu USB, tanpa uji serangan fisik."

**Pengganti kolom terakhir:**
> Mengukur dampak offloading pada STM32F103 dengan CPU masuk mode tidur dalam (Stop) selama DS28E38 bekerja pada kondisi normal dan barrage attack. Keterbatasan: 1 papan & 1 IC, 2 laju beban, catu USB dengan beban tetap papan, tanpa uji serangan fisik.

---

## 9. Sub-bab 2.5.1 butir 3 — tulis ulang

**FIX_5:** "3. Pengujian Strategi Race-to-Sleep Selama Offloading: Menidurkan CPU STM32F103 (mode WFI) selama DS28E38 mengeksekusi ECDSA. Pada uji pendahuluan, meskipun latensi transaksi 1-Wire total mencapai ~202 ms, CPU host hanya aktif ~72 ms per permintaan dan tidur pada sisa waktunya, mekanisme yang tidak diuji pada Noseda dkk. (2022)."

**Pengganti:**
> 3. Pengujian Strategi Race-to-Sleep Selama Offloading: Menidurkan CPU STM32F103 dalam mode tidur dalam (Stop) selama DS28E38 mengeksekusi ECDSA. Pada uji pendahuluan, latensi transaksi 1-Wire total sekitar 116 ms, tetapi CPU host hanya terjaga sekitar 19 ms per permintaan (menerima frame, mengirim perintah, membaca hasil, dan memulihkan jam) dan tidur dalam pada sisa waktunya, termasuk sekitar 107 ms selama IC menghitung, mekanisme yang tidak diuji pada Noseda dkk. (2022).

**Alasan/sumber:** logbook 18 Nov (kirim 3,4 ms + baca 5,6 ms; komputasi IC 106,6 ms); waktu terjaga ±18,7 ms per permintaan dihitung dari S_CPU K2 normal 98,17% (1,83% × 60,0 s / 59 permintaan) dan K4 83,97% (16,03% × 60,1 s / 514 permintaan). Angka 72 ms pada FIX_5 berasal dari konfigurasi pengembangan awal dan tidak lagi berlaku.

---

## 10. Sub-bab 2.5.1, kontribusi tambahan (opsional)

**Tambahkan sebagai butir 5 dan 6 (bila Anda ingin menegaskan kebaruan metodologis):**
> 5. Mekanisme Bangun dari Stop Tanpa Mengubah Penyerang: Menerima frame permintaan UART pada mikrokontroler yang sedang dalam mode Stop dengan mengenali pola tepi byte pertama frame sebagai sinkron bit, sehingga perangkat dapat tidur dalam tanpa memerlukan byte pembangun tambahan dari pengirim.
> 6. Prosedur Pengukuran Arus Tidur Dalam yang Dapat Diulang: Menetapkan keadaan papan bersih (daya dimatikan penuh dan tanpa sesi debugger) sebagai syarat pengukuran arus Stop, karena sesi debugger terbukti mengubah arus diam dan perilaku bangun.

**Periksa:** apakah Anda ingin menyatakan ini sebagai "kontribusi ilmiah". Kebaruan butir 5 dan 6 tidak dibandingkan dengan literatur khusus; bila diklaim, tambahkan satu kalimat bahwa tinjauan literatur yang dilakukan tidak menemukan hal serupa, dan batasi pada "dalam lingkup literatur yang ditinjau".

---

## 11. Sub-bab 2.6 (kerangka berpikir) dan Gambar 2.1

**FIX_5:** "...pendelegasian pembuatan tanda tangan ke IC DS28E38 (hardware offloading) yang memungkinkan CPU host masuk ke mode tidur selama IC bekerja."

**Pengganti:** ganti "mode tidur" dengan "mode tidur dalam (Stop)".

**Gambar 2.1:** ubah label "mode tidur" menjadi "tidur dalam (Stop)" dan tambahkan kotak "kedalaman mode tidur" sebagai faktor yang memperbesar dampak waktu aktif CPU pada energi.

---

## 12. Sub-bab 2.7 (hipotesis)

**FIX_5:** "(H3) mempertahankan persentase durasi tidur CPU lebih tinggi pada kondisi normal maupun barrage attack."

**Pengganti:**
> (H3) mempertahankan persentase durasi tidur dalam (Stop) CPU lebih tinggi pada kondisi normal maupun barrage attack.

H1, H2, H4, dan kalimat interaksi tidak berubah (dan tetap konsisten dengan Sub-bab 3.6.3).

---

## 13. Daftar Isi, Daftar Gambar, dan Daftar Pustaka

1. Daftar Isi: tambahkan **2.1.2 Mode Hemat Daya pada STM32F103**; ubah "2.1.2 Urgensi Autentikasi..." menjadi 2.1.3; tambahkan 3.5.4 dan 3.5.5 (Bab 3).
2. Daftar Pustaka (tambahkan; lengkapi tahun/revisi dari dokumen yang Anda unduh):
   - STMicroelectronics. ([tahun a]). *RM0008: STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx advanced Arm-based 32-bit MCUs* [Reference manual].
   - STMicroelectronics. ([tahun b]). *STM32F103x8/B datasheet*.
   - STMicroelectronics. ([tahun c]). *AN2867: Oscillator design guide for STM8AF/AL/S and STM32 microcontrollers* [Application note].
3. Entri "STMicroelectronics (2013) STM32F1 series product lines" boleh dipertahankan.

---

## 14. Hal yang harus Anda pastikan

| No | Pertanyaan |
|---|---|
| 1 | Nomor revisi/tahun RM0008, datasheet STM32F103x8/B, dan AN2867 yang Anda pakai; periksa istilah persis di dokumen tersebut sebelum menempel paragraf Sub-bab 2.1.2 |
| 2 | Datasheet DS28E38 menyatakan dukungan overdrive (halaman berapa)? (Butir 7) |
| 3 | Apakah kalimat Butir 5 didukung sumber? (Butir 5) |
| 4 | Apakah Anda ingin menambahkan butir kontribusi 5–6? (Butir 10) |
| 5 | Apakah ada literatur yang membandingkan mode Sleep dan Stop pada energi kriptografi? Bila ingin, saya dapat mencari referensi (arXiv/Consensus) untuk Sub-bab 2.1.2 dan 2.5 |
| 6 | Gambar 2.1 digambar ulang |
