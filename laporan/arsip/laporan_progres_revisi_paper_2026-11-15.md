# Laporan Progres dan Daftar Revisi Paper (acuan: SEMPRO_Syauqi_revisi_v3.pdf)

Tanggal: 15 November 2026
Status sistem: tahap A-C selesai, tahap D (pengambilan data final) sedang berjalan (blok 1 dari 4 selesai).
Rincian teknis harian ada di `logbook/2026-11-13.md` s.d. `logbook/2026-11-15.md`; laporan ini merangkum apa yang perlu diubah di paper.

Catatan angka: semua arus/daya di laporan ini masih memakai nilai shunt nominal 0,100 Ohm. Kalibrasi shunt (dijadwalkan 16 Nov) kemungkinan menaikkan arus absolut sekitar 17-20 %, tetapi TIDAK mengubah perbandingan K1-K4, S_cpu, latensi, maupun tingkat layanan. Data mentah disimpan, jadi semua angka bisa dihitung ulang tanpa mengambil data lagi.

---

## 1. Ringkasan progres

| Tahap | Isi | Status |
|---|---|---|
| A1 | Pemulihan bus I2C logger | Kode selesai, uji cabut-pasang ditunda (tidak pernah terpicu di ~200 run) |
| A2 | Kalibrasi INA226 | Offset -2 LSB selesai; nilai shunt efektif ~0,080-0,083 Ohm, BELUM final (multimeter lab 16 Nov) |
| A3 | Galat jam host | Selesai: jam host (HSI) lambat -0,45 s.d. -0,55 %; dikoreksi dengan jam kristal logger |
| B | Jalur kendali run (PC -> logger -> host, attacker otomatis) | Lulus (13 run) |
| B+ | Uji jalur galat (host diam, putus di tengah, tidak selesai, menolak) | Lulus 13/13 |
| B+ | Pengacakan K1/K2 dengan flash otomatis | Lulus 10/10 (akhirnya tidak dipakai, lihat 2.5) |
| C (langkah 8) | Integrasi K1-K4, ST-Link dicabut | Lulus 8/8 |
| C (langkah 9) | Pilot 3 run/kelompok | Lulus 16/16, CV arus <= 0,07 % |
| D | 120 run, pola blok ABBA | Blok 1 (K1) selesai: 21/22 sah (1 run gugur pemeriksaan keutuhan, diganti di blok 3); blok 2 (K2) berjalan |

Temuan penting selama pengerjaan:
1. **DBGMCU_CR**: OpenOCD/ST-Link menyalakan mode debug yang membuat jam inti tetap berjalan saat WFI; bit ini bertahan setelah ST-Link dicabut sampai daya diputus. Efeknya arus tidur naik 0,2-0,7 mA. Prosedur baru: setelah flash, bit dinolkan (`run_bench.py nodebug`).
2. **ST-Link terpasang mengurangi arus terbaca** 0,03 mA (K1) dan 0,11 mA (K2): data final diambil dengan ST-Link dicabut.
3. **Arus bergeser antar sesi** sekitar 1 %, mengikuti tegangan USB. Koreksi dengan IDLE di blok yang sama (P_dyn, E_lyn) menghilangkan pergeseran ini (E_lyn dua sesi berbeda 0,1-0,4 %).
4. **Tingkat layanan** (permintaan yang dilayani) ternyata deterministik dan cocok dengan model sederhana; diusulkan sebagai metrik tambahan (bagian 5).

---

## 2. Daftar perubahan paper (berdasarkan kondisi sistem sebenarnya)

Prioritas: **W** = wajib (paper tidak sesuai dengan yang dikerjakan), **S** = disarankan (memperjelas/menguatkan), **K** = menunggu keputusan.

### 2.1 Bab 1

| No | Lokasi v3 | Tertulis | Kondisi sebenarnya / usulan | Prio |
|---|---|---|---|---|
| 1 | 1.3 Batasan no. 1 | STM32F103 "72 MHz" | Host berjalan 48 MHz (HSI x PLL). Ganti menjadi 48 MHz agar konsisten dengan Tabel 3.1 dan 3.3.1. | W |
| 2 | 1.3 Batasan no. 4 | tiga metrik | Tambahkan metrik tingkat layanan (TL) dan permintaan terlewat bila diadopsi (bagian 5). | K |
| 3 | 1.3 Batasan no. 5 | "suhu ruangan standar (~25 °C)" | Tidak ada termometer di bangku; tulis "suhu ruang laboratorium (tidak dikendalikan, ruangan bersirkulasi udara)" atau sediakan termometer untuk sesi data. | S |
| 4 | 1.5 Manfaat bagi Industri | "solusi keamanan fisik berbiaya rendah ... tanpa mengorbankan ketahanan kriptografi" | Klaim keamanan fisik tidak diuji (lihat bagian 4). | W |

### 2.2 Bab 2

| No | Lokasi v3 | Tertulis | Usulan | Prio |
|---|---|---|---|---|
| 5 | 2.5.2 paragraf pembuka | "titik temu antara efisiensi energi ... dan penguatan keamanan fisik" | Lihat bagian 4. | W |
| 6 | 2.5.2 kontribusi no. 2 | "Perlindungan Kunci terhadap Node Tampering melalui ChipDNA PUF dan Die Shield" sebagai kontribusi | Bukan kontribusi penelitian ini karena tidak diuji; pindahkan ke alasan pemilihan DS28E38 (lihat bagian 4). | W |
| 7 | 2.5.2 kontribusi no. 3 | DS28E38 "hingga 130 ms per tanda tangan" | Tambahkan hasil ukur: latensi total DS28E38 (perintah 1-Wire + sign + baca hasil) 202,2 ms; CPU host hanya aktif ~72 ms per permintaan, sisanya tidur (WFI). | S |
| 8 | 2.6 Kerangka Berpikir (teks + Gambar 2.1) | "penggunaan memori" sebagai parameter utama | Metrik yang diukur: latensi, arus/daya/energi, %S_cpu (+ TL bila diadopsi). Hapus "penggunaan memori" atau jadikan data pendukung. | W |
| 9 | 2.6 Kerangka Berpikir | "kerentanan ... dan node tampering", "mengambil alih beban komputasi dan tugas keamanan" | Hapus "node tampering" dari rantai masalah yang diuji (bagian 4). | W |
| 10 | 2.7 Hipotesis | satu hipotesis gabungan | Tambahkan hipotesis TL bila diadopsi (bagian 5.4). | K |

### 2.3 Bab 3 - alat dan bahan (Tabel 3.1 dan 3.2)

| No | Lokasi v3 | Tertulis | Kondisi sebenarnya | Prio |
|---|---|---|---|---|
| 11 | Tabel 3.1 no. 3 (attacker node) | STM32F103C8T6 Blue Pill, catu USB tersendiri | **STM32F030C8T6** (Cortex-M0, tanpa USB). Frame dikirim lewat USART1 PA9 -> PA3 host melalui resistor seri ~1 kOhm (sesuai). Dikendalikan dan dicatu lewat konverter USB-serial CH340 di USART2 (perintah a/b/x/s dari skrip PC). Laju diatur pewaktu hardware TIM3. Frame 36 byte: 0xA5, nomor urut 16 bit, isi 32 byte, XOR. | W |
| 12 | Tabel 3.1 no. 4 (sumber daya) | papan, logger, attacker di port USB berbeda pada komputer yang sama | Papan host: USB-C langsung ke port komputer (tanpa hub). Logger dan attacker: lewat hub USB komputer yang sama. Tulis apa adanya. | W |
| 13 | Tabel 3.1 no. 5 (INA226) | shunt 0,5 atau 1 Ohm, alamat 0x40 | Modul INA226 dengan shunt bawaan berkode R100 (0,100 Ohm nominal; nilai efektif dikalibrasi, hasil ~0,080-0,083 Ohm, final 16 Nov), alamat **0x44** (A1 ke VS, A0 ke GND). Resolusi ~25-30 uA per LSB, rentang ~+-0,8-1 A. Pin VBUS INA226 disambung ke IN- (sisi papan). Resolusi tetap memadai: arus papan 11-19 mA, CV antar-run 0,01-0,07 %. | W |
| 14 | Tabel 3.1 no. 6 (logger) | STM32F103 Blue Pill | Sesuai (Blue Pill F103 klon) + kristal 8 MHz -> 72 MHz sebagai basis waktu tunggal (TIM2 72 MHz, 64 bit). Tambahkan bahwa logger juga meneruskan perintah run ke host (lihat 2.4). | S |
| 15 | Tabel 3.1 no. 9 | ST-Link dilepas saat pengukuran | Sesuai. Tambahkan prosedur menolkan DBGMCU_CR setelah flash (2.4 no. 22). | W |
| 16 | Tabel 3.2 no. 1 | STM32CubeIDE | **PlatformIO** (VS Code) + core STM32duino (Arduino), toolchain GCC ARM, **OpenOCD** untuk flash/debug. | W |
| 17 | Tabel 3.2 no. 2 dan 3.3.1 a.1 | mbedTLS, nonce deterministik RFC 6979 | Mbed TLS 3.6.7, **konfigurasi 1c**: optimasi upstream aktif (ASM, NIST_OPTIM, window 4) kecuali tabel comb titik basis dimatikan (FIXED_POINT_OPTIM = 0), nonce **hedged** (HMAC-DRBG di-seed entropi \|\| d \|\| digest, bukan RFC 6979 murni), kunci di-zeroize. Latensi 655,5 ms (waktu nyata). Konfigurasi final menunggu keputusan; tulis konfigurasi yang benar-benar dipakai. Jangan klaim mematikan comb sebagai mitigasi side-channel. | W, K |
| 18 | Tabel 3.2 no. 6 | skrip Python menangkap data dan memverifikasi tanda tangan luring | Skrip: `tools/run_bench.py` (kendali run, analisis, ringkasan). Verifikasi tanda tangan luring BELUM ada di protokol run (host hanya mengirim latensi). Pilihan: tambahkan, atau tulis bahwa kebenaran tanda tangan diverifikasi pada tahap pengujian fungsional (pra-eksperimen). | K |

### 2.4 Bab 3 - prosedur dan pengukuran

| No | Lokasi v3 | Tertulis | Kondisi sebenarnya / usulan | Prio |
|---|---|---|---|---|
| 19 | 3.1 dan 3.5.2 ("urutan run diacak") | pengacakan penuh 120 run | Desain blok ABBA: K1 \| K2 K2 \| K1 K1 \| K2 (firmware diganti 3 kali), tiap blok diapit run IDLE, urutan skenario A/B di dalam blok diacak (seed dicatat). Alasan: ganti firmware butuh ST-Link, padahal ST-Link harus dicabut saat mengukur; ABBA menyeimbangkan pergeseran linear terhadap waktu. | W |
| 20 | 3.4 c, 3.5, 3.5.2 A.1 | papan direset melalui NRST tiap run; papan mengirim RUN_START | Tidak ada reset per run. Host tetap menyala; tiap run dimulai oleh PC -> logger (perintah lewat USB) -> logger mengirim `S<detik>` ke host lewat USART1 -> host membalas `>START`, menjalankan jendela 60 s, lalu mengirim `>END` (ringkasan), `>L` (latensi tiap sign), `>DONE`. Attacker dijalankan PC setelah `>START` terlihat. | W |
| 21 | 3.4 c | ~20 tanda tangan per run dikirim untuk verifikasi luring | Belum diterapkan (lihat no. 18). | K |
| 22 | 3.3.1 c.6, 3.4 c | debugger SWD dilepas | Tambahkan: setelah flash, DBGMCU_CR dinolkan (bit DBG_SLEEP yang dinyalakan OpenOCD bertahan sampai power-on reset dan menaikkan arus tidur 0,2-0,7 mA). Terukur: ST-Link terpasang mengurangi arus terbaca 0,03-0,11 mA. | W |
| 23 | 3.4 e, 3.5.1 no. 2 | INA226 ~450 cuplikan/detik, tiap ~2,2 ms | Konfigurasi sesuai (1,1 ms + 1,1 ms, tanpa averaging), laju terukur **495,75 cuplikan/detik** (osilator internal INA226), selang terbesar ~2,1 ms, 0 cuplikan terlambat. | S |
| 24 | 3.5.1 no. 1 | CYCCNT dibandingkan lebar pulsa; pakai timer bila selisih > 1 % | Terbukti CYCCNT tetap berjalan saat WFI: latensi K2 identik (201,25 ms) dengan DBG_SLEEP menyala maupun mati. Tambahkan koreksi jam: HSI host lambat ~0,5 %, sehingga "latensi nyata" = latensi CYCCNT x (T_obs logger / T_obs host). K1: 652,2 ms nominal -> 655,5 ms nyata; K2: 201,25 -> 202,2 ms. | W |
| 25 | 3.5.1 no. 2 e, 3.6 | T_obs = 60 detik | Jendela 60 s menurut jam host = 60,29-60,52 s waktu nyata (jam logger). Semua perhitungan memakai T_obs dari logger (tepi pin status pertama s.d. terakhir). | W |
| 26 | 3.5.1 no. 2 e | E_req = (E - P_idle T_obs) / N_req | Sesuai dengan yang diterapkan (di kode: E_lyn). Tegaskan P_idle diambil dari IDLE di blok yang sama (koreksi pergeseran antar sesi), dan N_req = jumlah sign selesai. | S |
| 27 | 3.5.1 no. 3 | model duty cycle untuk pemeriksaan konsistensi | Terbukti: arus model vs arus terukur selisih <= 0,002 mA. | S |
| 28 | 3.5.2 B.2 | buffer DMA satu frame, sisanya terlewat | Kebijakan sebenarnya: satu flag `g_pending` tanpa antrean. Permintaan yang tiba saat host sibuk digabung menjadi satu sign berikutnya. Masukkan sebagai variabel kontrol (bagian 5). | W |
| 29 | 3.5.2 A.2 | satu permintaan tiap 1 detik selama 60 detik (= 60) | Frame pertama tiba ~1 s setelah jendela dimulai, sehingga terukur 59 permintaan per jendela; TL memakai jumlah terukur sebagai penyebut. | S |
| 30 | 3.5.1 c | multimeter seri, target selisih <= 5 % | Kalibrasi sedang dilakukan (multimeter lab 16 Nov). | K |
| 31 | 3.3.1 c.3 | komputer dijalankan dengan baterai bila memungkinkan | Tulis kondisi sebenarnya saat pengambilan data. | S |
| 32 | 3.6 | aturan data | Tambahkan aturan: run yang gagal pemeriksaan keutuhan otomatis (baris hilang, checksum, cuplikan terlambat, galat I2C, frame rusak, S_cpu host vs logger > 0,5 poin) digugurkan dan diganti, bukan ditambal. Blok 1: 1 dari 22 run gugur (10 baris cuplikan hilang di USB). | W |
| 33 | 3.6.3 | Shapiro-Wilk, Welch, Mann-Whitney, ANOVA, Holm | Tetap dipakai untuk latensi, daya, energi, S_cpu. Catatan: sebaran sangat kecil (CV 0,01-0,07 %), jadi p-value pasti sangat kecil; tekankan ukuran efek dan persen penghematan. TL dan jumlah layanan deterministik (varians ~0): jangan diuji t-test, laporkan sebagai nilai tetap + validasi model. | S |
| 34 | 3.6.2 | gain latensi, P_dyn, S_cpu | Tambahkan gain E_lyn (energi per layanan) dan TL bila diadopsi. | K |

### 2.5 Hal yang sengaja diubah dari rencana (perlu dijelaskan di paper)
- **Pengacakan per run tidak dipakai.** Sempat dibuat dan lulus uji (flash otomatis K1/K2 per run), tetapi butuh ST-Link terpasang yang terbukti memengaruhi arus. Diganti blok ABBA.
- **Data RAM.** Firmware run menyimpan latensi tiap sign di RAM (larik 400 x 4 B = 1,6 KB) untuk dikirim setelah jendela. Bila RAM puncak dilaporkan, sebutkan bahwa larik ini instrumen ukur.

---

## 3. Topik khusus 1: laju beban 1 dan 10 permintaan/detik

### 3.1 Kekhawatiran awal
Pada catatan 12 Nov, laju 1 permintaan/detik dikhawatirkan membuat implementasi software jenuh, karena baseline "heavy" Mbed TLS membutuhkan ~4,9 s per sign. Dengan konfigurasi 1c yang dipakai sekarang (655,5 ms), kekhawatiran itu tidak terjadi.

### 3.2 Besaran kunci (data pilot + tahap D blok 1)

| Arsitektur | t_sign (nyata) | Waktu CPU aktif per permintaan a | Ambang jenuh lambda_jenuh = 1/t_sign |
|---|---|---|---|
| Software (konfigurasi 1c) | 655,5 ms | 0,662 s | **1,53 permintaan/s** |
| DS28E38 | 202,2 ms | 0,072 s | **4,95 permintaan/s** |

a = (1 - S_cpu) / Theta dari skenario A. Pada DS28E38, CPU hanya aktif ~72 ms dari 202 ms tiap sign karena tidur selama IC bekerja.

Utilisasi rho = lambda x t_sign (rho < 1: tidak jenuh, rho > 1: jenuh):

| Skenario | Software | DS28E38 |
|---|---|---|
| A, 1 permintaan/s | rho = 0,66 (tidak jenuh) | rho = 0,20 (tidak jenuh) |
| B, 10 permintaan/s | rho = 6,6 (jenuh) | rho = 2,0 (jenuh) |

### 3.3 Hasil terukur

| Kelompok | Data | S_cpu | Datang/dilayani | TL | Theta (sign/s) | I_avg | E_lyn |
|---|---|---|---|---|---|---|---|
| K1 sw, 1/s | D blok 1, n = 9 | 35,46 % | 59/59 | 100 % | 0,976 | 16,30 mA | 24,22 mJ |
| K3 sw, 10/s | D blok 1, n = 10 | 0,38 % | 604/91 | 15,04 % | 1,506 | 18,69 mA | 23,73 mJ |
| K2 hw, 1/s | pilot, n = 3 | 92,99 % | 59/59 | 100 % | 0,979 | 13,17 mA | 5,39 mJ |
| K4 hw, 10/s | pilot, n = 3 | 64,72 % | 603/297 | 49,25 % | 4,927 | 16,62 mA | 4,68 mJ |

(Data K2/K4 tahap D sedang diambil; angka pilot dan integrasi sangat konsisten dengan blok 1.)

### 3.4 Kesimpulan tentang laju
1. **Skenario normal (1/s) sah untuk kedua arsitektur.** Keduanya melayani 100 % permintaan dan masih sempat tidur (35,5 % dan 93,0 %). Selisih daya di skenario A murni efek arsitektur karena jumlah pekerjaannya sama.
2. **Skenario barrage (10/s) memang membuat keduanya jenuh, dan itu yang diinginkan.** Serangan barrage yang efektif harus melebihi kapasitas korban. Yang penting adalah akibat kejenuhannya:
   - Software: CPU hampir tidak pernah tidur (0,38 %), hanya 15 % permintaan terlayani.
   - DS28E38: jenuh dalam hal layanan, tetapi CPU masih tidur 64,7 % karena menunggu IC dalam WFI, dan melayani 3,3 kali lebih banyak.
   Jadi "jenuh" pada DS28E38 tidak sama dengan "CPU terus terjaga". Ini justru inti temuan race-to-sleep.
3. **Model sederhana menjelaskan semua angka** sehingga hasil bisa digeneralisasi ke laju lain:
   - lambda < lambda_jenuh: TL = 100 %, S_cpu ~ 1 - lambda x a
   - lambda > lambda_jenuh: TL ~ 1/(lambda x t_sign), S_cpu ~ 1 - a/t_sign
   Validasi: TL model 15,24 % vs terukur 15,04 % (K3); 49,44 % vs 49,25 % (K4); S_cpu model K4 64,6 % vs terukur 64,72 %.

### 3.5 Prediksi untuk laju lain (dari model, bukan diukur)

| lambda (permintaan/s) | Software: TL / S_cpu | DS28E38: TL / S_cpu |
|---|---|---|
| 0,5 | 100 % / ~67 % | 100 % / ~96 % |
| 1 (skenario A) | 100 % / ~34-35 % | 100 % / ~93 % |
| 2 | ~76 % / ~0 % | 100 % / ~86 % |
| 3 | ~51 % / ~0 % | 100 % / ~79 % |
| 5 | ~31 % / ~0 % | ~99 % / ~65 % |
| 10 (skenario B) | ~15 % / ~0 % | ~49 % / ~65 % |

Pada laju 2-4 permintaan/s, software sudah jenuh total sementara DS28E38 masih melayani 100 % dan tidur 70-86 %. Ini wilayah perbedaan paling tajam. Opsional (bukan keharusan): satu skenario tambahan, misalnya 3 permintaan/s, sebagai uji sensitivitas atau validasi model. Attacker sekarang hanya punya mode 1/s dan 10/s; laju lain butuh perubahan kecil firmware attacker.

### 3.6 Usulan teks untuk paper (3.3.1 atau 3.5.2)
> Laju 1 permintaan/detik dipilih sebagai kondisi normal karena berada di bawah kapasitas layanan kedua arsitektur (utilisasi 0,66 untuk software dan 0,20 untuk DS28E38), sehingga kedua perlakuan mengerjakan jumlah permintaan yang sama. Laju 10 permintaan/detik dipilih sebagai barrage karena melampaui kapasitas kedua arsitektur (utilisasi 6,6 dan 2,0), sesuai karakter serangan yang berusaha membuat korban terus sibuk. Kapasitas layanan maksimum ditentukan oleh durasi sign, yaitu 1/t_sign.

### 3.7 Syarat
Kesimpulan di atas berlaku untuk konfigurasi K1 1c. Bila konfigurasi final K1 diganti ke profil heavy (~4,9 s per sign), laju 1/s akan membuat software jenuh bahkan di skenario normal (rho ~ 4,9), sehingga laju normal harus diturunkan ke <= 0,15 permintaan/s atau alasannya dibahas ulang.

---

## 4. Topik khusus 2: klaim perlindungan fisik / tampering

### 4.1 Status
Penelitian ini **tidak menguji** ketahanan fisik apa pun (side-channel, fault injection, probing, ekstraksi kunci). Fitur ChipDNA PUF dan die shield hanya bersumber dari datasheet. Paper v3 sudah memberi penafian di 1.1 (akhir subbab) dan 1.3 no. 3, tetapi beberapa bagian lain masih memperlakukannya sebagai klaim, kontribusi, atau bagian kerangka masalah.

### 4.2 Lokasi dan usulan

| Lokasi v3 | Masalah | Usulan |
|---|---|---|
| 1.1 paragraf "Perangkat IoT sering dipasang ... physical tampering" dan "DoSl dan pembongkaran fisik (node tampering)" | Tampering menjadi bagian masalah utama | Boleh tetap sebagai konteks, tetapi masalah yang diteliti dibatasi pada DoSl. Ubah kalimat "Permasalahan tersebut semakin kompleks dengan adanya serangan DoSl dan pembongkaran fisik" menjadi fokus DoSl; tampering disebut sebagai alasan memilih secure element, bukan masalah yang diuji. |
| 1.1 akhir, 1.3 no. 3 | Sudah ada penafian | Pertahankan. |
| 1.5 Manfaat bagi Industri | "solusi keamanan fisik berbiaya rendah ... tanpa mengorbankan ketahanan kriptografi" | Ganti, misalnya: "Memberikan data kuantitatif biaya waktu dan energi pemakaian secure element DS28E38 sebagai akselerator tanda tangan pada mikrokontroler berdaya rendah, sebagai pertimbangan desain bagi perancang sistem." |
| 2.1.3 dan 2.4.1 (teori tamper, ChipDNA, die shield) | Teori bersitasi | Boleh tetap (tinjauan pustaka), tanpa klaim hasil. |
| 2.5.1 akhir ("menghadirkan perlindungan kunci berbasis perangkat keras (merujuk spesifikasi pabrikan)") | Sudah diberi penafian | Pertahankan. |
| 2.5.2 paragraf pembuka ("penguatan keamanan fisik") | Posisi penelitian seolah meliputi keamanan fisik | Ganti menjadi "titik temu antara efisiensi energi dan ketahanan ketersediaan (availability) terhadap serangan Denial of Sleep pada lapisan persepsi". |
| 2.5.2 kontribusi no. 2 ("Perlindungan Kunci terhadap Node Tampering") | Ditulis sebagai kontribusi, padahal tidak diuji | Hapus dari daftar kontribusi, atau ubah judulnya menjadi "Alasan pemilihan DS28E38" dan pindahkan ke 2.4. Kontribusi diganti dengan hal yang benar-benar diukur, misalnya pengukuran tingkat layanan dan ambang jenuh (bagian 5) atau kerangka ukur dua jam (logger kristal + DWT). |
| 2.6 Kerangka Berpikir ("... dan node tampering", "tugas keamanan") | Masuk rantai masalah-solusi-pembuktian | Hapus "node tampering" dan "tugas keamanan"; rantai hanya: beban komputasi ECDSA -> kerentanan DoSl -> offloading -> uji latensi, energi, S_cpu (+ TL). |
| 2.7 Hipotesis | Tidak menyebut tampering | Sudah benar. |
| Bab 5 / kesimpulan nanti | Risiko | Jangan menyimpulkan apa pun tentang ketahanan fisik; boleh disebut sebagai saran penelitian lanjutan. |

### 4.3 Kalimat penafian standar (bisa dipakai berulang)
> Ketahanan fisik DS28E38 (ChipDNA PUF dan die shield) dalam penelitian ini hanya merujuk pada spesifikasi pabrikan (Maxim Integrated, 2020) dan tidak diuji; penelitian ini tidak melakukan analisis saluran samping, injeksi galat, maupun upaya ekstraksi kunci.

### 4.4 Hal terkait yang aman diklaim (karena terukur atau inheren pada desain)
- Pada K2, kunci privat tidak berada di memori STM32F103 (fakta desain, bukan hasil uji ketahanan).
- Melayani permintaan penyerang tidak membocorkan kunci (ECDSA aman terhadap pengumpulan tanda tangan; digest konstan).
- Offloading mengurangi dampak barrage terhadap waktu tidur dan ketersediaan layanan (terukur), tetapi tidak menyaring atau menolak permintaan penyerang. Penyaringan/autentikasi permintaan adalah saran pengembangan.

---

## 5. Metrik tambahan yang diusulkan: tingkat layanan dan permintaan terlewat

Sudah diterapkan di `tools/run_bench.py` (definisi lengkap di `logbook/2026-11-15.md`, bagian "Metrik tingkat layanan").

### 5.1 Rumus
- N_dtg = permintaan valid diterima host; N_lyn = sign selesai; N_ggl = sign gagal; N_rusak = frame rusak/hilang (dilaporkan terpisah)
- Permintaan terlewat: N_lwt = N_dtg - N_lyn - N_ggl
- Tingkat layanan: TL = N_lyn / N_dtg x 100 %; rasio terlewat R_lwt = N_lwt / N_dtg x 100 %
- Throughput: Theta = N_lyn / T_obs (sign/s); kapasitas maksimum Theta_max = 1 / t_sign
- Model validasi: TL_model = min(1, 1/(lambda_eff x t_sign)) x 100 %, lambda_eff = N_dtg / T_obs
- Energi per layanan: E_lyn = (E - P_idle x T_obs) / N_lyn (sama dengan E_req di paper v3)

### 5.2 Definisi yang perlu ditulis eksplisit
- Permintaan yang tiba saat host sibuk tidak diantrekan, tetapi digabung (satu flag pending); ini variabel kontrol.
- "Latensi" = durasi sign (DWT mulai..selesai), bukan waktu tunggu permintaan.
- Penyebut TL = N_dtg terukur (bukan 60 atau 600 nominal).

### 5.3 Apa yang dibuktikan
1. Dimensi kedua dampak barrage: selain waktu tidur turun, ketersediaan layanan juga turun. K3: S_cpu 0,38 %, TL 15 %. K4: S_cpu 64,7 %, TL 49 %.
2. Di skenario normal kedua arsitektur setara dalam layanan (TL = 100 %), sehingga perbandingan energi adil.
3. Permintaan terlewat disebabkan durasi sign, bukan kanal (N_rusak = 0, TL cocok dengan model).
4. Ambang jenuh software ~1,53/s vs DS28E38 ~4,95/s (~3,2 kali).
5. Energi per layanan: DS28E38 ~4,7-5,4 mJ vs software ~23,7-24,9 mJ (~4,6-5,2 kali lebih hemat).

### 5.4 Usulan hipotesis tambahan
> Hardware offloading DS28E38 meningkatkan tingkat layanan pada skenario barrage dibandingkan implementasi software, tanpa mengurangi tingkat layanan pada skenario normal.

### 5.5 Batasan
- TL deterministik (varians ~0): laporkan nilai + validasi model, bukan uji t.
- TL mengukur kapasitas, bukan pertahanan. Klaim: "mengurangi dampak", bukan "mencegah serangan".

---

## 6. Hasil sementara lain yang bisa langsung dipakai di paper

| Besaran | K1 software | K2 DS28E38 |
|---|---|---|
| Latensi sign nominal (CYCCNT @ 48 MHz) | 652,2 ms | 201,25 ms |
| Latensi sign nyata (koreksi jam host) | 655,5 ms | 202,2 ms |
| Sebaran latensi antar-run (sd) | 0,06-0,13 ms | 0,01 ms |
| Arus IDLE (tidur penuh) | 11,6 mA | 12,1 mA |
| Arus saat aktif | 18,6-18,7 mA | 19,0-19,2 mA |
| Arus tidur saat barrage | 11,7 mA | 15,2 mA (DS28E38 bekerja saat CPU tidur) |
| Reduksi latensi (K2 vs K1) | - | 69,2 % |
| Reduksi daya dinamis skenario A (P_dyn) | 23,6-24,3 mW | 5,3 mW (~78 % lebih kecil) |
| Kenaikan S_cpu, skenario A / B | - | +57,5 / +64,3 poin |

Tegangan bus: 5,08-5,17 V (berubah antar sesi; dicatat tiap cuplikan). Galat jam host: -4.543 s.d. -5.465 ppm.

---

## 7. Keputusan yang masih terbuka

| No | Hal | Dampak |
|---|---|---|
| 1 | Konfigurasi K1 final (sekarang 1c) | Bila berubah, data K1/K3 diambil ulang dan bagian 3 dihitung ulang |
| 2 | Nilai shunt final (16 Nov) | Arus/daya absolut naik ~17-20 %, perbandingan tidak berubah; analisis ulang `run_bench.py ringkas --rshunt` |
| 3 | Adopsi metrik TL dan hipotesis tambahan | Perubahan 1.3, 2.6, 2.7, 3.3, 3.6 |
| 4 | Verifikasi tanda tangan luring di protokol run | Tambah fitur, atau ubah teks Tabel 3.2 no. 6 dan 3.4 c |
| 5 | Skenario laju tambahan (mis. 3/s) | Opsional; butuh perubahan firmware attacker |
| 6 | Uji A1 (pemulihan I2C) | Opsional; tidak pernah terpicu |
| 7 | Termometer ruang | Untuk 1.3 no. 5 |

---

## 8. Lokasi data dan alat
- Data mentah per run: `data/runs/<tanggal_jam>_<K1|K2>/NNN_<arsitektur>_<skenario>.csv`, ringkasan `ringkasan.csv`, kondisi sesi `sesi.txt`.
- Tahap D: `data/runs/20261115_173019_K1` (blok 1) dan blok berikutnya.
- Pilot: `data/runs/20261115_163407_K2`, `data/runs/20261115_164758_K1`. Integrasi: `20261115_155057_K1`, `20261115_161521_K2`.
- Skrip: `tools/run_bench.py` (run, plan, ringkas, nodebug), `tools/uji_galat.py`, `tools/a3_clock.py`.
- Firmware: host K1 `src/k1_software/`, K2 `src/k2_hardware/`, protokol `src/host_common/host_link.h`, logger `src/logger/`, attacker `src/attacker/`.
