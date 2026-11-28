# Gambar Bab 5 dengan skenario C (K5, K6) — 28 Nov 2026

Dibuat oleh `tools/3_analisis/gambar_skenario_c.py` dari `data/analisis/kapasitas/runs.csv` (hanya run sah).

**Tegangan acuan:** daya dan energi K5/K6 dinyatakan pada tegangan 18 Nov, dihitung ulang
pada tegangan K1-K4 (5,218 V Software, 5,217 V DS28E38). Besaran lain (arus, tidur CPU, latensi, tingkat layanan) tidak
bergantung pada tegangan. Angka ini berbeda dari `ringkasan.md` (yang memakai tegangan asli 5,13 V): P_dyn K5 72,75 mW
(bukan 71,28), K6 30,89 (bukan 30,14); E_lyn K5 49,12 mJ (bukan 48,13), K6 3,61 (bukan 3,52).

| Gambar | Berkas | Status | Isi |
|---|---|---|---|
| 5.1 | Gambar_5.1_frame_hilang_K4_K6.png | diperluas | frame hilang K4 dan K6 |
| 5.7 | Gambar_5.7_durasi_tidur_cpu.png | diganti | enam kelompok |
| 5.8 | Gambar_5.8_daya_dinamis.png | diganti | enam kelompok (tegangan 18 Nov) |
| 5.9 | Gambar_5.9_energi_per_permintaan.png | diganti | enam kelompok (tegangan 18 Nov) |
| 5.11 | Gambar_5.11_tingkat_layanan_vs_model.png | diganti | enam kelompok |
| 5.12 | Gambar_5.12_throughput_vs_batas.png | diganti | enam kelompok + garis kapasitas |
| 5.13 | Gambar_5.13_durasi_tidur_terhadap_utilisasi.png | BARU | tidur CPU lawan utilisasi |
| 5.14 | Gambar_5.14_arus_terhadap_laju_datang.png | BARU | arus lawan laju datang |

Tidak berubah: 5.2, 5.3, 5.4, 5.5, 5.6, dan 5.10 (interaksi 2x2; skenario C tidak masuk faktorial).
Semua nilai memakai metrik jendela 60 s (sama dengan A/B). Keadaan tunak (tanpa jeda awal) dibahas di teks, bukan di gambar.
Ulangi: `.venv-analisis/Scripts/python tools/3_analisis/gambar_skenario_c.py`
