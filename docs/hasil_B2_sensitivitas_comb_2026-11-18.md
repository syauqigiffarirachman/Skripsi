# Hasil B2: sensitivitas konfigurasi software, tabel comb Mbed TLS aktif (18 November 2026, malam)

Build `host_k1_stop_comb_auto` (kode K1S identik; hanya `MBEDTLS_ECP_FIXED_POINT_OPTIM = 1`, `include/mbedtls_k1_comb_config.h`; sha256 biner `968cf7681a64…`).
Diukur di bangku dengan `tools/dev/tahap_stop/k1s_run.py` (1 IDLE, 3 run skenario A, 2 run skenario B; semua run lolos pemeriksaan standar; `gagal` = 0 pada semua run,
jadi tabel comb muat di heap). Berkas mentah: `data/dev/k1s/*_comb.csv`. **Data pengembangan, bukan data final**: host dalam keadaan setelah flash ST-Link
(lantai IDLE 2,557 mA, bukan 1,99 mA), N kecil, shunt nominal 0,100 Ω. Energi dihitung relatif terhadap IDLE sesi yang sama sehingga lantai tak banyak memengaruhi.

## Hasil
| Metrik | K1 baseline (comb mati, data final, N=30) | K1S comb aktif (N=3 A, 2 B) | K2 DS28E38 (data final) |
|---|---|---|---|
| Latensi sign (ms) | 654,88 | **278,56** (sd 0,02) | 116,23 |
| Energi dinamis per layanan (mJ), Normal | 41,03 | **16,9** | 3,01 |
| Tidur CPU Normal (%) | 35,08 | 71,70 | 98,17 |
| Tingkat layanan Barrage (%) | 15,04 | **34,94** | 85,50 (K4) |
| Energi per layanan Barrage (mJ) | 40,59 | 16,7 | 3,00 |

Latensi 278,6 ms cocok dengan pengukuran pengembangan 13 Nov (278,6 ms), sehingga kedua pengukuran saling menguatkan.

## Keunggulan DS28E38 terhadap software dengan comb aktif
| Metrik | vs comb mati | vs comb aktif |
|---|---|---|
| Latensi | 82,2 % lebih cepat (5,6×) | **58,3 %** lebih cepat (2,4×) |
| Energi per tanda tangan | 92,7 % lebih hemat (13,6×) | **82,2 %** lebih hemat (5,6×) |
| Tingkat layanan Barrage | 85,5 % vs 15,0 % | 85,5 % vs 34,9 % |

**Tafsiran:** arah dan kesimpulan tidak berubah, tetapi besar keunggulan turun. Latensi 58 % (bukan 82 %) dan energi 82 % (bukan 93 %) ialah batas bawah yang adil
bila penguji menyoal konfigurasi software. Keunggulan atas konfigurasi software terbaik tetap bermakna; angka 82 % harus selalu ditulis bersama label "konfigurasi 1c, tabel comb dimatikan".

## Batasan
- Satu papan, 5 run (dev); belum 30 run, tanpa urutan ABBA, tanpa POR bersih; nilai energi untuk perbandingan kasar (±beberapa persen), bukan untuk tabel utama.
- Tabel comb dihitung sekali lalu di-cache (diduga pada tanda tangan pertama sejak boot, belum diverifikasi di kode). Run terukur ialah siklus host lanjutan (k1s_run mendahuluinya dengan satu rekaman sinkron), tidak ada tanda tangan lambat di dalamnya (lat_min 277,3 ms, lat_max 279,9 ms).
- Menyertakan comb aktif dalam data skripsi final akan membutuhkan 4 × 30 run ulang; disarankan menyatakannya sebagai batasan dan saran.

## Kalimat untuk naskah
*"Pada konfigurasi Mbed TLS dengan tabel comb dimatikan (konfigurasi penelitian), DS28E38 lebih cepat 82,2 %. Uji sensitivitas dengan tabel comb aktif (latensi software 278,6 ms; N kecil, data pengembangan) menurunkan keunggulan menjadi 58,3 % pada latensi dan 82,2 % pada energi per tanda tangan; arah kesimpulan tetap sama."*
