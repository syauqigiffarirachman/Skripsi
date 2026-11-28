# Hasil dan pembahasan (Bab 4 dan Bab 5)

Folder ini berisi semua bahan untuk Bab 4 (Hasil dan Pembahasan) dan Bab 5 (Kesimpulan). Baca urutannya:

| No | Berkas | Untuk apa |
|---|---|---|
| 1 | `laporan/2_hasil_dan_pembahasan/ringkasan_bab4_varian_stop.md` | Inti hasil dalam lima kalimat dan semua angka utama |
| 2 | `laporan/2_hasil_dan_pembahasan/bahan_pembahasan_bab4_2026-11-18.md` | Bahan pembahasan per tabel dan skrip pembangkitnya |
| 3 | `laporan/2_hasil_dan_pembahasan/panduan_grafik_bab4_2026-11-18.md` | Lembar kerja tiap gambar (data, sumbu, keterangan) |
| 4 | `bukti_pendukung_B1_B2_C5_2026-11-18.md` | Bukti tambahan: tanda tangan sah, sensitivitas konfigurasi, diagnosis frame hilang |
| 5 | `bukti_pendukung_dan_pembahasan_skenario_c_2026-11-28.md` | Skenario C (K5, K6): bukti, pembahasan, dan daftar gambar yang diubah/ditambah |

Bagian di bawah memetakan tiap sub-bab naskah ke berkas sumbernya (dulu `laporan/2_hasil_dan_pembahasan/README.md`).

---

## Kerangka Bab 4 dan Bab 5 (peta sumber, 18 November 2026)

Dibuat untuk menempatkan semua bahan yang sudah ada ke dalam urutan naskah. Isi tiap butir sudah ada di berkas sumbernya; dokumen ini hanya peta.
**Angka mutlak (mA, mW, mJ) menunggu kalibrasi A1/A2**; persentase, latensi, S_cpu, tingkat layanan, dan p-value sudah final. Hanya data *deep sleep*; jangan menyebut atau membandingkan dengan tahap D.

### Bab 4. Hasil dan Pembahasan

| Subbab | Isi | Gambar/tabel | Sumber | Status angka |
|---|---|---|---|---|
| 4.1 Kondisi dan keabsahan pengukuran | Run per sesi, IDLE pembuka/penutup (drift ≤ 0,004 mA), kesesuaian dua pengukur S_cpu, galat jam, run gugur dan sebabnya | Gambar 4.9, 4.11; Tabel C, D, E | `stop_final/pemeriksaan.md`, panduan grafik | final |
| 4.2 Gambaran jejak arus | Satu run contoh tiap kelompok, fase aktif/tidur | Gambar 4.1, 4.1b | `gambar_bab4/` | nilai mA: kalibrasi |
| 4.3 RM1: latensi dan energi (Normal) | Latensi 654,9 vs 116,2 ms (82,2 %); E per layanan 41,0 vs 3,0 mJ (92,7 %); daya dinamis | Gambar 4.2, 4.4, 4.5; Tabel A | `ringkasan.md` bagian 1, 3 | latensi final; mJ: kalibrasi |
| 4.4 Dekomposisi energi | CPU aktif 664 vs 18,6 ms; IC menghitung 62 % energi K2 | Gambar 4.12 (baru) | `dekomposisi_energi.md`, `bahan_pembahasan` bagian 3 | mJ: kalibrasi |
| 4.5 Peran kecepatan overdrive dan polling | 192,5 → 115,6 ms; overdrive 70 %, polling 30 % | tabel C3 | `bahan_pembahasan` bagian 4 | final (eksperimen pengembangan) |
| 4.6 RM2: pengaruh serangan | S_cpu 0,38 % vs 84,0 %; tingkat layanan 15,0 % vs 85,5 %; throughput | Gambar 4.3, 4.7, 4.8, 4.10; Tabel B | `ringkasan.md` bagian 4, 6 | final |
| 4.7 RM3: interaksi | DD tidur CPU +20,5 poin; daya dinamis dan energi per pekerjaan | Gambar 4.6 | `ringkasan.md` bagian 5 | persentase final |
| 4.8 Uji hipotesis | H1a-c, interaksi, H4; Holm; p < 10⁻¹⁰⁰ | Tabel A | `sensitivitas/p_nilai.csv`, `bahan_pembahasan` bagian 2 | final |
| 4.9 Ketahanan hasil | Sensitivitas toleransi frame hilang (17–41 run), leave-one-block-out, jalur galat 7/7 | tabel B3 | `bahan_pembahasan` bagian 1; `leave_one_block_out.csv` | final |
| 4.10 Pembahasan | Mengapa DS28E38 hemat; implikasi baterai; perbandingan literatur | tabel C7; C2 menunggu | `bahan_pembahasan` bagian 3, 6 | C2 butuh angka paper (Anda) |
| 4.11 Ancaman terhadap validitas | 17 butir dikelompokkan | tabel teks | `bahan_pembahasan` bagian 5 | final |

Hal yang jangan diklaim: lihat `laporan/2_hasil_dan_pembahasan/ringkasan_bab4_varian_stop.md` bagian 8. Tambahan: jangan menyatakan tanda tangan final terverifikasi sebelum B1 dijalankan.

### Bab 5. Kesimpulan dan Saran

Bahan lengkap ada di rekap 18 Nov Bagian 10 (`laporan/1_rekap_penelitian/rekap_logbook_17-18_nov_2026-11-18.md`): 6 butir kesimpulan, 8 butir saran. Penyesuaian yang disarankan:
- Kesimpulan 1: tambahkan "pada konfigurasi software dengan tabel comb dimatikan; hasil B2 menunjukkan batas keunggulan pada konfigurasi lain" setelah B2 dijalankan.
- Saran 1 (memisahkan overdrive, polling, Stop) sebagian sudah dijawab oleh eksperimen pengembangan (bagian 4 bahan pembahasan); tulis sebagai "dijawab pada kondisi tanpa Stop".
- Tambahkan saran: pengukuran waktu tunggu antrean (C4), penerima frame terpisah (C5), kalibrasi lantai papan tanpa LED (C7).

### Pekerjaan yang tersisa untuk naskah
1. Tempel revisi Bab 1-3 (E1); isi tahun/revisi dokumen ST dan halaman datasheet overdrive (E3); gambar ulang Gambar 2.1, 3.1, 3.2, 3.4 (E4).
2. Setelah kalibrasi: jalankan `analisis_d.py --stop --rshunt <> --offset <> --u-rel <>`, `dekomposisi_energi.py --rshunt <> --offset <>`, `gambar_bab4_stop.py --shunt <>`, `proyeksi_baterai.py`, lalu perbarui angka mutlak di bab.
