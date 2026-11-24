# Laporan: bahan paper skripsi

Folder ini berisi semua yang dipakai untuk **menulis dan merevisi naskah** (proposal dan skripsi). Nomor folder mengikuti
urutan kerja: pahami apa yang terjadi, lihat hasilnya, terapkan ke naskah, siapkan sidang, cek yang masih kurang.
Data yang sah untuk skripsi hanya data **deep sleep (Stop mode)**; jangan menyebut atau membandingkan dengan tahap D di naskah.

| Folder | Pertanyaan yang dijawab | Berkas utama |
|---|---|---|
| [`1_rekap_penelitian/`](1_rekap_penelitian) | Apa yang terjadi selama penelitian? | `rekap_logbook_10-16_nov` (tahap D), `rekap_logbook_17-18_nov` (beralih ke Stop) |
| [`2_hasil_dan_pembahasan/`](2_hasil_dan_pembahasan/README.md) | Apa hasilnya dan bagaimana membahasnya (Bab 4 dan 5)? | `README.md` (peta sub-bab ke sumber), `ringkasan_bab4_varian_stop`, `bahan_pembahasan_bab4`, `panduan_grafik_bab4`, `bukti_pendukung_B1_B2_C5` |
| [`3_revisi_naskah/`](3_revisi_naskah) | Teks apa yang harus diganti di naskah? | `revisi_bab1/2/3_propo_fix5` (proposal), `perubahan_skripsi_v3` (20 Nov), **`perubahan_skripsi_v4_kalibrasi` (21 Nov, terbaru)** |
| [`4_sidang/`](4_sidang) | Apa yang akan ditanyakan penguji? | `antisipasi_pertanyaan_penguji` |
| [`5_status_dan_referensi/`](5_status_dan_referensi) | Apa yang masih kurang, dan rujukan apa yang perlu dicek? | `daftar_kekurangan_data_analisis`, `referensi_dokumen_E3` |
| [`arsip/`](arsip) | Dokumen lama yang sudah digantikan | lihat tabel di bawah |

## Mana yang berlaku
- Menulis ulang naskah: mulai dari `3_revisi_naskah/perubahan_skripsi_v4_kalibrasi_2026-11-21.md` (angka mutlak setelah kalibrasi 21 Nov).
  `perubahan_skripsi_v3` adalah daftar perubahan untuk naskah v3 (20 Nov; naskah v4 dibuat sesudahnya, jadi cek dulu butir mana yang sudah ada di naskah Anda);
  `revisi_bab1-3_propo_fix5` untuk proposal (data deep sleep).
- Angka mutlak (mA, mW, mJ) di dokumen bertanggal 18 Nov memakai shunt nominal 0,100 Ohm; nilai terkalibrasi (0,0821 Ohm, offset -2,83)
  ada di `data/analisis/stop_final/` dan `data/kalibrasi/kalibrasi_final_2026-11-21.md`.

## Arsip (digantikan)
| Berkas | Digantikan oleh | Alasan |
|---|---|---|
| `arsip/laporan_progres_revisi_paper_2026-11-15.md` | `3_revisi_naskah/` (propo fix5, v3, v4) | berdasar paper SEMPRO v3 dan data tahap D |
| `arsip/revisi_bab3_v4_2026-11-16.md` | `3_revisi_naskah/revisi_bab3_propo_fix5_2026-11-18.md` | berdasar data tahap D |
| `arsip/panduan_grafik_bab4_2026-11-16.md` | `2_hasil_dan_pembahasan/panduan_grafik_bab4_2026-11-18.md` | dokumen baru menyatakan menggantikannya |
