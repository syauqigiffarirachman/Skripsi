# Peta dokumen

Dokumen proyek dibagi menurut **siapa yang membacanya**. Mulai dari baris yang sesuai dengan tujuan Anda.

| Saya ingin... | Buka | Isi |
|---|---|---|
| **Belajar** memahami penelitian dan kodenya dari nol | [`docs/belajar/`](belajar/00_peta_belajar.md) | Seri bacaan 01-08 (konsep, bangku uji, kode, hasil, kuis, alur data), catatan belajar harian di `belajar/harian/` |
| **Menulis / merevisi skripsi** (bahan paper) | [`laporan/`](../laporan/README.md) | Rekap penelitian, hasil dan pembahasan Bab 4/5, daftar perubahan naskah, antisipasi penguji, status kekurangan |
| **Menjalankan bangku ukur** atau memahami perangkat keras | [`docs/referensi/`](referensi/pinout_arsitektur_pengujian.md) dan [`docs/prosedur/`](prosedur/prosedur_kalibrasi_bagian_A.md) | Pinout, rencana deep sleep, prosedur kalibrasi/B1/B2, panduan demo sidang |
| **Mengikuti alur kode** dari firmware sampai kesimpulan | [`docs/PETA_ALUR.md`](PETA_ALUR.md) | Kode mana final, mana pengembangan, urutan baca per board dan skrip PC |
| **Tahu mengapa** sebuah keputusan desain diambil | [`docs/adr/`](adr/0001-nama-folder-mengikuti-urutan-penautan.md) dan [`GLOSSARY.md`](../GLOSSARY.md) | Keputusan sulit dibalik; istilah proyek |
| **Melihat riwayat kerja** hari demi hari | [`logbook/`](../logbook/README.md) | Jurnal harian 10-18 Nov dan 24 Nov |

## Isi `docs/`
```
docs/
  PETA_ALUR.md           alur kode final (mulai di sini untuk memahami kode)
  belajar/               UNTUK BELAJAR: seri 00-08 + harian/
  referensi/             pinout_arsitektur_pengujian, rencana_deep_sleep, PEMETAAN_PATH (path lama -> baru)
  prosedur/              langkah kerja di bangku: kalibrasi, B1/B2, demo sidang
  review/                laporan review kode 18 Nov
  adr/                   keputusan desain
  superpowers/           rancangan dan rencana kerja (demo sidang, pemisahan final/dev)
```

## Aturan penamaan
- Berkas bertanggal memakai akhiran `_YYYY-MM-DD` (tanggal penyusunan); versi terbaru yang berlaku dijelaskan di `README.md` tiap folder.
- Dokumen yang sudah digantikan dipindahkan ke `laporan/arsip/`, bukan dihapus.
- Path lama ke baru: [`docs/referensi/PEMETAAN_PATH.md`](referensi/PEMETAAN_PATH.md) (kode dan dokumen).
