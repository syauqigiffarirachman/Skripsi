# Bahan pembahasan Bab 4 dan Bab 5 (18 November 2026)

Disusun dari data final varian Stop (`data/analisis/stop_final/`). Seluruh nilai mutlak (mA, mW, mJ) memakai shunt 0,100 Ω
dan offset −2,0 LSB, sehingga **harus dihitung ulang setelah kalibrasi A1/A2**; persentase antar-arsitektur tidak terpengaruh.
Skrip pembangkit tiap tabel disebut di kepala bagiannya. Butir daftar kekurangan: B3, B4, B5, C1, C3, C6, C7.

---

## 1. Sensitivitas toleransi frame hilang K4 (B3)

Skrip `tools/3_analisis/analisis_sensitivitas.py`; berkas `data/analisis/stop_final/sensitivitas/`.

Kriteria K4 (frame UART hilang < 0,25 % dari permintaan yang tiba) ditetapkan 18 Nov setelah melihat hasil, atas keputusan peneliti.
Agar kesimpulan tidak bergantung pada ambang itu, K4 dianalisis ulang pada enam ambang dan dengan seluruh 41 run.

| Ambang (% hilang) | Run K4 lolos | Latensi (ms) | Daya dinamis (mW) | Tidur CPU (%) | Energi/layanan (mJ) | Tingkat layanan (%) |
|---|---|---|---|---|---|---|
| 0 (tanpa kehilangan) | 17 | 116,24 | 25,70 | 83,96 | 3,0000 | 85,46 |
| **0,25 (dipakai)** | **30** | 116,30 | 25,69 | 83,97 | 3,0000 | 85,50 |
| 0,35 | 36 | 116,34 | 25,69 | 83,97 | 3,0000 | 85,51 |
| 1,0 (semua) | 41 | 116,38 | 25,68 | 83,97 | 3,0002 | 85,53 |

Rentang rerata antar-ambang: latensi 0,14 ms (0,12 %), daya dinamis 0,025 mW (0,10 %), tingkat layanan 0,07 poin. K3 vs K4 (Welch):

| Metrik | Gain % (30 run sah) | Gain % (41 run) | p (30) | p (41) | Kesimpulan |
|---|---|---|---|---|---|
| Latensi | 82,24 | 82,23 | < 1e-100 | < 1e-100 | sama |
| Daya dinamis | 58,00 | 58,02 | < 1e-100 | < 1e-100 | sama |
| Tidur CPU | arah sama | arah sama | < 1e-100 | < 1e-100 | sama |
| Energi per layanan | 92,61 | 92,61 | 7,8e-92 | 2,0e-91 | sama |
| Tingkat layanan | K4 > K3 | K4 > K3 | 1,7e-88 | < 1e-100 | sama |

**Kalimat untuk naskah:** pemilihan ambang toleransi frame hilang tidak memengaruhi kesimpulan; dari 17 hingga 41 run K4 yang dipakai,
semua metrik utama bergeser < 0,15 % dan semua keputusan uji tetap sama. Penyebab gugurnya 11 run hanya frame UART yang hilang
(2–4 dari 598–600), bukan galat tanda tangan atau pin salah.

## 2. Pelaporan nilai p (B5)

Pada `data/analisis/stop_final/` tidak ada nilai p = 0; p_welch dan p_holm tertulis mis. 8,2e−179. Nilai "0" yang pernah terlihat berasal dari
`anova.csv` data pengembangan (bukan data skripsi). Untuk naskah, `sensitivitas/p_nilai.csv` memuat t, derajat bebas (Welch–Satterthwaite),
dan log10 p; nilai di bawah 1e−100 ditulis sebagai **"p < 10⁻¹⁰⁰"**. Contoh format: *t(55,4) = 21 194, p < 10⁻¹⁰⁰* (latensi K3 vs K4).
Catatan jujur: p sekecil itu hanya mencerminkan simpangan baku sangat kecil dan 30 run; ukuran efek (gain %, IK 95 %) lebih informatif.
Sebaiknya laporkan p dengan satu kalimat bahwa semua perbandingan utama bermakna pada α = 0,05 setelah koreksi Holm.

## 3. Dekomposisi energi per tanda tangan (C1)

Skrip `tools/3_analisis/dekomposisi_energi.py`; berkas `dekomposisi_energi.md/.csv` (Normal, 30 run K1 dan 30 run K2). Fase menurut pin status:
aktif (CPU terjaga), menunggu (CPU tidur di dalam episode; pada K2 = IC DS28E38 menghitung), tidur di luar episode.
Jumlah fase cocok dengan E_lyn utama (selisih +0,00 %).

| Kelompok | Fase | Durasi (ms/sign) | Arus rerata (mA) | E dinamis (mJ/sign) | Porsi |
|---|---|---|---|---|---|
| K1 | CPU aktif (hitung software) | 664,1 | 13,82 | 40,98 | 99,9 % |
| K1 | tidur di luar episode | 359,0 | 2,02 | 0,05 | 0,1 % |
| K2 | CPU aktif (kirim perintah + baca hasil) | 18,6 | 13,01 | 1,07 | 35,5 % |
| K2 | **IC menghitung (CPU tidur)** | 100,9 | 5,56 | **1,88** | **62,4 %** |
| K2 | tidur di luar episode | 897,5 | 2,00 | 0,06 | 2,0 % |

Bahan pembahasan "mengapa DS28E38 hemat":
- Pada software, 99,9 % energi dinamis adalah CPU aktif menghitung selama 664 ms pada 13,8 mA.
- Pada DS28E38 CPU aktif hanya 18,6 ms (dua jendela: ±6,9 ms mengirim perintah, ±12,1 ms membaca hasil); sisa ±101 ms CPU tidur dan arus total 5,56 mA,
  yaitu ±3,6 mA di atas lantai papan, mewakili tarikan IC saat menghitung (dan rail 1-Wire).
- Energi terbesar K2 (62 %) justru dari IC yang menghitung. Pengurangan energi ±13,6 kali (41,0 → 3,0 mJ) berasal dari
  CPU aktif 36 kali lebih singkat (664 → 18,6 ms) dan arus selama menunggu 2,5 kali lebih rendah dari arus CPU.
- Catatan nilai mutlak: bagian arus IC tidak terpisah dari tarikan LDO/papan pada rail 3V3; angka 3,6 mA adalah batas atas untuk IC saja.

## 4. Latensi kecepatan biasa vs overdrive (C3)

Sumber: eksperimen pengembangan 17 Nov (`data/dev/ds_od/`, `tools/dev/tahap_stop/ds_od.py --analyse`), tanda tangan diverifikasi di komputer (kunci publik pada
kurva; tanda tangan yang sengaja dirusak ditolak). Kolom ms; "kirim" = menulis perintah dan challenge, "sibuk" = IC menghitung,
"baca" = membaca r‖s.

| Mode | n | Kirim | Sibuk (IC) | Baca | **Total** | sd | Valid di PC |
|---|---|---|---|---|---|---|---|
| Biasa + tunggu tetap 130 ms | 15 | 23,77 | 129,37 | 39,40 | **192,53** | 0,21 | 15/15 |
| Biasa + polling | 15 | 23,77 | 107,17 | 38,69 | 169,63 | 0,26 | 15/15 |
| Overdrive + tunggu tetap 130 ms | 10 | 3,39 | 129,60 | 5,70 | 138,69 | 0,001 | 10/10 |
| **Overdrive + polling (dipakai K2S)** | 16 | 3,39 | 106,66 | 5,60 | **115,66** | 0,26 | 16/16 |
| Overdrive + polling (uji beban) | 269 | 3,39 | 106,57 | 5,60 | 115,56 | 0,22 | 269/269 |

Dekomposisi selisih (192,5 → 115,6 ms, total −76,9 ms):
- **Overdrive:** menyusutkan kirim+baca dari 63,2 ke 9,1 ms; efek total hampir sama dengan atau tanpa polling (−53,8 ms dengan tunggu tetap, −54,0 ms dengan polling).
- **Polling:** menggantikan tunggu tetap 130 ms dengan waktu IC sebenarnya (±107 ms), −22,9 ms (biasa) atau −23,0 ms (overdrive).
- Dua efek hampir aditif; overdrive 70 %, polling 30 % dari penghematan.
- Latensi episode pin pada data final (K2S) 119,5 ms dan latensi DWT 116,2 ms; selisih ±3,3 ms ialah waktu bangun dari Stop + fase baca (bukan galat pengukuran; sudah dibahas di rekap 18 Nov butir 8 Bagian 9).
- Konsekuensi: **latensi K2 pada data final mencerminkan konfigurasi terbaik DS28E38 (overdrive + polling)**. Tanpa keduanya latensi ±192 ms (keunggulan atas software 655 ms turun dari 82 % ke ±71 %). Ini perlu dinyatakan sebagai batasan.

## 5. Ancaman terhadap validitas (C6)

Kerangka: validitas internal, konstruk, eksternal, kesimpulan statistik.

**Internal**
1. *Satu papan host dan satu chip DS28E38.* Variasi antar-unit tidak teramati; semua nilai merupakan ukuran satu unit.
2. *Logger klon dan INA226 modul.* Tegangan/arus melalui INA226 dengan shunt belum dikalibrasi terhadap acuan presisi; u_kal 5 % adalah asumsi. Jam logger = acuan waktu, galat jam host −64…−31 ppm dikoreksi.
3. *Lantai arus 1,99 mA didominasi LED daya papan.* Nilai mutlak daya rata-rata mencerminkan papan ini, bukan chip telanjang; selisih dinamis tetap sah.
4. *Keadaan debug.* Satu sesi OpenOCD menaikkan lantai ±0,58 mA; dicegah dengan POR penuh tiap ganti blok. Keadaan bersih diverifikasi dari IDLE pembuka/penutup tiap blok (drift ≤ 0,004 mA).
5. *Urutan ABBA 4 blok.* Mengurangi drift waktu, tetapi bukan pengacakan penuh; leave-one-block-out tersedia (`leave_one_block_out.csv`).
6. *HSI 8 MHz setelah bangun.* Jam sistem 48 MHz (bukan 72 MHz) dan HSI memengaruhi latensi software absolut; tidak dapat dibandingkan langsung dengan penelitian lain.
7. *Landasan NOP.* Obat empiris untuk salah-eksekusi pasca-Stop; akar penyebab silikon tidak diselidiki.

**Konstruk**
8. *"Latensi" = lama pembuatan satu tanda tangan,* bukan waktu tunggu antrean (tidak ada antrean pada model ini).
9. *Konfigurasi software 1c* (Mbed TLS dengan tabel comb dimatikan) adalah baseline; dengan comb aktif latensi software ±278,6 ms (catatan 13 Nov) sehingga keunggulan waktu DS28E38 jauh lebih kecil (±58 %); bila energi software diperkirakan sebanding dengan waktunya (±17 mJ per tanda tangan), keunggulan energi turun dari ±93 % ke ±83 % (perkiraan awal). **Terukur 18 Nov malam (B2, data pengembangan N=5):** latensi software 278,6 ms, energi 16,9 mJ per tanda tangan, sehingga keunggulan DS28E38 58,3 % (waktu) dan 82,2 % (energi); lihat `docs/hasil_B2_sensitivitas_comb_2026-11-18.md`.
10. *K2S memakai overdrive + polling.* Kontribusi overdrive, polling, dan mode Stop tidak dipisah pada data final (lihat bagian 4).
11. *Daya "dinamis"* dihitung terhadap IDLE varian Stop (10,4 mW) sehingga hanya sah untuk membandingkan dua arsitektur di data ini.

**Eksternal**
12. *Penyerang periodik* (10 permintaan/detik, 60 s) adalah model Denial of Sleep sederhana; penyerang adaptif tidak diuji.
13. *Satu algoritme dan satu kurva* (ECDSA P-256). Hasil tidak otomatis berlaku untuk perangkat lain.
14. *Perangkat dengan baterai nyata, suhu berbeda, atau papan tanpa LED* tidak diuji.

**Kesimpulan statistik**
15. *Simpangan baku sangat kecil* menghasilkan p yang sangat kecil; yang bermakna ialah ukuran efek. IK bootstrap 10 000 resampel, seed 20261116.
16. *Toleransi frame hilang K4* (< 0,25 %) ditetapkan setelah melihat hasil; sensitivitas pada bagian 1 menunjukkan kesimpulan tidak berubah.
17. *Keabsahan tanda tangan pada firmware final belum diverifikasi per run* (butir B1; dikerjakan lewat build diagnostik terpisah).

## 6. Implikasi baterai (C7)

Skrip `tools/3_analisis/proyeksi_baterai.py`; berkas `proyeksi_baterai.md/.csv`. Ilustrasi: 1000 mAh × 3,7 V (13,32 kJ), regulator diabaikan.

| Kelompok | E/layanan (mJ) | Tanda tangan per baterai (ribu) | Arus rerata (mA) | Umur dgn lantai (jam) | Umur tanpa lantai (jam) | Tingkat layanan |
|---|---|---|---|---|---|---|
| K1 Software, normal | 41,03 | 325 | 9,68 | 103 | 130 | 100 % |
| K2 DS28E38, normal | 3,01 | 4 426 | 2,55 | 392 | 1 767 | 100 % |
| K3 Software, barrage | 40,59 | 328 | 13,72 | 73 | 85 | 15,0 % |
| K4 DS28E38, barrage | 3,00 | 4 440 | 6,91 | 145 | 203 | 85,5 % |

Pembacaan: pada beban normal 1 permintaan/detik, DS28E38 memperpanjang umur dari ±103 jam menjadi ±392 jam (3,8 kali) dengan lantai papan 1,99 mA,
dan ±13,6 kali tanda tangan per baterai (4,4 juta vs 325 ribu). Serangan memperpendek umur 29 % pada software (103 → 73 jam) sambil hanya melayani
15 % permintaan; pada DS28E38 memperpendek 63 % (392 → 145 jam) tetapi melayani 85,5 %. Perbandingan umur K3 vs K4 tidak setara pekerjaan.
Ini proyeksi dari pengukuran, bukan umur baterai terukur; lantai chip sebenarnya (1–20 µA) jauh di bawah lantai papan sehingga umur "tanpa lantai" lebih dekat ke perangkat terintegrasi.

---

## 7. Waktu respons dan antrean (C4)

Yang diukur ialah **latensi tanda tangan** (lama satu operasi sign), bukan waktu tunggu dalam antrean. Menurut kode (`g_pending` satu bendera di
`src/host/deepsleep/stop_common.h`), host menampung paling banyak **satu** permintaan yang tiba selagi sign berjalan; permintaan berikutnya yang tiba
sebelum yang tertampung dilayani akan hilang (dihitung sebagai tidak terlayani, selaras dengan model tingkat layanan `TL = min(1, 1/(λ·t_sign))`).
Konsekuensi (dari struktur kode, belum diukur langsung per permintaan):
- Normal (λ = 1/s, latensi 116 ms atau 655 ms < 1 s): tak ada permintaan yang menunggu; waktu respons = latensi.
- Barrage: permintaan yang tertampung menunggu paling lama satu latensi, sehingga waktu respons terburuk ≤ 2 × latensi
  (K4: ≈ 233 ms; K3: ≈ 1,31 s). Tingkat layanan tetap menjadi ukuran utama dampak serangan.
Kalimat batasan untuk naskah: *"Penelitian ini tidak mengukur waktu tunggu antrean; latensi yang dilaporkan ialah lama pembuatan tanda tangan. Dengan penampung
satu permintaan, waktu respons terburuk dibatasi dua kali latensi."* Bila ingin diukur: tandai waktu tiba tiap frame di firmware (butir tindak lanjut, perlu keputusan Anda).

## 8. Penyebab sisa frame hilang ±0,17 % (C5): terdiagnosis sebagian

Lihat `docs/diagnosis_frame_hilang_K2S_2026-11-18.md`. Ringkas: tiap run dengan frame hilang memuat tanda tangan berdurasi tepat ±210 ms (sd 0,3 ms) bukan 116 ms; jumlahnya sama dengan
frame hilang pada pengukuran langsung (8 dan 8). Mekanisme yang konsisten: frame tiba hampir bersamaan dengan alarm RTC penanda selesainya perhitungan chip, alarm tak terdeteksi,
host tidur satu interval (≈96 ms) tambahan. Penyebab tingkat register belum dibuktikan (hipotesis: baca-ubah-tulis `RTC->CRL` yang menghapus ALRF). Data final tetap sah; ditulis sebagai batasan.
