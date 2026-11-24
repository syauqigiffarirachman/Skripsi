# Ringkasan hasil varian Stop untuk Bab 4

Disusun 18 November 2026 dari data final varian Stop (*deep sleep*): `data/tahap_stop.txt`, analisis `data/analisis/stop_final/`.
Hanya data dan analisis ini yang sah dipakai untuk skripsi. Semua angka di bawah diambil dari keluaran analisis itu.

## 1. Inti dalam lima kalimat

1. Pada varian **Stop mode** (host tidur dalam), DS28E38 mengalahkan software-based di semua ukuran yang diuji: latensi
   tanda tangan turun **82 %** (654,9 menjadi 116,2 ms), daya dinamis turun **92,6 %** (normal) dan **58,0 %** (barrage),
   dan CPU tidur **98,2 %** versus **35,1 %** pada skenario normal.
2. Pada serangan barrage, software-based nyaris tidak pernah tidur (**0,38 %**) dan hanya melayani **15,0 %** permintaan;
   DS28E38 tetap tidur **84,0 %** dan melayani **85,5 %**. Inilah argumen mitigasi *Denial of Sleep*.
3. Lantai IDLE host sendiri (1,99 mA) sama dengan lantai saat semua papan tersambung, sehingga selisih yang diamati berasal
   dari prosesor dan chip, bukan dari papan lain.
4. **Hati-hati membacanya:** latensi dan tingkat layanan K2 mencerminkan konfigurasi overdrive + polling; kontribusi
   overdrive, polling, dan mode Stop tidak dipisahkan dalam data final.
5. Harga yang dibayar Stop pada barrage K2: **0,1-0,2 % frame UART hilang** (batas toleransi < 0,25 % per run).

## 2. Kondisi pengukuran (untuk Sub-bab 3.x/4.1)

| Butir | Nilai |
|---|---|
| Firmware | K1S (Mbed TLS, ECDSA P-256) dan K2S (DS28E38, overdrive 1-Wire, tunggu chip di Stop) |
| Jam inti | 48 MHz (HSI/2 x 12), bukan 72 MHz; bangun dari Stop selalu di HSI 8 MHz lalu PLL dipulihkan |
| Skenario | A = 1 permintaan/s, B = barrage 10 permintaan/s, IDLE; tiap run 60 s |
| Jumlah | 4 kelompok x 30 run sah + IDLE di awal dan akhir tiap blok = 139 run, 11 gugur |
| Urutan | ABBA: K2S, K1S, K1S, K2S (blok 1-4); urutan A/B diacak dalam blok |
| Instrumen | INA226 (shunt 0,100 ohm nominal, offset -2 LSB), di input 5 V; logger menghitung tidur dari pin PB1 |
| Keadaan papan | setiap ganti firmware: POR penuh dan **tanpa sesi OpenOCD** (lihat bagian 7) |
| Pemeriksaan | MEMENUHI STANDAR: drift IDLE <= 0,004 mA, selisih S_cpu logger vs firmware <= 0,08 poin, frame rusak 0 |

## 3. Hasil utama (rerata, N = 30 per kelompok, sd dalam kurung)

| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi sign (ms) | 654,88 (0,09) | 116,23 (0,06) | 654,91 (0,11) | 116,30 (0,09) |
| Durasi tidur CPU (%) | 35,08 (0,008) | 98,17 (0,005) | 0,377 (0,001) | 83,97 (0,03) |
| Daya dinamis (mW) | 40,10 (0,04) | 2,96 (0,02) | 61,18 (0,05) | 25,70 (0,04) |
| Arus dinamis (mA) | 7,69 | 0,566 | 11,73 | 4,93 |
| Daya rata-rata (mW) | 50,50 | 13,33 | 71,57 | 36,06 |
| Energi per permintaan terlayani, dinamis (mJ) | 41,03 | 3,01 | 40,59 | 3,00 |
| Tingkat layanan (%) | 100 | 100 | 15,04 | 85,50 |
| Throughput (sign/s) | 0,977 | 0,983 | 1,507 | 8,565 |
| Arus saat tidur (mA) | 2,04 | 2,37 | 2,08 | 5,63 |

Lantai IDLE (host sendiri dan semua papan tersambung): **1,99 mA** untuk K1S dan K2S.
Ketidakpastian gabungan nilai mutlak didominasi kalibrasi shunt (u_rel 5 %); pada selisih/persen antar-arsitektur
komponen itu saling menghapus.

## 4. Menjawab rumusan masalah

**RM1 / H1a-c dan RM3 (SW vs HW, positif = DS28E38 lebih baik), uji Welch + Holm, bootstrap 10.000:**

| Skenario | Metrik | Selisih [IK 95 %] | Gain % [IK 95 %] | p (Holm) | Keputusan |
|---|---|---|---|---|---|
| Normal | Latensi | 538,7 ms [538,6; 538,7] | 82,25 [82,25; 82,26] | 4e-178 | tolak H0 |
| Normal | Daya dinamis | 37,14 mW [37,13; 37,16] | 92,62 [92,61; 92,64] | 2e-113 | tolak H0 |
| Normal | Tidur CPU | +63,08 poin [63,08; 63,09] | - | 3e-174 | tolak H0 |
| Barrage | Latensi | 538,6 ms [538,6; 538,7] | 82,24 [82,24; 82,25] | 3e-192 | tolak H0 |
| Barrage | Daya dinamis | 35,48 mW [35,46; 35,50] | 58,00 [57,97; 58,02] | 3e-141 | tolak H0 |
| Barrage | Tidur CPU | +83,59 poin [83,58; 83,60] | - | 7e-103 | tolak H0 |

Distribusi kedua kelompok tidak tumpang tindih pada semua perbandingan (rasio terkecil aturan persen = 20,0 >= 3).
**Interaksi (RM3):** manfaat tidur DS28E38 **lebih besar saat barrage** (DD tidur CPU = +20,51 poin [20,50; 20,52]),
sedangkan manfaat daya dinamis **lebih kecil saat barrage** (DD = -1,66 mW) karena K2S di bawah barrage sibuk melayani
8,6 permintaan/s. Manfaat latensi tidak berbeda antar-skenario (DD = -0,04 ms, IK memuat nol).

**RM2 (pengaruh barrage per arsitektur, deskriptif):**
- Software: tidur CPU 35,08 menjadi 0,38 % (-34,7 poin), tingkat layanan 100 menjadi 15,04 %, daya dinamis +52,5 %.
  Energi dinamis per permintaan tidak berubah (-1,1 %): barrage hanya membuat CPU kerja terus, bukan lebih boros per tanda tangan.
- DS28E38: tidur CPU 98,17 menjadi 83,97 % (-14,2 poin), tingkat layanan 100 menjadi 85,5 %, throughput 0,98 menjadi
  8,56 sign/s. Daya dinamis naik 768 % tetapi dari dasar yang sangat kecil (2,96 menjadi 25,7 mW).

**H4 (tingkat layanan):** K3 15,04 % (model 15,24 %), K4 85,50 % (model 85,83 %); K1 dan K2 100 %. Kecocokan dengan model
utilisasi (rho = 6,56 untuk K3, 1,17 untuk K4) menunjukkan jalur layanan berperilaku seperti yang diprediksi.
Persentase tidur K3/K4 harus dibaca bersama layanan: CPU K4 tampak tidur lama justru karena 14,5 % permintaan terlewat.

## 5. Uji ketahanan dan jalur galat (Sub-bab 4.x "Ketahanan")

- Ketahanan: 139 run final + dua sesi pilot (24 run) tanpa satu pun crash atau macet; ganti firmware otomatis 9 kali.
- Attacker berhenti total, dinyalakan ulang, atau hidup-mati tiap 1,5 s selama jendela: jendela tetap utuh,
  S_cpu logger = host, frame rusak 0, run normal sesudahnya 59/59.
- DS28E38 dicabut SAAT berjalan: 67 sign gagal tercatat dengan benar, tidak ada crash, host tetap tidur; dipasang kembali
  host pulih tanpa reboot (59/59). DS28E38 tidak ada saat boot: host melapor "belum siap" dan `run_bench` menolak
  merekam data palsu.

## 6. Temuan metodologis yang layak ditulis (keterbatasan dan prosedur)

- **Keadaan debug mengubah arus tidur.** Satu sesi OpenOCD menaikkan lantai 1,94 menjadi 2,52 mA dan melonjakkan bangun palsu
  sampai POR penuh. Maka data Stop hanya sah setelah POR penuh tanpa debugger; prosedur ini dipakai di semua blok final.
- **Tanpa landasan NOP, beberapa instruksi pertama sesudah bangun dari Stop kadang salah dieksekusi** pada F103 yang dipakai (bergantung
  tata letak kode). Firmware memakai 24 NOP 16-bit tepat sesudah WFE. Layak dicatat sebagai catatan implementasi.
- **Sinkron bit tanpa mengubah attacker:** UART mati saat Stop; byte pertama 0xA5 dikenali dari pola tepinya dan penerima
  dinyalakan sebelum byte kedua (59/59 pada 1 Hz, 602/602 pada 10 Hz di uji S2).
- **Kalibrasi shunt INA226 dan pembanding multimeter belum final**; ketidakpastian kalibrasi 5 % tetap dilaporkan.
- **Jam 48 MHz, bukan 72 MHz**, sehingga latensi K1 absolut tidak dapat disamakan dengan penelitian lain.
- LED PWR Blue Pill dibiarkan; ia dan board DS9121 (sekitar 0,06-0,09 mA diam) membentuk lantai 1,99 mA yang sama di semua
  arsitektur, jadi tidak memengaruhi selisih.

## 7. Saran susunan Bab 4 dan gambar

1. Hasil varian Stop: tabel bagian 3, tabel RM1/RM3 (bagian 4), RM2 dan H4.
2. Ketahanan dan jalur galat (bagian 5).
3. Keterbatasan dan prosedur (bagian 6).

Gambar sudah dibuat di `data/analisis/stop_final/gambar/` (box plot, interaction plot, drift, Bland-Altman) dan mengikuti
`laporan/arsip/panduan_grafik_bab4_2026-11-16.md`; 

## 8. Yang jangan diklaim

- Jangan menyatakan efek overdrive, polling, dan mode Stop terpisah satu sama lain; data final tidak memisahkannya.
- Jangan menyebut tingkat layanan K4 85,5 % tanpa menyebut latensi 116 ms dan beban 10 permintaan/detik.
- Jangan menyembunyikan frame UART hilang 0,1-0,2 % pada K4.
- Nilai mutlak mA/mW/mJ masih memakai shunt nominal 0,100 ohm (kalibrasi belum final).
