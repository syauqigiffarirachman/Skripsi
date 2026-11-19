# Referensi dokumen untuk Bab 2/3 (butir E3, 18 November 2026)

Hasil pencarian web (hanya salinan pihak ketiga; situs resmi ST tidak terbaca dari sini). **Isi tahun/revisi final setelah Anda membuka halaman resmi
dokumennya**; kolom "Keyakinan" menunjukkan seberapa kuat buktinya. Jangan menulis revisi yang tak bisa Anda lihat sendiri di PDF yang Anda pakai.

| Dokumen | Judul | Temuan | Keyakinan | Tindakan |
|---|---|---|---|---|
| RM0008 | STM32F101xx, F102xx, F103xx, F105xx and F107xx advanced Arm-based 32-bit MCUs (Reference manual) | Rev 21, 22 Feb 2021 (salinan pihak ketiga); Rev 18 Mar 2018 dan Rev 16 Nov 2015 juga beredar | sedang | Tulis revisi + tahun dari halaman judul PDF yang Anda baca |
| DS5319 | STM32F103x8, STM32F103xB datasheet | Rev 20 terlihat di salinan; Rev 19 = Sep 2023; Rev 18 = Mar 2022 | rendah-sedang | Buka halaman judul PDF Anda |
| AN2867 | Oscillator design guide for STM8AF/AL/S and STM32 MCUs and MPUs (judul edisi baru; edisi lama "...for STM8S, STM8A and STM32 microcontrollers") | Rev 12 Jan 2020, Rev 15 Okt 2021, Rev 23 Jan 2025, kepala PDF resmi menunjukkan Rev 25 | rendah | Judul di naskah harus sama dengan edisi yang Anda baca |
| DS28E38 | DeepCover Secure ECDSA Authenticator (Maxim/Analog Devices) | Antarmuka 1-Wire 11,7 kbps dan 62,5 kbps (ringkasan fitur). Tabel perintah ROM mencantumkan **Overdrive-Skip ROM (3Ch)** dan **Overdrive-Match ROM (69h)**; perangkat yang sudah overdrive tetap overdrive sesudah perintah itu. | tinggi untuk keberadaan overdrive; angka waktu eksekusi sign tak terbaca | Cantumkan bagian tabel perintah ROM sebagai sumber klaim overdrive; waktu sign ukur sendiri (115,6 ms) |

Catatan:
- Pencarian tidak mengembalikan nomor revisi datasheet DS28E38 (kemungkinan dokumen 19-100135 menurut kode ADI; belum terverifikasi). Isi dari PDF yang Anda pakai.
- Klaim di naskah "IC mendukung overdrive" kini punya sumber (tabel perintah ROM, Overdrive-Skip ROM 3Ch). Angka "62,5 kbps" belum dipastikan sebagai overdrive pada datasheet: baca bagian itu langsung sebelum mengutipnya.
- Sumber pencarian: salinan pihak ketiga di sekorm.com (RM0008), radiolocman.com dan sejenisnya (DS5319), stmcu.com.cn / pdf4pro (AN2867), analog.com dan mouser.com (DS28E38).
