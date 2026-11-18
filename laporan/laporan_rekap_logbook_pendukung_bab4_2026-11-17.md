# Laporan Rekapitulasi Logbook Penelitian 10–16 November 2026

**Dokumen Pendukung Bab 4 (Hasil dan Pembahasan) dan Bab 5 (Kesimpulan)**

Judul penelitian (ringkas): Perbandingan pembuatan tanda tangan digital ECDSA P-256 secara perangkat lunak (*software*) pada mikrokontroler STM32F103 dengan pengalihan ke chip keamanan DS28E38 (*hardware offloading*) pada kondisi normal dan kondisi serangan *barrage* (*Denial of Sleep*).

Tanggal penyusunan: 17 November 2026
Sumber: logbook harian 10, 11, 12, 13, 14, 15, dan 16 November 2026, berkas hasil analisis statistik tahap D (keluaran 16 November 2026 pukul 01.25), serta laporan progres dan revisi Bab 3 yang sudah ada.

---

## Daftar Isi

1. Pendahuluan
2. Daftar Istilah
3. Gambaran Umum Penelitian
4. Uraian Kegiatan per Hari
   - 4.1 Senin, 10 November 2026
   - 4.2 Selasa, 11 November 2026
   - 4.3 Rabu, 12 November 2026
   - 4.4 Kamis, 13 November 2026
   - 4.5 Jumat, 14 November 2026
   - 4.6 Sabtu, 15 November 2026
   - 4.7 Minggu, 16 November 2026
5. Rekapitulasi Hasil Akhir (Pengambilan Data Tahap D)
6. Pembahasan Pendukung
7. Bukti Keandalan dan Keabsahan Data
8. Perubahan terhadap Rencana Awal (Proposal)
9. Keterbatasan dan Hal yang Belum Tuntas
10. Bahan Penyusunan Kesimpulan dan Saran
11. Lampiran

---

## 1. Pendahuluan

### 1.1 Latar belakang penyusunan

Selama 10–16 November 2026, kegiatan penelitian dicatat dalam logbook harian. Sebagian logbook ditulis atau dilengkapi setelah kegiatan selesai, sehingga catatannya bersifat teknis, terpisah per hari, dan memuat banyak istilah perangkat. Dokumen ini menyusun ulang seluruh catatan tersebut secara runtut dan dengan bahasa yang lebih umum, supaya dapat dipakai sebagai:

1. bahan pendukung penulisan Bab 4 (Hasil dan Pembahasan),
2. dasar penarikan kesimpulan dan saran pada Bab 5,
3. bukti tertulis proses penelitian bagi pembimbing dan penguji.

### 1.2 Cakupan

Dokumen ini mencakup seluruh kegiatan sejak pembuatan program awal (10 November) sampai pengambilan data final, analisis statistik, dan perapian kode (16 November). Hal yang dibahas: tujuan tiap kegiatan, hasilnya, kendala dan cara mengatasinya, keputusan yang diambil, serta kaitannya dengan Bab 4.

### 1.3 Catatan kejujuran data

- Angka di dokumen ini disalin dari logbook dan berkas hasil analisis, bukan dari ingatan. Bila logbook menyebut suatu hal "belum diuji" atau "dugaan", status itu dipertahankan.
- Satu bagian logbook 14 November (pengujian penangkap sinyal pin, disebut tahap L4) ditulis pada 15 November berdasarkan isi kode, karena sesi 14 November tidak ditutup dengan catatan. Hasil uji tahap ini baru ada pada 15 November.
- Nilai mutlak arus, daya, dan energi masih memakai nilai hambatan sensor arus nominal (0,100 ohm). Kalibrasi belum selesai saat dokumen ini disusun (lihat Bagian 9). Persentase penghematan tidak terpengaruh oleh hal ini.

---

## 2. Daftar Istilah

Istilah berikut dipakai berulang kali. Penjelasannya sengaja disederhanakan.

| Istilah | Arti sederhana |
|---|---|
| Tanda tangan digital (ECDSA P-256) | Bukti matematis bahwa suatu pesan disahkan oleh pemilik kunci rahasia. Siapa pun yang memegang kunci publik dapat memeriksanya, tetapi tidak dapat memalsukannya. |
| Software (perangkat lunak), K1/K3 | Tanda tangan dihitung sendiri oleh prosesor mikrokontroler memakai pustaka Mbed TLS. |
| Hardware offloading (pengalihan ke perangkat keras), K2/K4 | Mikrokontroler hanya meminta chip DS28E38 membuat tanda tangan. Kunci rahasia tersimpan di dalam chip dan tidak pernah keluar. |
| Mikrokontroler STM32F103 ("papan host") | Komputer kecil satu chip yang diuji dalam penelitian ini, berjalan pada 48 MHz. |
| Papan penyerang (attacker node) | Mikrokontroler kedua yang mengirim permintaan tanda tangan secara berkala ke papan host. |
| Papan pencatat (logger) | Mikrokontroler ketiga yang merekam arus listrik, tegangan, dan sinyal "sibuk/tidur" dari papan host, lalu mengirimkannya ke komputer. |
| INA226 | Sensor kecil yang mengukur arus dan tegangan listrik yang masuk ke papan host. |
| Shunt | Hambatan listrik sangat kecil di jalur daya. Arus dihitung dari tegangan kecil yang muncul di hambatan ini. |
| Skenario normal (A) | Papan host menerima 1 permintaan tanda tangan per detik. |
| Skenario barrage (B) | Papan host dibanjiri 10 permintaan tanda tangan per detik, meniru serangan. |
| Denial of Sleep | Serangan yang membuat perangkat terus terjaga sehingga tidak sempat "tidur" hemat energi dan baterainya cepat habis. |
| Mode tidur (WFI) | Keadaan ketika prosesor berhenti bekerja dan menunggu, sehingga konsumsi daya turun. |
| Run | Satu kali pengamatan selama 60 detik. Satu run menghasilkan satu baris data. |
| Blok | Sekumpulan run berturut-turut dengan firmware yang sama, diapit pengukuran diam di awal dan akhir. |
| IDLE | Pengamatan 60 detik tanpa permintaan, untuk mengetahui konsumsi dasar papan saat diam. |
| Latensi tanda tangan | Lama waktu membuat satu tanda tangan (dalam milidetik). Ini bukan waktu tunggu permintaan dalam antrean. |
| %S_cpu (persentase tidur CPU) | Berapa persen dari 60 detik prosesor berada dalam mode tidur. Makin tinggi makin hemat. |
| Daya dinamis (P_dyn) | Daya tambahan yang muncul karena melayani permintaan, yaitu daya rata-rata dikurangi daya saat diam. |
| Arus dinamis (I_dyn) | Arus tambahan karena melayani permintaan, yaitu arus rata-rata dikurangi arus saat diam. |
| Energi per permintaan terlayani (E_lyn) | Energi tambahan yang dihabiskan untuk menyelesaikan satu tanda tangan. |
| Tingkat layanan (TL) | Persentase permintaan yang benar-benar dilayani dari seluruh permintaan yang masuk. |
| Throughput | Jumlah tanda tangan yang selesai per detik. |
| Permintaan terlewat | Permintaan yang datang ketika papan sedang sibuk sehingga digabung dengan permintaan lain dan tidak dilayani tersendiri. |
| Firmware | Program yang ditanam di mikrokontroler. |
| ST-Link | Alat pemrogram untuk menanamkan firmware ke mikrokontroler. |
| Build from base | Aturan kerja penelitian ini: program dibangun bertahap, satu fungsi per tahap, dan tiap tahap diuji sampai stabil sebelum fungsi berikutnya ditambahkan. |
| Gain | Persentase perbaikan dari software ke hardware offloading. |
| Interval kepercayaan 95 % (IK 95 %) | Rentang nilai yang dengan keyakinan 95 % memuat nilai sebenarnya. |
| Nilai p | Ukuran statistik. Bila sangat kecil (di bawah 0,05), perbedaan yang teramati hampir pasti bukan kebetulan. |
| ppm | Bagian per sejuta. 1.000 ppm sama dengan 0,1 %. |

---

## 3. Gambaran Umum Penelitian

### 3.1 Pertanyaan yang ingin dijawab

Penelitian ini membandingkan dua cara membuat tanda tangan digital pada perangkat IoT kecil:

- **Cara software:** prosesor menghitung sendiri. Perhitungannya berat sehingga prosesor lama terjaga.
- **Cara hardware offloading:** prosesor menyerahkan pekerjaan kepada chip DS28E38, lalu tidur sambil menunggu chip selesai.

Perbandingan dilakukan dalam dua keadaan: normal (1 permintaan per detik) dan diserang (10 permintaan per detik). Rumusan masalah yang dijawab (sesuai Bab 3 versi 5):

- **RM1:** Berapa latensi dan energi per permintaan pada masing-masing cara?
- **RM2:** Seberapa besar pengaruh serangan barrage pada tiap cara?
- **RM3:** Seberapa besar selisih kedua cara, dan apakah selisih itu berbeda antara kondisi normal dan kondisi diserang?

Hipotesis yang diuji: H1a (latensi), H1b (daya dinamis), H1c (persentase tidur CPU), hipotesis interaksi (selisih persentase tidur lebih besar saat diserang), dan H4 (tingkat layanan, metrik tambahan).

### 3.2 Desain percobaan

Desain faktorial 2 × 2 dengan 30 ulangan per kelompok, sehingga total 120 run sah:

| Kelompok | Cara | Skenario | Jumlah run |
|---|---|---|---|
| K1 | Software (Mbed TLS) | Normal, 1 permintaan/detik | 30 |
| K2 | Hardware offloading (DS28E38) | Normal, 1 permintaan/detik | 30 |
| K3 | Software (Mbed TLS) | Barrage, 10 permintaan/detik | 30 |
| K4 | Hardware offloading (DS28E38) | Barrage, 10 permintaan/detik | 30 |

### 3.3 Susunan alat uji

Tiga mikrokontroler dan satu sensor arus dipakai bersama:

1. **Papan host (STM32F103C8, 48 MHz):** papan yang diuji. Ia menerima permintaan, membuat tanda tangan (sendiri atau lewat DS28E38), lalu tidur.
2. **Papan penyerang (STM32F030C8):** mengirim permintaan tanda tangan dengan pewaktu perangkat keras yang presisi, 1 atau 10 kali per detik.
3. **Papan pencatat (STM32F103C8 dengan kristal, 72 MHz):** merekam arus dan tegangan dari INA226 sekitar 496 kali per detik dan menangkap setiap perubahan sinyal "sibuk/tidur" dari papan host. Karena memakai kristal, jamnya jauh lebih tepat daripada jam papan host, sehingga dipakai sebagai acuan waktu.
4. **Sensor INA226:** dipasang di jalur daya USB papan host. Semua listrik yang dipakai papan host melewati sensor ini.

Komputer menjalankan skrip yang mengatur urutan run, memberi perintah, menyimpan data mentah, dan menghitung hasil.

### 3.4 Besaran yang diukur

| Besaran | Cara memperoleh |
|---|---|
| Latensi tanda tangan | Penghitung siklus prosesor papan host, lalu dikoreksi dengan jam kristal papan pencatat |
| Persentase tidur CPU | Dihitung firmware dan, secara terpisah, dari sinyal pin yang direkam papan pencatat |
| Arus, daya, energi | Sensor INA226, dijumlahkan sepanjang 60 detik |
| Tingkat layanan, throughput | Jumlah permintaan masuk dan jumlah tanda tangan selesai yang dilaporkan papan host |

---

## 4. Uraian Kegiatan per Hari

### 4.1 Senin, 10 November 2026: Program awal komunikasi dengan chip DS28E38

**Tujuan:** membuat komputer dan mikrokontroler dapat "berbicara" dengan chip DS28E38 dan memintanya membuat tanda tangan.

**Kegiatan dan hasil:**

1. **Program penghubung komputer–chip (Python).** Komputer mengirim perintah ke DS28E38 dan DS2476 melalui alat penghubung USB DS9481P-300. Program dapat membaca identitas chip dan meminta tanda tangan. Perintah yang bersifat permanen sengaja dikunci agar chip tidak rusak karena salah perintah.
2. **Program tanda tangan dan pemeriksa.** Tiga fungsi: melihat identitas dan kunci publik chip, menandatangani pesan atau berkas, serta memeriksa keaslian tanda tangan tanpa memerlukan chip. Hasil uji: tanda tangan asli dinyatakan sah, sedangkan pesan yang diubah atau tanda tangan yang dirusak ditolak.
3. **Versi bahasa C.** Program ditulis ulang dalam bahasa C agar lebih dekat dengan program mikrokontroler. Tanda tangan dari versi Python dapat diperiksa oleh versi C, dan sebaliknya.
4. **Program awal STM32.** Papan menerima perintah teks dari komputer lalu meneruskannya ke DS28E38. Program masih tahap awal dan belum aktif.
5. **Alat bantu:** pengaturan pemrograman lewat ST-Link dan program monitor yang dapat menyambung ulang otomatis.

**Kendala:** program STM32 belum dapat dijalankan, dan dokumentasi belum konsisten.

**Kaitan dengan Bab 4:** menjadi dasar bahwa tanda tangan dari DS28E38 dapat diperiksa secara independen di komputer. Pemeriksaan semacam ini dipakai di seluruh tahap berikutnya untuk memastikan setiap tanda tangan sah.

---

### 4.2 Selasa, 11 November 2026: Mempercepat chip, pembanding software terberat, dan alat ukur

**Tujuan:** mempercepat jalur DS28E38, menyiapkan pembanding software, dan menyiapkan alat ukur daya.

**Kegiatan dan hasil:**

1. **Percepatan tanda tangan lewat DS28E38.** Sebelumnya program selalu menunggu 520 ms, padahal chip selesai sekitar 107 ms. Program diubah agar langsung membaca hasil begitu chip selesai, dan alamat chip disimpan agar tidak dicari ulang. Hasilnya, satu tanda tangan turun dari sekitar 0,6 detik menjadi sekitar **190 ms** (kira-kira 3 kali lebih cepat). Semua tanda tangan sah. Mode kecepatan tinggi (*overdrive*) dicoba tetapi chip tidak membalas, sehingga tidak dipakai.
2. **Pembanding software terberat.** Mbed TLS tanpa optimasi apa pun membutuhkan sekitar **4,9 detik** per tanda tangan pada 48 MHz. Sebagai gambaran, pustaka micro-ecc yang ringan sekitar 260 ms. Temuan penting hari ini: **pilihan pustaka dan pengaturannya sangat menentukan hasil perbandingan software dengan hardware.**
3. **Penanda untuk alat ukur.** Tiga pin dijadikan penanda (sedang menandatangani, sedang tidur, dan jendela pengamatan 60 detik). Aturan tidur dibuat sama untuk software dan hardware supaya perbandingannya adil.
4. **Program jendela 60 detik** untuk mode normal dan mode serangan. Ditemukan bahwa papan dapat macet bila antrean data USB penuh.
5. **Perkiraan jumlah siklus dengan emulator**, dikalibrasi dengan hasil nyata micro-ecc. Hasil ini hanya perkiraan dan tidak dipakai sebagai data utama.

**Kendala:** pengukuran Mbed TLS terberat sangat lama, dan pengukuran arus belum dilakukan.

**Kaitan dengan Bab 4:** memberi konteks bahwa angka latensi software bergantung kuat pada konfigurasi pustaka, sehingga konfigurasi yang dipakai wajib disebutkan secara eksplisit.

---

### 4.3 Rabu, 12 November 2026: Memahami protokol, aturan kerja bertahap, dan tahap pertama software serta penyerang

**Tujuan:** memahami cara pengambilan data pada Bab 3, membandingkannya dengan kode yang ada, lalu mulai membangun sistem secara bertahap.

**Kegiatan dan hasil:**

1. **Kajian Bab 3 proposal (revisi v2, kemudian v3).** Disusun ringkasan desain 2 × 2, alur satu run, tiga metrik, variabel kontrol, dan rencana analisis. Ditetapkan aturan bahwa **naskah proposal terbaru adalah acuan utama**; logbook hanya pengingat.
2. **Analisis kesenjangan.** Ditemukan tujuh hal yang belum sesuai dengan proposal: jalur komunikasi masih USB (proposal memakai UART), belum ada protokol mulai/akhir run, belum ada firmware pencatat dan penyerang, belum ada firmware DS28E38 dengan jendela 60 detik, belum ada skrip 120 run, dan periode skenario normal di kode masih 10 detik (proposal: 1 detik).
3. **Aturan "build from base" ditetapkan** dan berlaku untuk semua papan.
4. **Keputusan baseline software.** Profil terberat (4,9 detik) diganti dengan ECDSA standar: nonce deterministik RFC 6979 dan optimasi bawaan Mbed TLS aktif. Profil terberat tetap disimpan sebagai batas atas.
5. **Software tahap 1: LULUS.** Kunci publik cocok, 30 dari 30 tanda tangan sah dan identik dengan perhitungan acuan independen di komputer. Latensi rata-rata **276,436 ms** (sebaran 0,5 %). Pembanding DS28E38 dari data lama sekitar 224 ms, sehingga pada konfigurasi ini DS28E38 sekitar 19 % lebih cepat.
6. **Papan penyerang tahap 1: LULUS.** Papan penyerang ternyata STM32F030C8 (bukan Blue Pill F103 seperti di proposal). Papan mengirim paket permintaan 36 byte tepat 1 atau 10 kali per detik. Tiga kesalahan program ditemukan dan diperbaiki: dua port serial berebut satu perangkat, satu port serial tidak aktif otomatis, dan pewaktu mengirim paket pertama terlalu cepat. Laju terverifikasi tepat 1,000 Hz dan selang 100,000 ms pada mode 10 Hz.
7. Kabel dari papan penyerang ke papan host disambung secara fisik, dan pemrograman otomatis tanpa memindah jumper diatur.

**Pertanyaan terbuka hari itu:** bagaimana mendefinisikan permintaan "terlewat" bila tanda tangan software lebih lama dari selang permintaan.

**Kaitan dengan Bab 4:** menjelaskan asal-usul baseline software dan bukti bahwa papan penyerang menghasilkan beban yang tepat.

---

### 4.4 Kamis, 13 November 2026: Pembangunan sistem host software dan hardware sampai permintaan nyata

Hari ini merupakan hari pembangunan paling padat untuk papan host.

**a. Verifikasi jalur penyerang ke host.** Pemrograman papan penyerang dipindah ke ST-Link. Penerima paket di papan host dibuat sebagai tahap terpisah dan **LULUS**: pada 1 Hz dan 10 Hz tidak ada paket rusak atau hilang. Ditemukan bahwa papan host tidak boleh menunggu sambungan USB saat dinyalakan, karena paket awal akan hilang.

**b. Software tahap 1b, pengerasan keamanan: LULUS.** Atas permintaan peneliti, Mbed TLS diatur seketat mungkin dalam batas kemampuan papan:
- nonce *hedged* (gabungan acak dan deterministik), sehingga tetap aman meskipun sumber acaknya lemah;
- kunci rahasia hanya dimuat ke memori selama menandatangani, lalu dihapus;
- sumber acak dikumpulkan dari beberapa sumber di luar jendela pengukuran latensi.

Hasil: 30/30 sah, semua nilai unik, latensi **278,614 ms**. Biaya pengerasan hanya sekitar 2,2 ms (0,8 %).

**c. Software tahap 1c, mematikan tabel percepatan (*comb*), dan pengaruh tiap opsi.** Ditemukan jebakan: setelah pengaturan pustaka diubah, pustaka tidak dibangun ulang otomatis sehingga hasil pertama keliru. Kesimpulan awal yang salah dicabut, dan aturan "bersihkan dulu sebelum membangun ulang" ditetapkan. Hasil terukur yang benar:

| Varian Mbed TLS | Latensi rata-rata | Keterangan |
|---|---|---|
| Tahap 1b (semua optimasi aktif) | 278,6 ms | acuan |
| **Tahap 1c: tabel comb dimatikan (dipakai dalam penelitian)** | **654,7 ms** | 2,35 kali lebih lambat |
| 1c + optimasi rakitan dimatikan | 687,4 ms | hanya +5 % |
| 1c + optimasi khusus kurva P-256 dimatikan | 4.331,9 ms | 6,6 kali lebih lambat |

Catatan penting dari logbook: **keputusan menyalakan atau mematikan tabel comb mengubah keunggulan DS28E38 dari sekitar 19 % menjadi sekitar 66 % lebih cepat**, sehingga harus dinyatakan secara eksplisit di Bab 4.

**d. Pemakaian memori (RAM) puncak.** Diukur dengan metode yang sama untuk kedua cara, yaitu satu operasi tanda tangan:

| | Statik | Heap | Stack | Total puncak |
|---|---|---|---|---|
| Software (konfigurasi 1c) | 4.648 B | 3.236 B | 1.760 B | 9.644 B (47,0 % dari 20 KB) |
| DS28E38 | 4.584 B | 0 B | 784 B | 5.368 B (26,2 %) |

Software memakai sekitar 1,8 kali lebih banyak RAM. Selisihnya hampir seluruhnya berasal dari pustaka kripto. Setelah fitur tahap 3 dan 4 ditambahkan, angkanya menjadi 10.116 B (software) dan 6.056 B (DS28E38).

**e. Firmware DS28E38 dibangun ulang dari dasar: LULUS.** Firmware lama digantikan firmware baru yang strukturnya serupa dengan software. Latensi **200,936 ms** (sebaran 0,23 %), 30/30 sah, 30/30 tanda tangan rusak ditolak. Firmware baru 23 ms lebih cepat dari yang lama karena pencarian alamat chip hanya dilakukan sekali saat menyala. Perbedaan yang harus dicatat di laporan: pada DS28E38 proses hash dilakukan oleh chip, sedangkan software menandatangani digest secara langsung.

**f. Tahap 3: mode tidur, pewaktu perangkat keras, pin status, dan persentase tidur: LULUS untuk kedua cara.** Prosesor tidur di antara permintaan. Pada DS28E38, prosesor juga tidur selama menunggu chip bekerja (130 ms). Hasil dengan permintaan buatan:

| Uji | Software %S_cpu | DS28E38 %S_cpu |
|---|---|---|
| Diam | 99,99 % | 99,99 % |
| 1 permintaan/detik | 36,15 % | 92,99 % |
| 10 permintaan/detik | 0,32 % (47 dari 299 dilayani) | 64,71 % (149 dari 299 dilayani) |

Temuan teknis penting: USB membangunkan prosesor 1.000 kali per detik. Pada pengukuran sebenarnya USB hanya menjadi sumber daya, sehingga gangguan ini dinonaktifkan selama jendela pengamatan.

**g. Tahap 4: permintaan nyata dari papan penyerang: LULUS untuk kedua skenario.** Permintaan sintetis diganti paket nyata dari papan penyerang. Satu kesalahan tersembunyi pada jalur pemulihan paket rusak ditemukan melalui uji paksa dan diperbaiki. Hasilnya konsisten dengan permintaan buatan:

| | Software | DS28E38 |
|---|---|---|
| %S_cpu, normal 60 detik | 35,57 % | 92,75 % |
| %S_cpu, barrage 30 detik | 0,30 % | 64,65 % |
| Rasio dilayani saat barrage | 15,4 % | 49,3 % |
| Paket rusak/hilang | 0 | 0 |

**h. Papan pencatat tahap L1 dibangun tetapi belum dapat ditanam.** Rencana enam tahap (L1–L6) disusun. Ditemukan bahwa papan host dan papan penyerang berjalan dari osilator internal tanpa kristal (toleransi sekitar ±1 %), sehingga papan pencatat berkristal menjadi acuan waktu yang lebih teliti. Sambungan pemrograman ke papan pencatat gagal berulang kali. Penyebab yang ditemukan antara lain kabel 3,3 V yang putus, tetapi masalahnya belum tuntas hari itu.

**Kaitan dengan Bab 4:** hari ini menghasilkan angka validasi awal semua metrik utama. Hasil awal ini konsisten dengan data final dua hari kemudian, sehingga memperkuat keyakinan terhadap data final.

---

### 4.5 Jumat, 14 November 2026: Papan pencatat dan sensor arus

**Tujuan:** menyiapkan papan pencatat dan sensor INA226.

**Kegiatan dan hasil:**

1. **Papan pencatat sementara STM32F030C8.** Karena hanya papan ini yang tersedia, papan pencatat sementara memakai STM32F030C8, sementara STM32F103C8 dipesan. Tahap L1 (jam kristal benar, 48 MHz) **LULUS**.
2. **Tahap L2, sensor INA226: komunikasi LULUS.** Sensor terdeteksi pada alamat **0x44**, bukan 0x40 seperti di proposal. Identitas chip dan pengaturan terbaca benar. Hambatan shunt pada modul berkode R100 (0,100 ohm), sehingga resolusi arus 25 µA dan jangkauan ±819 mA.
3. **Tahap L3, perekaman kontinu: LULUS.** Laju perekaman nyata **495,7 cuplikan per detik**, bukan sekitar 450 seperti perkiraan proposal. Sensor ini ternyata bekerja sekitar 8 % lebih cepat dari spesifikasi nominal. Konsekuensinya, perhitungan energi wajib memakai cap waktu nyata tiap cuplikan, bukan selang nominal. Satu kesalahan pada penghitung waktu ditemukan dan diperbaiki.
4. **Papan pencatat diganti ke STM32F103C8 yang baru tiba.** Papan ini kemungkinan besar klon: nomor identitas uniknya nol semua. Akibatnya Windows menolak mengenali port USB-nya. Masalah ini diatasi dengan mengisi nomor seri USB secara manual. Pengiriman data dari papan pencatat ke komputer berfungsi, tetapi pengiriman perintah dari komputer ke papan pencatat lewat USB gagal (kemudian diatasi dengan cara lain pada 15 November).
5. **L2 dan L3 dipindah ke F103: LULUS**, dengan laju 495,75 cuplikan/detik, identik dengan hasil pada F030. Ini membuktikan bahwa laju tersebut adalah sifat modul sensor, bukan sifat papan pencatat.
6. **Tahap L4, penangkap tepi sinyal pin status:** kode selesai, tetapi uji dilakukan pada 15 November. Ditemukan bahwa pewaktu yang dipakai hanya 16 bit (proposal menduga 32 bit), sehingga dibuat penghitung limpahan untuk memperpanjang cap waktu.

**Kaitan dengan Bab 4:** dasar keabsahan alat ukur arus (laju cuplikan, resolusi) dan sejumlah koreksi spesifikasi alat terhadap proposal.

---

### 4.6 Sabtu, 15 November 2026: Validasi alat ukur, sistem kendali run, uji coba, dan pengambilan data final

Hari ini merupakan hari terpenting karena seluruh sistem diintegrasikan dan **data final 120 run diambil**.

**a. Uji penangkap sinyal pin (L4).**
- Percobaan pertama gagal. Penyebabnya kabel pin status papan host yang masih tertancap di pin yang sama dengan generator uji, sehingga kedua sinyal saling melawan. Dugaan yang sempat dibuat (kesalahan pada chip klon) terbukti salah dan dicatat agar tidak terulang.
- Setelah kabel dicabut, uji **LULUS sempurna**: lebar pulsa 2.999,944 µs dan periode 9.999,916 µs, persis sama dengan sinyal uji, tanpa satu pun tepi terlewat dalam 5 detik.
- Generator uji kemudian dibuat mati secara bawaan supaya aman bila jumper lupa dilepas.

**b. Uji dengan pin status papan host sungguhan: LULUS, disertai temuan galat jam.**
- Persentase aktif menurut papan host 65,72 %, menurut papan pencatat 65,75 %. Kedua cara pengukuran cocok.
- **Temuan: jam papan host sekitar 0,55 % lebih lambat** (frekuensi efektif sekitar 47,74 MHz, bukan 48 MHz). Masih dalam toleransi pabrik (±1 %). Akibatnya, latensi yang dihitung dengan anggapan 48 MHz sedikit lebih kecil dari waktu nyata. Untuk itu semua latensi final dikoreksi dengan jam kristal papan pencatat.
- Rekaman panjang sekitar 11 menit menegaskan galat **−5.465 ppm (−0,547 %)** tanpa pergeseran selama rekaman.

**c. Validasi sensor arus.**
- Pin tegangan INA226 disambungkan; tegangan bus terbaca sekitar 5,04 V, dan perkalian tegangan × arus cocok dengan register daya sensor.
- Offset tanpa beban sekitar −50 µA, masih dalam spesifikasi pabrik. Nilai ini dicatat dan dikurangkan saat pengolahan data.
- **Uji akurasi dengan resistor beban belum tuntas.** Dengan dua resistor berbeda, sensor membaca arus sekitar 17–20 % lebih kecil dari perkiraan. Sensor terbukti linear, sehingga galatnya berupa faktor skala tetap. Dugaan terkuat: hambatan shunt efektif modul sekitar 0,080–0,083 ohm walaupun berkode 0,100 ohm. **Dampaknya: persentase perbandingan antar-kelompok tidak terpengaruh, tetapi nilai mutlak mA, mW, dan mJ perlu dikoreksi setelah kalibrasi.** Untuk itu data mentah disimpan dalam bentuk tegangan shunt agar dapat dihitung ulang tanpa mengulang pengukuran.

**d. Pemulihan otomatis sensor.** Kode pemulihan otomatis bila sensor berhenti menjawab ditulis. Uji cabut-pasangnya ditunda atas keputusan peneliti, tetapi selama pengambilan data tidak pernah terjadi kesalahan komunikasi sensor.

**e. Sistem kendali run: LULUS uji bangku.**
- Papan host menerima perintah dari papan pencatat lewat kabel serial, lalu mengirim laporan hasil setelah run selesai. Dengan begitu, pengiriman data tidak mengganggu pengukuran.
- Papan pencatat merekam arus, sinyal pin, dan laporan host dalam satu basis waktu, lalu mengalirkannya ke komputer.
- Karena jalur perintah USB pada papan pencatat klon gagal, perintah dikirim melalui pengaturan kecepatan port sebagai sandi perintah.
- Komputer baru menyalakan papan penyerang setelah papan host menyatakan siap.
- Hasil uji awal sudah menunjukkan pola yang sama dengan data final.

**f. Uji jalur kesalahan: LULUS 13/13.** Kesalahan sengaja dipaksakan: papan host dihentikan sebelum, selama, dan setelah run, serta laporan dipotong. Semua kesalahan terdeteksi dengan benar, dan sistem pulih tanpa merusak run berikutnya.

**g. Temuan: alat pemrogram menaikkan arus tidur.** Setelah ST-Link dipakai, terpasang pengaturan debug yang membuat jam prosesor tetap berjalan saat tidur, sehingga arus tidur naik 0,2–0,7 mA. Prosedur penonaktifan setelah pemrograman ditambahkan ke skrip.

**h. Uji ST-Link terpasang vs dicabut.** Arus diam sedikit berbeda (0,03–0,12 mA), dan perbedaannya tidak sama antara software dan DS28E38. **Keputusan: data final diambil dengan ST-Link dicabut**, dan urutan diacak per blok, bukan per run.

**i. Uji integrasi (8/8 lolos) dan pilot (16/16 lolos).** Pilot tiga run per kelompok menunjukkan:
- variasi antar-run sangat kecil (koefisien variasi arus ≤ 0,07 %), sehingga 30 run per kelompok jauh lebih dari cukup;
- sumber ragam terbesar justru perbedaan antar-sesi karena tegangan USB berubah. Karena itu dipakai pola blok berselang-seling (ABBA), dan setiap blok diapit pengukuran diam.

**j. Penetapan metrik tingkat layanan (TL) dan permintaan terlewat.** Papan host hanya mampu menyimpan satu permintaan yang menunggu (tanpa antrean). Permintaan yang tiba selagi sibuk digabung menjadi satu. Inilah yang didefinisikan sebagai "terlewat". Ditambahkan juga metrik throughput, kapasitas maksimum (1/latensi), model prediksi TL, dan energi dinamis per layanan (E_lyn). Pada data pilot, TL terukur cocok dengan model sampai selisih 0,2 poin.

**k. Pengambilan data final (tahap D): SELESAI dan MEMENUHI STANDAR.** Dilaksanakan sekitar pukul 17.30–21.30. ST-Link papan host dan papan pencatat dicabut, dan daya papan host langsung dari port komputer.

| Blok | Isi | Run | Sah |
|---|---|---|---|
| 1 | Software 10 normal + 10 barrage | 22 | 21 |
| 2 | DS28E38 20 normal + 20 barrage | 42 | 39 |
| 3 | Software 21 normal + 20 barrage | 43 | 37 |
| 3b | Software 4 normal + 2 barrage (pengganti) | 8 | 8 |
| 4 | DS28E38 12 normal + 11 barrage (termasuk pengganti) | 25 | 25 |

Hasil: **K1 = K2 = K3 = K4 = 30 run sah.**

Masalah selama tahap D dan penanganannya:
1. **Sebagian baris data hilang di jalur USB** pada 10 run. Penyebabnya batas waktu pengiriman USB bawaan yang terlalu pendek (3 ms). Batas itu diperbesar tanpa mengubah kode pengukuran. Setelah perbaikan, 33/33 run lolos. Run dengan data hilang digugurkan.
2. **Aturan "terlambat ringan"** (keputusan peneliti): run tetap sah bila paling banyak satu cuplikan sensor terlewat per celah dan jumlahnya ≤ 10. Ini dibuktikan tidak mengubah hasil (selisih arus ≤ 0,008 mA).
3. **Pergeseran arus tetap +0,78 mA** muncul setelah kabel USB papan pencatat dicabut-pasang ketika papan host menyala. Diduga papan host menerima daya balik lewat jalur sinyal antarpapan. Pergeseran hilang setelah papan host dimatikan lalu dinyalakan kembali. Blok yang terdampak digugurkan dan diulang, dan prosedur baru ditetapkan: setelah ada papan yang dicabut-pasang, papan host dimatikan lalu dinyalakan kembali, kemudian arus diamnya diperiksa.

**l. Skrip analisis statistik** ditulis: pemeriksaan keutuhan data, statistik deskriptif, uji beda, gain, grafik, dan lain-lain. Hasil pertama sudah memenuhi standar.

**Kaitan dengan Bab 4:** seluruh angka utama Bab 4 berasal dari tahap D. Uraian masalah dan penanganannya menjadi bahan subbab keabsahan data dan prosedur.

---

### 4.7 Minggu, 16 November 2026: Analisis final, verifikasi kode, dan revisi Bab 3

**Kegiatan dan hasil:**

1. **Refaktor kode papan host dan verifikasi.** Bagian kode yang sama untuk software dan DS28E38 dijadikan satu berkas bersama, sehingga metode pengukuran keduanya dijamin identik. **Isi memori program yang ditanam terbukti identik bit per bit** dengan firmware yang dipakai saat tahap D. Artinya, data tahap D tetap sah untuk kode yang disimpan di repositori.
2. **Analisis ulang sesuai Bab 3 versi 4, lalu disederhanakan (versi 5).** Uji utama memakai uji-t Welch dengan koreksi Holm atas enam perbandingan, dan Mann-Whitney sebagai pemeriksaan. Interval kepercayaan memakai bootstrap 10.000 kali. Ditambahkan analisis interaksi (selisih dari selisih), tabel tingkat layanan, kesesuaian dua instrumen (Bland-Altman), uji tanpa satu blok (*leave-one-block-out*), dan tabel ketidakpastian. Hasil akhir tercantum di Bagian 5.
3. **Daftar revisi Bab 3 (v4 ke v5)** disusun.
4. **Perapian kode per papan (tahap 1–5).** Tiap tahap diperiksa dengan membandingkan sidik digital firmware. Seluruh 11 firmware identik dengan acuan, termasuk firmware tahap D.

**Masih terbuka:** kalibrasi shunt INA226 dengan multimeter pinjaman dan pembaruan laporan progres.

---

## 5. Rekapitulasi Hasil Akhir (Pengambilan Data Tahap D)

Seluruh angka di bagian ini berasal dari analisis tanggal 16 November 2026 pukul 01.25 (120 run sah, 30 per kelompok). Angka ditulis sebagai rerata, dengan simpangan baku dalam kurung. **Nilai mutlak arus, daya, dan energi masih memakai shunt nominal 0,100 ohm** (lihat Bagian 9).

### 5.1 Tabel hasil utama

| Besaran | K1 Software normal | K2 DS28E38 normal | K3 Software barrage | K4 DS28E38 barrage |
|---|---|---|---|---|
| Latensi tanda tangan (ms, waktu nyata) | 655,40 (0,07) | 202,15 (0,03) | 655,51 (0,09) | 202,17 (0,03) |
| Persentase tidur CPU (%) | 35,46 (0,02) | 92,99 (0,0001) | 0,38 (0,002) | 64,72 (0,001) |
| Daya dinamis (mW) | 23,25 (0,62) | 5,30 (0,10) | 35,11 (0,74) | 23,05 (0,14) |
| Arus dinamis (mA) | 4,69 (0,02) | 1,02 (0,02) | 7,09 (0,01) | 4,46 (0,02) |
| Energi per permintaan terlayani (mJ) | 23,83 (0,63) | 5,42 (0,10) | 23,32 (0,49) | 4,68 (0,03) |
| Muatan per layanan (mC) | 4,81 (0,02) | 1,04 (0,02) | 4,71 (0,005) | 0,91 (0,003) |
| Daya rata-rata total (mW) | 82,46 (0,81) | 67,76 (0,13) | 94,28 (0,88) | 85,50 (0,23) |
| Arus rata-rata total (mA) | 16,25 (0,04) | 13,08 (0,07) | 18,66 (0,03) | 16,53 (0,07) |
| Tingkat layanan (%) | 100 | 100 | 15,04 | 49,25 |
| Permintaan masuk / dilayani / terlewat | 59 / 59 / 0 | 59 / 59 / 0 | 604–605 / 91 / 513–514 | 603–605 / 297–298 / 306–307 |
| Throughput (tanda tangan/detik) | 0,976 | 0,979 | 1,506 | 4,927 |
| Kapasitas maksimum (tanda tangan/detik) | 1,53 | 4,95 | 1,53 | 4,95 |

### 5.2 Perbandingan software dan DS28E38 (RM1, RM3, H1a–H1c)

Arah positif berarti DS28E38 lebih baik.

| Skenario | Besaran | Software | DS28E38 | Perbaikan [IK 95 %] | Keputusan |
|---|---|---|---|---|---|
| Normal | Latensi | 655,4 ms | 202,2 ms | lebih cepat 69,16 % (selisih 453,2 ms; 3,24 kali) | H0 ditolak |
| Normal | Daya dinamis | 23,25 mW | 5,30 mW | hemat 77,19 % [76,94; 77,46] | H0 ditolak |
| Normal | Tidur CPU | 35,46 % | 92,99 % | naik 57,53 poin | H0 ditolak |
| Normal | Energi per layanan | 23,83 mJ | 5,42 mJ | hemat 77,27 % (4,4 kali) | pendukung |
| Normal | Arus dinamis | 4,69 mA | 1,02 mA | hemat 78,27 % | pendukung |
| Normal | Daya rata-rata total | 82,46 mW | 67,76 mW | hemat 17,82 % | pendukung |
| Barrage | Latensi | 655,5 ms | 202,2 ms | lebih cepat 69,16 % (selisih 453,3 ms) | H0 ditolak |
| Barrage | Daya dinamis | 35,11 mW | 23,05 mW | hemat 34,36 % [33,86; 34,89] | H0 ditolak |
| Barrage | Tidur CPU | 0,38 % | 64,72 % | naik 64,34 poin | H0 ditolak |
| Barrage | Energi per layanan | 23,32 mJ | 4,68 mJ | hemat 79,94 % (5,0 kali) | pendukung |
| Barrage | Arus dinamis | 7,09 mA | 4,46 mA | hemat 37,06 % | pendukung |
| Barrage | Daya rata-rata total | 94,28 mW | 85,50 mW | hemat 9,32 % | pendukung |
| Barrage | Tingkat layanan | 15,04 % | 49,25 % | naik 34,21 poin | (H4) |
| Barrage | Throughput | 1,506/detik | 4,927/detik | 3,27 kali | (H4) |

Keenam uji hipotesis utama menolak H0 dengan nilai p terkoreksi Holm jauh di bawah 0,05 (nilai terbesar sekitar 7 × 10⁻³⁹). Sebaran kedua kelompok tidak pernah tumpang tindih pada satu pun besaran. Pemeriksaan Mann-Whitney memberi kesimpulan yang sama.

### 5.3 Pengaruh serangan barrage pada tiap cara (RM2)

| Cara | Besaran | Normal | Barrage | Perubahan |
|---|---|---|---|---|
| Software | Tidur CPU | 35,46 % | 0,38 % | turun 35,08 poin (praktis tidak pernah tidur) |
| Software | Daya dinamis | 23,25 mW | 35,11 mW | naik 51 % |
| Software | Tingkat layanan | 100 % | 15,04 % | turun 84,96 poin |
| Software | Throughput | 0,976/detik | 1,506/detik | naik 54 % (sudah di batas kapasitas) |
| Software | Energi per layanan | 23,83 mJ | 23,32 mJ | turun 2,1 % |
| DS28E38 | Tidur CPU | 92,99 % | 64,72 % | turun 28,27 poin |
| DS28E38 | Daya dinamis | 5,30 mW | 23,05 mW | naik 335 % |
| DS28E38 | Tingkat layanan | 100 % | 49,25 % | turun 50,75 poin |
| DS28E38 | Throughput | 0,979/detik | 4,927/detik | naik 403 % (sudah di batas kapasitas) |
| DS28E38 | Energi per layanan | 5,42 mJ | 4,68 mJ | turun 13,6 % |
| Kedua cara | Latensi | — | — | berubah kurang dari 0,02 % (praktis tetap) |

### 5.4 Interaksi cara dan skenario (RM3, hipotesis interaksi)

Interaksi diukur sebagai "selisih dari selisih": keunggulan DS28E38 saat barrage dikurangi keunggulannya saat normal.

| Besaran | Keunggulan DS28E38 saat normal | Keunggulan saat barrage | Selisih dari selisih [IK 95 %] | Arti |
|---|---|---|---|---|
| Tidur CPU | 57,53 poin | 64,34 poin | +6,80 [6,795; 6,809] poin | keunggulan lebih besar saat diserang (**hipotesis interaksi didukung**) |
| Daya dinamis | 17,95 mW | 12,07 mW | −5,88 [−6,22; −5,53] mW | keunggulan lebih kecil saat diserang |
| Arus dinamis | 3,67 mA | 2,63 mA | −1,04 mA | keunggulan lebih kecil saat diserang |
| Energi per layanan | 18,41 mJ | 18,64 mJ | +0,23 [−0,05; 0,52] mJ | tidak terbukti berbeda |
| Latensi | 453,25 ms | 453,33 ms | +0,08 ms | praktis sama |

### 5.5 Tingkat layanan dan kesesuaian model (H4)

| Kelompok | TL terukur | TL model | Selisih |
|---|---|---|---|
| K1 | 100 % | 100 % | 0 |
| K2 | 100 % | 100 % | 0 |
| K3 | 15,04 % | 15,24 % | −0,20 poin |
| K4 | 49,25 % | 49,44 % | −0,19 poin |

Ketiga kriteria H4 terpenuhi: TL sama-sama 100 % pada kondisi normal, TL DS28E38 lebih tinggi saat barrage, dan selisih terhadap model kurang dari 1 poin. **H4 didukung.**

### 5.6 Ringkasan status hipotesis

| Hipotesis | Isi ringkas | Status |
|---|---|---|
| H1a | Latensi software dan DS28E38 berbeda | Didukung pada kedua skenario (DS28E38 69,2 % lebih cepat) |
| H1b | Daya dinamis berbeda | Didukung pada kedua skenario (hemat 77,2 % normal, 34,4 % barrage) |
| H1c | Persentase tidur CPU berbeda | Didukung pada kedua skenario (+57,5 dan +64,3 poin) |
| Interaksi | Selisih tidur CPU lebih besar saat barrage | Didukung (+6,80 poin) |
| H4 | DS28E38 meningkatkan tingkat layanan saat barrage tanpa menurunkannya saat normal | Didukung |

---

## 6. Pembahasan Pendukung

Bagian ini menjelaskan mengapa hasil di atas terjadi, dengan bahasa sederhana. Uraiannya dapat dikembangkan menjadi subbab pembahasan.

### 6.1 Mengapa DS28E38 lebih cepat

Pada cara software, prosesor 48 MHz harus mengerjakan sendiri perhitungan kurva eliptik yang sangat berat (sekitar 655 ms). DS28E38 memiliki rangkaian khusus untuk perhitungan itu, sehingga satu tanda tangan, termasuk waktu komunikasi melalui jalur 1-Wire, selesai dalam sekitar 202 ms. Latensi kedua cara hampir tidak berubah antara kondisi normal dan barrage, karena satu tanda tangan selalu dikerjakan dengan cara yang sama. Serangan hanya menambah jumlah permintaan, bukan memperlambat tiap tanda tangan.

**Catatan penting untuk penulisan:** besar keunggulan ini bergantung pada konfigurasi Mbed TLS. Penelitian ini memakai konfigurasi 1c, yaitu tabel percepatan comb dimatikan (655 ms). Bila tabel itu dinyalakan, logbook 13 November mencatat software sekitar 278,6 ms, sehingga DS28E38 hanya sekitar 1,4 kali lebih cepat (angka ini dari pengukuran pada tahap pengembangan, bukan dari protokol tahap D). Pilihan konfigurasi ini wajib disebutkan beserta alasannya di Bab 3 dan Bab 4.

### 6.2 Mengapa prosesor lebih banyak tidur pada DS28E38

Pada cara software, prosesor terjaga selama seluruh perhitungan (sekitar 650 ms per permintaan). Pada DS28E38, prosesor hanya terjaga saat mengirim perintah dan membaca hasil lewat jalur 1-Wire (sekitar 71 ms per permintaan), lalu tidur selama chip bekerja. Karena itu, pada kondisi normal prosesor tidur 93 % waktu dengan DS28E38, dibandingkan hanya 35 % dengan software.

Saat diserang, perbedaannya makin besar. Prosesor pada cara software praktis tidak pernah tidur (0,38 %), sedangkan dengan DS28E38 prosesor masih tidur hampir dua pertiga waktu (64,7 %). Inilah inti ketahanan terhadap *Denial of Sleep*.

### 6.3 Persentase tidur tinggi bukan karena DS28E38 malas melayani

Kekhawatiran yang wajar adalah bahwa prosesor DS28E38 tampak banyak tidur karena melayani lebih sedikit permintaan. Data menunjukkan sebaliknya: saat barrage, DS28E38 melayani **297 permintaan per menit, sedangkan software hanya 91** (3,27 kali lebih banyak), dan prosesor DS28E38 tetap tidur 64,7 % waktu. Pada kondisi normal, keduanya melayani 100 % permintaan, sehingga perbedaan daya dan tidur di kondisi normal murni akibat cara kerjanya, bukan akibat jumlah pekerjaan.

### 6.4 Mengapa penghematan daya dinamis lebih kecil saat diserang

Saat normal, DS28E38 menghemat daya dinamis 77 %, tetapi saat barrage hanya 34 %. Hal ini bukan karena DS28E38 memburuk. DS28E38 justru mengerjakan 3,3 kali lebih banyak tanda tangan, dan setiap tanda tangan membutuhkan energi. Bila dihitung per tanda tangan yang selesai (E_lyn), penghematan DS28E38 tetap sekitar 77–80 % pada kedua skenario, dan perbedaan antar-skenario tidak signifikan. Jadi:

- **daya dinamis** menggambarkan beban energi total selama diserang;
- **energi per layanan** menggambarkan efisiensi tiap pekerjaan, dan ukuran ini paling adil untuk membandingkan kedua cara.

### 6.5 Mengapa tingkat layanan turun saat diserang

Permintaan datang setiap 100 ms, sedangkan satu tanda tangan software butuh 655 ms dan DS28E38 202 ms. Kedua cara tidak mampu mengikuti laju tersebut, sehingga permintaan yang datang selagi sibuk digabung (terlewat). Kapasitas maksimum masing-masing adalah 1 dibagi latensi, yaitu sekitar 1,53 tanda tangan/detik untuk software dan 4,95 untuk DS28E38. Throughput terukur sudah menyentuh batas ini (1,506 dan 4,927), artinya kedua cara bekerja pada kapasitas penuh. Model sederhana TL = 1 / (laju permintaan × latensi) memprediksi TL dengan selisih hanya 0,2 poin. Ini membuktikan bahwa permintaan terlewat disebabkan lamanya tanda tangan, bukan gangguan jalur komunikasi (jumlah paket rusak 0).

Model ini juga memungkinkan generalisasi: pada laju permintaan di bawah 1,53/detik keduanya melayani 100 %, antara 1,53 dan 4,95/detik hanya DS28E38 yang masih melayani semua, dan di atas 4,95/detik keduanya jenuh.

### 6.6 Daya rata-rata total

Daya total papan host mencakup konsumsi dasar saat diam (sekitar 59–62 mW, termasuk regulator, LED, dan komponen papan), sehingga penghematan total lebih kecil daripada penghematan dinamis: 17,8 % saat normal dan 9,3 % saat barrage. Arus diam papan dengan DS28E38 sedikit lebih tinggi (sekitar 0,5 mA) karena chip DS28E38 dan resistor pull-up jalur 1-Wire ikut menarik arus. Hal ini sudah tercakup dalam perhitungan daya dinamis, karena daya dinamis dihitung terhadap kondisi diam arsitektur yang sama di blok yang sama.

### 6.7 Pemakaian memori

Software memakai RAM puncak sekitar 1,7–1,8 kali lebih besar daripada DS28E38 (10.116 B vs 6.056 B pada build tahap 4). Selisih ini berasal dari pustaka kripto. Hasil ini dapat dipakai sebagai temuan pendukung. Perlu dicatat bahwa angka tersebut diukur sebelum penambahan larik penyimpan latensi pada tahap 5 (sekitar 1,6 KB), yang merupakan instrumen pengukuran, bukan bagian dari fungsi tanda tangan.

### 6.8 Aspek keamanan

- Pada DS28E38, kunci rahasia tidak pernah berada di memori mikrokontroler. Ini merupakan fakta desain, bukan hasil uji ketahanan fisik.
- Melayani permintaan penyerang tidak membocorkan kunci, karena ECDSA aman terhadap pengumpulan tanda tangan dan yang ditandatangani adalah nilai tetap.
- Hardware offloading **mengurangi dampak** serangan (prosesor tetap bisa tidur dan lebih banyak permintaan terlayani), tetapi **tidak mencegah atau menyaring** permintaan penyerang. Penyaringan dan autentikasi permintaan menjadi saran pengembangan.
- Ketahanan fisik DS28E38 (ChipDNA PUF, die shield) hanya merujuk spesifikasi pabrikan dan **tidak diuji** dalam penelitian ini.

---

## 7. Bukti Keandalan dan Keabsahan Data

Bagian ini dapat dipakai untuk subbab validasi instrumen dan keabsahan data di Bab 4.

### 7.1 Keutuhan data

- Target 30 run sah per kelompok tercapai untuk keempat kelompok.
- Dari 140 run dalam sesi resmi (termasuk run diam), 10 run digugurkan. Semuanya karena baris data hilang di jalur USB, yang dideteksi dengan pencocokan jumlah dan checksum. Selain itu, 4 sesi (18 run) digugurkan utuh karena kondisi yang tidak memenuhi syarat, misalnya pergeseran arus +0,78 mA. Run gugur tidak dianalisis.
- Jumlah paket permintaan yang rusak atau hilang di jalur penyerang–host: **0** pada seluruh run sah.
- Kesalahan komunikasi sensor arus: **0**.

### 7.2 Kesesuaian dua cara pengukuran

| Pasangan yang dibandingkan | Selisih rata-rata | Arti |
|---|---|---|
| Persentase tidur: pin (papan pencatat) vs firmware, 120 run | −0,0013 poin (batas 95 %: −0,0037 s.d. 0,0010) | dua cara pengukuran hampir identik |
| Latensi DS28E38: pin vs penghitung siklus, 1.770 tanda tangan | +0,023 ms | hampir identik |
| Latensi software: pin vs penghitung siklus, 1.770 tanda tangan | +6,06 ms (tetap) | selisih tetap karena pin juga mencakup kerja sebelum tanda tangan dihitung (pengumpulan sumber acak dan persiapan) |
| Model arus (arus aktif × proporsi aktif + arus tidur × proporsi tidur) vs arus terukur | maksimal 0,0034 mA | data arus konsisten dengan sinyal pin |

### 7.3 Kestabilan selama pengambilan data

- Pergeseran arus diam di awal dan akhir tiap blok paling besar 0,032 mA, di bawah batas 0,05 mA yang ditetapkan.
- Uji tanpa satu blok (*leave-one-block-out*): bila salah satu blok dikeluarkan, penghematan daya dinamis tetap 77,0–77,8 % (normal) dan 33,8–35,8 % (barrage). Latensi dan persentase tidur tidak berubah. Jadi kesimpulan tidak bergantung pada satu blok tertentu.
- Galat jam papan host per run: −5.119 s.d. −4.238 ppm. Galat ini dikoreksi untuk setiap run memakai jam kristal papan pencatat.

### 7.4 Ketidakpastian pengukuran

| Besaran | Ketidakpastian gabungan (relatif) | Komponen dominan |
|---|---|---|
| Latensi | 0,01–0,02 % | sangat kecil |
| Persentase tidur CPU | ≤ 0,31 % | sangat kecil |
| Daya dinamis, energi per layanan | sekitar 5–6 % | asumsi ketidakpastian kalibrasi shunt 5 % |
| Arus dinamis, muatan per layanan | sekitar 5–6 % | asumsi ketidakpastian kalibrasi shunt 5 % |

Ketidakpastian kalibrasi bekerja sebagai faktor pengali yang sama untuk semua kelompok, sehingga **saling meniadakan pada persentase penghematan**. Persentase penghematan (gain) jauh lebih pasti daripada nilai mutlaknya.

### 7.5 Catatan tentang nilai p dan sebaran yang sangat kecil

Karena sebaran antar-run sangat kecil (alat dan perangkat sangat stabil), nilai p menjadi sangat kecil. Karena itu kesimpulan ditekankan pada besar selisih, persentase penghematan, dan interval kepercayaannya, bukan pada nilai p saja. Seluruh run berasal dari satu papan host dan satu chip DS28E38. Uji statistik menunjukkan bahwa perbedaan pada perangkat yang diuji bukan akibat variasi acak antar-run, tetapi tidak menjamin besar perbedaan yang sama pada unit perangkat lain.

### 7.6 Ketertelusuran firmware

Sidik digital (sha256) firmware tahap D dicatat dalam berkas sesi. Setelah kode dirapikan, isi firmware terbukti identik bit per bit (software c55d8ca31cc0…, DS28E38 4df67c4f9975…, pencatat 7ac6b702300d…). Kode di repositori adalah kode yang benar-benar menghasilkan data.

---

## 8. Perubahan terhadap Rencana Awal (Proposal)

Perubahan berikut terjadi karena kondisi nyata alat dan perlu dijelaskan di Bab 3 atau Bab 4.

| Aspek | Rencana di proposal | Kondisi nyata | Alasan / dampak |
|---|---|---|---|
| Konfigurasi software | Mbed TLS, nonce RFC 6979 | Mbed TLS dengan pengerasan (nonce hedged, penghapusan kunci) dan tabel comb dimatikan (konfigurasi 1c) | Keputusan peneliti; latensi 655 ms. Harus dinyatakan eksplisit |
| Papan penyerang | Blue Pill STM32F103 | STM32F030C8 | Ketersediaan papan; fungsinya terverifikasi tepat |
| Papan pencatat | Blue Pill STM32F103 | STM32F103C8 klon (identitas unik nol) | Perlu perbaikan nomor seri USB; perintah lewat pengaturan kecepatan port |
| Alamat INA226 | 0x40 | 0x44 | Pengkabelan modul |
| Hambatan shunt | 0,5 atau 1 ohm | 0,100 ohm (nominal), efektif diduga 0,080–0,083 ohm | Modul yang tersedia; menunggu kalibrasi |
| Laju cuplikan | sekitar 450/detik | 495,75/detik | Sifat modul; energi dihitung dengan cap waktu nyata |
| Pewaktu penangkap pin | 32 bit | 16 bit + penghitung limpahan | Sifat chip; terbukti tepat |
| Acuan waktu | 48 MHz papan host | Jam kristal papan pencatat; jam host −0,55 % | Latensi dikoreksi per run |
| Pengacakan urutan | Acak penuh per run | Acak per blok dengan pola ABBA, diapit pengukuran diam | ST-Link harus dicabut; mengendalikan pergeseran tegangan USB |
| Metrik | Latensi, daya, tidur CPU | Ditambah tingkat layanan, throughput, energi per layanan, arus dinamis | Dampak serangan terhadap ketersediaan layanan belum terukur oleh tiga metrik awal |
| Uji statistik | Shapiro-Wilk, Levene, Welch/Mann-Whitney, ANOVA dua arah | Welch utama, Mann-Whitney sebagai pemeriksaan, Holm 6 perbandingan, bootstrap, selisih-dari-selisih | Data hampir konstan sehingga uji kenormalan dan ANOVA tidak tepat |

---

## 9. Keterbatasan dan Hal yang Belum Tuntas

1. **Kalibrasi shunt INA226 belum selesai.** Bukti awal menunjukkan arus terbaca sekitar 17–20 % lebih kecil dari sebenarnya. Persentase penghematan tidak terpengaruh, tetapi nilai mutlak mA, mW, dan mJ perlu dihitung ulang dengan nilai shunt hasil kalibrasi. Data mentah sudah disimpan sehingga tidak perlu pengukuran ulang.
2. **Akurasi INA226 terhadap multimeter belum diverifikasi.** Multimeter yang dipakai saat itu juga diragukan karena membaca tegangan sekitar 5 % lebih rendah.
3. **Satu papan host dan satu chip DS28E38.** Hasil berlaku untuk perangkat yang diuji. Variasi antar-unit tidak diukur.
4. **Konfigurasi software memengaruhi besar keunggulan hardware.** Dengan tabel comb aktif, keunggulan latensi DS28E38 jauh lebih kecil.
5. **Waktu pengumpulan sumber acak software tidak termasuk dalam latensi tanda tangan** (sekitar 6 ms per permintaan, terlihat dari selisih pin dan penghitung siklus). Sebaliknya, latensi DS28E38 sudah mencakup pembangkit acak internalnya.
6. **Arti "latensi"** di sini adalah lama pembuatan satu tanda tangan, bukan waktu tunggu permintaan sejak tiba sampai dilayani.
7. **Daya dinamis dipengaruhi tegangan USB** yang berbeda antar-blok (5,02–5,22 V). Arus dinamis dan muatan per layanan lebih stabil, sehingga dapat dijadikan pendamping atau metrik utama.
8. **Galat jam papan host** baru diukur pada satu papan dan tanpa pengukuran suhu ruang.
9. **Pemulihan otomatis sensor** sudah ditulis tetapi uji cabut-pasangnya belum dilakukan. Fitur ini tidak pernah terpicu selama pengambilan data.
10. **Kebijakan tanpa antrean** (permintaan yang datang saat sibuk digabung) adalah variabel kontrol. Dengan antrean, tingkat layanan bisa lebih tinggi tetapi waktu tunggu bertambah.
11. **Ketahanan fisik** DS28E38 tidak diuji.
12. **Penyebab pergeseran arus +0,78 mA** setelah cabut-pasang papan lain belum diketahui pasti. Dampaknya sudah dikendalikan dengan prosedur mematikan lalu menyalakan kembali papan host dan menggugurkan blok yang terdampak.

---

## 10. Bahan Penyusunan Kesimpulan dan Saran

Butir berikut adalah rancangan isi kesimpulan berdasarkan data. Angka energi mutlak perlu diperbarui setelah kalibrasi shunt.

### 10.1 Rancangan kesimpulan

1. **Jawaban RM1 (latensi dan energi per permintaan).** Pembuatan tanda tangan ECDSA P-256 dengan DS28E38 membutuhkan sekitar 202 ms, sedangkan software Mbed TLS (konfigurasi 1c) sekitar 655 ms, sehingga DS28E38 69,2 % lebih cepat (3,24 kali). Energi tambahan per tanda tangan yang selesai dengan DS28E38 sekitar 5,4 mJ (normal) dan 4,7 mJ (barrage), dibandingkan sekitar 23,3–23,8 mJ dengan software, atau penghematan sekitar 77–80 %.
2. **Jawaban RM2 (pengaruh serangan).** Serangan barrage membuat prosesor pada cara software hampir tidak pernah tidur (0,38 %) dan hanya 15 % permintaan terlayani. Pada DS28E38, prosesor masih tidur 64,7 % waktu dan 49 % permintaan terlayani. Latensi tiap tanda tangan tidak berubah oleh serangan pada kedua cara.
3. **Jawaban RM3 (besar selisih dan interaksi).** Keunggulan DS28E38 dalam persentase tidur CPU meningkat saat diserang (dari +57,5 menjadi +64,3 poin). Keunggulan daya dinamisnya mengecil (dari 77 % menjadi 34 %) karena DS28E38 mengerjakan 3,3 kali lebih banyak tanda tangan. Bila diukur per pekerjaan yang selesai, keunggulannya tetap sekitar 77–80 % pada kedua skenario.
4. **Hipotesis.** H1a, H1b, H1c, hipotesis interaksi untuk persentase tidur, dan H4 seluruhnya didukung oleh data.
5. **Kesimpulan umum.** Pengalihan pembuatan tanda tangan ke DS28E38 membuat perangkat lebih cepat, lebih hemat energi per pekerjaan, lebih banyak tidur, dan mampu melayani lebih banyak permintaan saat diserang. Dengan demikian dampak serangan *Denial of Sleep* berkurang, walaupun serangannya tidak tercegah.

### 10.2 Rancangan saran

1. Melakukan kalibrasi shunt dengan acuan presisi (resistor 1 % atau multimeter laboratorium) untuk memperoleh nilai mutlak energi yang tepat.
2. Mengulang pengujian pada beberapa papan dan beberapa chip DS28E38 untuk melihat variasi antar-unit.
3. Membandingkan beberapa konfigurasi software (tabel comb aktif, pustaka lain seperti micro-ecc) agar gambaran keunggulan hardware lebih lengkap.
4. Menambahkan penyaringan atau pembatasan laju permintaan (misalnya autentikasi permintaan) agar serangan dapat dicegah, bukan hanya dikurangi dampaknya.
5. Menguji mode tidur yang lebih dalam (Stop/Standby) dan pemakaian baterai sungguhan.
6. Mempertimbangkan antrean permintaan dan mengukur waktu tunggu permintaan, bukan hanya lama tanda tangan.
7. Mengukur pengaruh suhu terhadap jam papan dan sensor.

---

## 11. Lampiran

### Lampiran A. Kronologi ringkas

| Tanggal | Kegiatan utama | Status akhir hari |
|---|---|---|
| 10 Nov | Program penghubung DS28E38 (Python, C), program awal STM32 | Tanda tangan chip dapat diperiksa di komputer |
| 11 Nov | Percepatan DS28E38 (0,6 s menjadi 190 ms), Mbed TLS terberat (4,9 s), alat ukur jendela 60 detik | Alat siap, pengukuran belum |
| 12 Nov | Kajian Bab 3, aturan build from base, software tahap 1 (276,4 ms), papan penyerang tahap 1 | Keduanya lulus |
| 13 Nov | Software tahap 1b/1c/2/3/4, DS28E38 dibangun ulang tahap 1–4, papan pencatat L1 dibangun | Host lulus sampai permintaan nyata; pencatat belum tertanam |
| 14 Nov | Pencatat F030 sementara (L1–L3), INA226 0x44, 495,7 cuplikan/detik, ganti ke F103 klon, perbaikan USB | Sensor dan perekaman lulus |
| 15 Nov | L4 lulus, galat jam host −0,55 %, validasi sensor, sistem kendali run, uji kesalahan 13/13, pilot, metrik TL, **tahap D 120 run**, skrip analisis | Data final memenuhi standar |
| 16 Nov | Verifikasi kode identik, analisis final (Bab 3 v5), revisi Bab 3, perapian kode | Analisis final selesai; kalibrasi shunt tersisa |

### Lampiran B. Kaitan bagian dokumen ini dengan Bab 4

| Subbab Bab 4 (usulan) | Bahan di dokumen ini | Gambar/tabel terkait (panduan grafik Bab 4) |
|---|---|---|
| Hasil perancangan dan pengujian sistem uji | Bagian 4.3–4.6, Bagian 8 | — |
| Validasi instrumen dan keutuhan data | Bagian 7 | Tabel D (sebaran run per blok) |
| Hasil latensi (RM1, H1a) | Bagian 5.1–5.2, 6.1 | Gambar 4.2 |
| Hasil persentase tidur CPU (H1c) | Bagian 5.1–5.2, 6.2–6.3 | Gambar 4.3 |
| Hasil daya dan energi (RM1, H1b) | Bagian 5.1–5.2, 6.4, 6.6 | Gambar 4.1, 4.4, 4.5, Tabel C |
| Pengaruh serangan (RM2) | Bagian 5.3 | Tabel B |
| Interaksi (RM3) | Bagian 5.4 | Gambar 4.6 |
| Tingkat layanan dan throughput (H4) | Bagian 5.5, 6.5 | Gambar 4.7, 4.8 |
| Uji hipotesis | Bagian 5.2, 5.6 | Tabel A |
| Keterbatasan penelitian | Bagian 9 | — |

### Lampiran C. Lokasi data dan berkas di repositori

| Isi | Lokasi |
|---|---|
| Logbook harian | `logbook/2026-11-10.md` s.d. `logbook/2026-11-16.md` |
| Data mentah tahap D (per sesi) | `data/runs/<sesi>/` (daftar sesi di `data/tahap_d.txt`) |
| Run dan sesi gugur | `data/dev/gugur/` |
| Hasil analisis final | `data/analisis/20261116_012530_r0.1/` (`ringkasan.md`, `pemeriksaan.md`, berkas CSV, `gambar/`) |
| Rekaman galat jam host | `data/a3_clock_20261115_1259_run1.txt` |
| Log uji jalur kesalahan | `data/dev/err/` |
| Revisi Bab 3 (v4 ke v5) | `laporan/revisi_bab3_v4_2026-11-16.md` |
| Panduan grafik Bab 4 | `laporan/panduan_grafik_bab4_2026-11-16.md` |
| Laporan progres dan revisi paper | `laporan/laporan_progres_revisi_paper_2026-11-15.md` |
| Skrip kendali run dan analisis | `tools/run_bench.py`, `tools/analisis_d.py` |
