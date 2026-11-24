# Daftar kekurangan data, analisis, dan pembahasan (18 November 2026)

Disusun dari seluruh logbook, laporan rekap, dan hasil analisis data *deep sleep* (`data/analisis/stop_final/`). Isinya butir-butir yang
masih belum jelas atau kurang untuk memenuhi data, analisis, dan pembahasan di Bab 4 dan Bab 5. Kolom **Status** diperbarui tiap kali ada kemajuan.
Kolom **Siapa**: Anda = butuh tangan atau alat fisik; Saya = pekerjaan kode/analisis yang dapat langsung dikerjakan.

Status: `belum` | `siap` (alat/prosedur sudah ada, tinggal dikerjakan) | `proses` | `selesai`.

## A. Wajib: data dan instrumen

| No | Butir | Mengapa penting | Yang dibutuhkan | Siapa | Status |
|---|---|---|---|---|---|
| A1 | **Kalibrasi shunt INA226 terhadap multimeter** | Rekap 1 mencatat arus terbaca ±17–20 % lebih kecil (shunt efektif diduga 0,080–0,083 Ω, bukan 0,100 Ω), dengan acuan yang lemah (ohmmeter murah, multimeter membaca tegangan ±5 % rendah). Ketidakpastian kalibrasi 5 % di analisis hanya asumsi. Seluruh nilai mutlak mA, mW, mJ di Bab 4 bergantung pada ini, termasuk klaim kalibrasi di Bab 3 propo | Multimeter yang terverifikasi, pembacaan arus tunak di ±2, ±4, ±7, ±13, ±15, ±18 mA, resistor 1 %; hitung R_efektif dan u_kal | Anda (alat); analisis ulang Saya | `siap`: `tools/2_kalibrasi/kalibrasi_shunt.py`, `docs/prosedur/prosedur_kalibrasi_bagian_A.md` |
| A2 | **Offset nol INA226 pada arus rendah** | Analisis memakai −2,0 satuan; pengukuran 15 Nov −2 lalu −3,4 satuan, 18 Nov ±−3,2. Selisih ±1,2 satuan = ±0,03 mA ≈ 1,5 % dari lantai 1,99 mA. Tidak memengaruhi metrik dinamis, memengaruhi nilai mutlak | Pembacaan beban nol (IN− dilepas), awal dan akhir sesi | Anda | `siap` (prosedur A2) |
| A3 | **Pull-up 1-Wire dan komponen DS9121** | Catatan bangku menandai 4,7 kΩ sebagai dugaan; masuk spesifikasi di Bab 3 | Ukur resistansi PA15–3V3 (papan DS9121 dicabut dan terpasang), catat jumper VCC/CEXT | Anda | `siap` (prosedur A3) |
| A4 | **Osiloskop**: bukti bentuk gelombang | Propo menyebut osiloskop opsional; tidak ada data tangkapannya | 4 tangkapan (PB1+PA15, bit overdrive, PA3 frame, rail 3V3 saat bangun) atau hapus dari Tabel 3.1 | Anda | `siap` (prosedur A4) |

## B. Wajib: keabsahan klaim analisis

| No | Butir | Mengapa penting | Cara | Siapa | Status |
|---|---|---|---|---|---|
| B1 | **Keabsahan tanda tangan pada firmware final (Stop)** | Verifikasi di komputer hanya pada eksperimen overdrive (269/269) dan pengembangan K1 awal. K1S/K2S final tidak pernah mengirim tanda tangan ke komputer | Build diagnostik yang mengirim r‖s untuk 30 tanda tangan; verifikasi di komputer dengan `k1_check`/`k2_check` | Saya (kode, skrip), Anda (flash, POR) | `selesai` 18 Nov malam: 30/30 sah K1S dan K2S, lihat `laporan/2_hasil_dan_pembahasan/bukti_pendukung_B1_B2_C5_2026-11-18.md` |
| B2 | **Uji sensitivitas konfigurasi software** | "82 % lebih cepat" bergantung pada Mbed TLS tabel comb dimatikan (655 ms). Catatan 13 Nov: comb aktif ±278,6 ms. Penguji pasti menanyakan | Satu blok kecil K1S dengan comb aktif (jika muat RAM/Flash) atau kutip pengukuran pengembangan sebagai batasan | Saya dan Anda | `selesai` (data dev, N kecil) 18 Nov malam: comb aktif 278,6 ms; keunggulan DS28E38 58 % waktu, 82 % energi; `laporan/2_hasil_dan_pembahasan/bukti_pendukung_B1_B2_C5_2026-11-18.md` |
| B3 | **Sensitivitas toleransi frame hilang** | Kriteria K4 ditetapkan setelah melihat hasil; perlu bukti kesimpulan tidak berubah | Ulang analisis dengan ke-41 run K4 (termasuk 11 gugur) dan tabel perbandingan | Saya | `selesai`: `tools/3_analisis/analisis_sensitivitas.py`, `laporan/2_hasil_dan_pembahasan/bahan_pembahasan_bab4_2026-11-18.md` bagian 1; kesimpulan tak berubah |
| B4 | **Rilis firmware yang membangkitkan data** | Sesi satu-arsitektur tidak mencatat sha256 firmware | Manifes sha256 K1S, K2S, logger, attacker, plus hash commit | Saya | `selesai`: `data/manifes_firmware_stop.md`; bukti tak langsung (tak ada sha yang dicatat saat sesi), keterbatasan tertulis |
| B5 | **Nilai p tertulis 0** | CSV menampilkan p = 0 (di bawah presisi mesin) | Laporkan sebagai "< 1e-100" dengan statistik uji dan derajat bebas | Saya | `selesai`: `sensitivitas/p_nilai.csv`; p = 0 hanya ada di data pengembangan, bukan `stop_final` |

## C. Sebaiknya: bahan pembahasan

| No | Butir | Mengapa penting | Cara | Siapa | Status |
|---|---|---|---|---|---|
| C1 | **Dekomposisi energi per tanda tangan** (CPU aktif, IC menghitung, tidur) | Pembahasan "mengapa DS28E38 hemat" perlu angka; jejak arus menunjukkan plato ±5–6 mA selama IC menghitung | Hitung dari jejak arus mentah: durasi dan arus tiap fase, tabel mJ per fase | Saya | `selesai`: `tools/3_analisis/dekomposisi_energi.py`, Gambar 4.12; mJ menunggu kalibrasi |
| C2 | **Perbandingan dengan literatur** | Noseda dkk. (waktu 27–50 %, energi 51–87 %), Peradeniya (336 ms), Istiyanto | Angka tepat dari paper diambil Anda; tabel disusun Saya; referensi tambahan dapat dicari | Anda | `belum` |
| C3 | **Latensi kecepatan biasa vs overdrive** | Menjelaskan K2: 192,5 ms (biasa + tunggu tetap) vs 115,6 ms (overdrive + polling); data di `data/dev/ds_od` belum jadi tabel naskah | Tabel pembahasan | Saya | `selesai`: `bahan_pembahasan` bagian 4 |
| C4 | **Waktu respons saat antrean** | Latensi di sini hanya lama tanda tangan, bukan waktu tunggu | Nyatakan sebagai batasan, atau model antrean sederhana | Saya | `proses`: batasan + bound 2 x latensi ditulis (bagian 7); Anda putuskan perlu pengukuran atau tidak |
| C5 | **Penyebab sisa frame hilang (±0,17 %)** | Mekanisme utama terdiagnosis; sisa masih dugaan | Tulis sebagai dugaan atau uji dengan diagnostik tambahan | Anda memutuskan | `selesai` sebagian: mekanisme dikarakterisasi (frame hilang = tanda tangan 210 ms akibat kebetulan frame-alarm RTC), penyebab tingkat register belum; `laporan/2_hasil_dan_pembahasan/bukti_pendukung_B1_B2_C5_2026-11-18.md` |
| C6 | **Ancaman terhadap validitas** | Satu papan/chip, logger klon, LED daya, HSI, konfigurasi 1c, penyerang periodik, shunt | Satu subbab terstruktur (bahan di rekap 18 Nov Bagian 9) | Saya bantu | `selesai`: 17 butir di `bahan_pembahasan` bagian 5 (draf, tinjau) |
| C7 | **Implikasi praktis untuk baterai** | Topiknya penguras baterai, tetapi tak ada angka umur baterai | Proyeksi energi per jam dari `E_mJ`/`P_avg` dengan catatan lantai LED 1,99 mA | Saya (tabel); Anda memutuskan | `selesai`: `tools/3_analisis/proyeksi_baterai.py`; angka menunggu kalibrasi; asumsi baterai ilustratif perlu Anda setujui |

## D. Opsional

| No | Butir | Catatan |
|---|---|---|
| D1 | Papan atau chip DS28E38 kedua | Satu-satunya cara menyatakan variasi antar-unit; tanpa ini tetap batasan |
| D2 | Uji jalur galat K1S | Jalur Stop identik; nilai tambah kecil |
| D3 | Pencatat suhu ruang | Drift suhu belum diukur |
| D4 | Uji galat kristal LSE dan tegangan turun | Berisiko menyentuh kristal; tidak disarankan |
| D5 | Kelompok pemisah efek overdrive dari efek Stop | Tidak perlu: data skripsi hanya varian Stop |

## E. Pekerjaan dokumen

| No | Butir | Status |
|---|---|---|
| E1 | Menempel revisi Bab 3, 2, 1 ke naskah (daftar "hal yang harus dipastikan" di laporan Bab 3 Butir 33) | `belum` |
| E2 | Menulis Bab 4 dan Bab 5 (kerangka di panduan grafik dan laporan rekap) | `proses`: peta bab di `laporan/2_hasil_dan_pembahasan/README.md`; teks bab belum ditulis, angka mutlak menunggu kalibrasi |
| E3 | Isi tahun/revisi dokumen ST (RM0008, datasheet STM32F103x8/B, AN2867) dan halaman datasheet DS28E38 yang menyatakan overdrive | `proses`: `laporan/5_status_dan_referensi/referensi_dokumen_E3_2026-11-18.md` (overdrive terverifikasi di tabel perintah ROM; revisi/tahun ST perlu Anda lihat dari PDF) |
| E4 | Gambar ulang Gambar 2.1, 3.1, 3.2, 3.4 di naskah | `belum` |
| E5 | Perbarui `docs/referensi/pinout_arsitektur_pengujian.md` (lantai 1,98 vs 2,565 mA sudah terpecahkan: debug power) | `selesai` |

## Urutan kerja yang disarankan

1. Anda: A1 dan A2 (kalibrasi). Semua nilai mutlak menunggu ini.
2. Saya bersamaan: B3, B4, B5, C1, C3, E5 (hanya butuh data yang ada).
3. Saya lalu Anda: B1 (tanda tangan final), lalu B2 (sensitivitas comb) bila waktu cukup.
4. Setelah kalibrasi: analisis dan gambar dijalankan ulang, lalu Bab 4 ditulis dengan angka final.

Risiko terbesar bagi sidang: A1 (kalibrasi), B2 (konfigurasi software), B1 (keabsahan tanda tangan firmware final).
