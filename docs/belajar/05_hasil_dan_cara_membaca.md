# 05. Hasil Tahap D dan Cara Membacanya

Sumber: `logbook/2026-11-15.md` (tahap D, langkah 11), `laporan/arsip/laporan_progres_revisi_paper_2026-11-15.md`,
`logbook/2026-11-16.md`. Semua angka memakai shunt nominal 0,100 ohm kecuali dinyatakan lain; kalibrasi shunt
(~0,080-0,083 ohm) menaikkan arus absolut sekitar 17-20 % tetapi **tidak** mengubah perbandingan.

## Tabel utama (30 run sah per kelompok, rerata dan sd)

| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi sign nyata (ms) | 655,4 (0,07) | 202,2 (0,03) | 655,5 (0,09) | 202,2 (0,03) |
| S_cpu (%) | 35,46 | 92,99 | 0,38 | 64,72 |
| I_dyn (mA) | 4,689 | 1,019 | 7,093 | 4,465 |
| P_dyn (mW) | 23,25 | 5,30 | 35,11 | 23,05 |
| E_lyn (mJ) | 23,83 | 5,42 | 23,32 | 4,68 |
| TL (%) | 100 | 100 | 15,04 | 49,25 |
| Throughput (sign/s) | 0,976 | 0,979 | 1,506 | 4,927 |

## Cara membaca tiap baris

**Latensi.** Software 655 ms vs hardware 202 ms: **-69,2 %**. Sama di kedua skenario, sebab durasi satu sign tidak
bergantung pada beban. Selisih kecil antar skenario (0,1 ms) signifikan secara statistik hanya karena sd sangat kecil,
bukan bermakna fisik.

**S_cpu.** Normal: software tidur 35 %, hardware 93 %. CPU software sibuk 65 % hanya untuk 1 sign/detik.
Barrage: software praktis tidak pernah tidur (0,38 %), hardware masih tidur 64,7 %.
Ini bukti paling jelas **efek Denial of Sleep** pada software dan "race-to-sleep" pada hardware.

**I_dyn dan P_dyn.** Arus/daya tambahan di atas dasar papan.
- Normal: -78 % (arus), -77 % (daya). Kedua arsitektur mengerjakan jumlah pekerjaan sama, jadi selisih murni efek arsitektur.
- Barrage: hanya -37 % / -34 %. Jangan kaget: K4 **melayani 3,3x lebih banyak** sign, jadi lebih banyak kerja,
  lebih banyak arus. Karena itu metrik yang adil untuk barrage adalah energi per layanan.

**E_lyn.** Energi per tanda tangan selesai: ~5 mJ (HW) vs ~24 mJ (SW), 4,6-5,2x lebih hemat, bahkan di barrage.
Ini metrik paling tahan terhadap perbedaan jumlah layanan.

**TL.** Normal keduanya 100 %. Barrage: SW 15 %, HW 49 %. Model utilisasi cocok sampai 0,2 poin.

## Interaksi (apakah manfaat HW lebih besar saat barrage?)

Selisih-dari-selisih (DD), positif berarti manfaat HW lebih besar saat barrage:
- **S_cpu: +6,80 poin [6,795; 6,809]**. Hipotesis interaksi terdukung.
- P_dyn -5,88 mW, I_dyn -1,04 mA: manfaat daya dinamis *lebih kecil* saat barrage (karena K4 melayani lebih banyak).
- E_lyn: IK memuat nol. Tidak ada bukti interaksi.
- Latensi: 0,08 ms, praktis sama.

## Mengapa p-value tidak jadi bintang utama

Sebaran antar run sangat kecil (CV 0,01-0,07 %), jadi hampir semua uji menolak H0 dengan p < 1e-10. Itu benar tetapi
tidak informatif. Yang disajikan: ukuran efek (persen gain, poin S_cpu), IK 95 % bootstrap, dan ketidakpastian gabungan
(U_gab ~5-6 % untuk metrik energi, didominasi 5 % kalibrasi asumsi). Aturan pelaporan persen: rerata SW / U >= 3.

## Tegangan USB dan metrik daya

Rasio pergeseran antar blok / sd dalam blok 6,8-12 untuk P_dyn K1/K3, sebab tegangan bus turun saat beban naik
dan berbeda antar blok (5,02-5,22 V). Metrik arus (I_dyn, Q_lyn) stabil. Usulan: jadikan **I_dyn dan Q_lyn
metrik dinamis utama**, dan P_dyn pelengkap.

## Model kejenuhan: prediksi untuk laju lain (dari model, bukan diukur)

| lambda (permintaan/s) | Software TL / S_cpu | DS28E38 TL / S_cpu |
|---|---|---|
| 0,5 | 100 % / ~67 % | 100 % / ~96 % |
| 1 (A) | 100 % / ~34-35 % | 100 % / ~93 % |
| 2 | ~76 % / ~0 % | 100 % / ~86 % |
| 5 | ~31 % / ~0 % | ~99 % / ~65 % |
| 10 (B) | ~15 % / ~0 % | ~49 % / ~65 % |

Wilayah 2-4 permintaan/s adalah tempat perbedaannya paling tajam: software sudah jenuh total, DS28E38 masih
melayani 100 %. Itu usulan skenario tambahan opsional.

## Klaim yang BOLEH dan TIDAK BOLEH ditulis

| Boleh | Tidak boleh |
|---|---|
| Offloading mengurangi dampak barrage terhadap tidur dan ketersediaan (terukur) | "Offloading mencegah serangan" (host tidak menyaring/autentikasi permintaan) |
| Pada K2 kunci privat tidak ada di memori STM32 (fakta desain) | "DS28E38 tahan tampering/side-channel" (tidak diuji; hanya rujukan datasheet) |
| Melayani penyerang tidak membocorkan kunci (digest konstan, ECDSA aman terhadap pengumpulan tanda tangan) | Menyimpulkan apa pun tentang ketahanan fisik |
| Perbandingan berlaku untuk konfigurasi K1 1c (comb mati) | Menggeneralisasi "HW 3x lebih cepat" ke semua pustaka software |
| "Latensi" = durasi sign | "Latensi" = waktu tunggu permintaan |

## Keterbatasan yang harus disebut jujur

1. Satu papan host, satu sesi data per blok, suhu ruang tidak diukur (tidak ada termometer).
2. HSI host tidak sempurna; dikoreksi dengan jam logger.
3. Shunt efektif ~0,08 ohm; nilai absolut mutlak masih bergantung kalibrasi.
4. Entropi K1 bukan TRNG, dan pengumpulan entropi di luar jendela ukur.
5. Lantai tidur Stop didominasi LED PWR (~2 mA), nilai mutlak adalah tingkat papan.
6. Tanda tangan tidak diverifikasi di tiap run tahap D (verifikasi dilakukan saat uji fungsional `k1_check.py`/`k2_check.py`).
