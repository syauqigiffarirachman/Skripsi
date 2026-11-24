# tools/ — skrip PC, disusun menurut aliran data

Urutan folder = urutan alur dari bangku ukur sampai kesimpulan. Peta lengkap: `docs/PETA_ALUR.md`.

| Folder | Pertanyaan yang dijawab | Skrip |
|---|---|---|
| `1_akuisisi/` | Bagaimana data mentah diambil? | `run_bench.py` (kendali run + rumus metrik per run), `flash_stop.py` (flash host yang sedang di Stop) |
| `2_kalibrasi/` | Berapa R shunt efektif dan offset? | `kalibrasi_shunt.py` (ukur + hitung), `kalibrasi_param.py` (nilai final, SATU sumber untuk semua default) |
| `3_analisis/` | Bagaimana CSV mentah menjadi tabel, gambar, kesimpulan? | `analisis_d.py` (statistik utama), `dekomposisi_energi.py`, `analisis_sensitivitas.py`, `proyeksi_baterai.py`, `gambar_bab4_stop.py`, `ekspor_gambar_naskah.py`, `hitung_ulang_satu_run.py` (bukti rumus) |
| `4_verifikasi/` | Apakah data dan kode ini benar? | `cek_biner.py` (firmware identik), `manifes_firmware.py`, `b1_verifikasi.py` + `p256.py` (tanda tangan sah), `diag_long.py`, `uji_galat_stop.py` |
| `demo/` | Demo sidang | `demo_sidang.py` |
| `dev/` | Pengembangan; TIDAK dipakai pada data akhir | `tahap_stop/` (S0..S2, overdrive, `k1s_run.py`), `tahap_d/` (uji PC tahap D, emulator), `monitor.py`, `provenance.py` |

## Cara kerja impor
Skrip saling mengimpor dengan nama modul biasa (`import run_bench`). Tiap skrip memuat `tools/_jalur.py`, yang
menambahkan semua subfolder ke `sys.path` dan menyediakan `ROOT` (akar repo). Jadi skrip bisa dijalankan dari
folder mana pun: `python tools/3_analisis/analisis_d.py --help`.

## Uji
`python -m unittest discover -s tests` (tanpa perangkat keras; 53 uji).
