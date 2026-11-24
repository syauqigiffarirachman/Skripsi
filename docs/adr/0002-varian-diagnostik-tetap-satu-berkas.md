# Varian diagnostik tetap sebagai #ifdef di berkas final

K1S dan K2S punya varian untuk diagnosis (`DIAG_SIG`, `DIAG_LONG`, `K1S_AUTO`, `K2S_AUTO`, `DBG_RX`). Kami membiarkannya sebagai
`#ifdef` di `src/host/final/*/main.cpp` dan `bersama/*.h`, dan hanya mengelompokkan envnya di `platformio.ini` (KELOMPOK 2
PENDUKUNG), bukan memisahkannya ke berkas lain.

Alasannya: varian itu harus menjalankan jalur layanan yang SAMA dengan firmware final agar menjadi bukti sah (verifikasi tanda
tangan B1, diagnosis frame hilang). Menyalin `main.cpp` ke folder dev akan menciptakan dua versi yang bisa menyimpang diam-diam.
Akibatnya folder `final/` memuat kode yang mati pada env final; kode itu tidak masuk biner (flag tidak diberikan).
`tools/4_verifikasi/cek_biner.py` menjaga hal ini. Ditolak: duplikasi berkas, dan menghapus varian (bukti B1/B2 tak bisa diulang).
