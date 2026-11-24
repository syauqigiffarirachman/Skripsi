# Hasil B1: verifikasi tanda tangan pada jalur layanan firmware Stop (18 November 2026, malam)

**Kesimpulan: LULUS untuk K1S dan K2S.** Tanda tangan yang keluar dari jalur layanan sebenarnya (bangun dari Stop oleh frame attacker, penerima frame,
pembuatan tanda tangan) sah menurut verifikasi ECDSA P-256 murni Python di komputer (`tools/4_verifikasi/p256.py`, independen dari Mbed TLS dan dari DS28E38).
Ini menutup butir 14 Bagian 9 rekap 18 Nov dan butir B1 di daftar kekurangan.

## Cara
- Build diagnostik `host_k1_stop_diag_auto` / `host_k2_stop_diag_auto` (flag `-DDIAG_SIG`): menyalin r‖s 30 tanda tangan pertama dan mengirimnya sebagai baris `>SIG` sebelum `>DONE`.
  Kode layanan tidak diubah; build final (`host_k1_stop`, `host_k2_stop`) dibangun ulang dari bersih dan identik bit demi bit dengan biner data final
  (sha256 `2ca6e8d0d861…` dan `5b4e017827e3…`, lihat `data/manifes_firmware_stop.md`).
- Attacker mode A (1 permintaan/detik, 60 s) dinyalakan saat `>START`; verifikasi 30 tanda tangan pertama dengan `tools/4_verifikasi/b1_verifikasi.py`.
- Rekaman mentah: `data/dev/b1/20261118_223404_K1S_sig.csv`, `data/dev/b1/20261118_223643_K2S_sig.csv`.

## Hasil
| Arsitektur | Diterima | Sah di komputer | Dirusak (s xor 1) ditolak | Pemeriksaan lain |
|---|---|---|---|---|
| K1S (Mbed TLS, comb mati) | 30 | **30/30** | 30/30 | kunci publik = d × G; 30 nilai r berbeda semua (nonce hedged) |
| K2S (DS28E38 overdrive) | 30 | **30/30** | 30/30 | ROM dari firmware = ROM chip pada rekaman pengembangan (`4d2b423200000094`); kunci publik chip pada kurva; pesan = SHA-256(ROM‖page0‖digest‖0x00‖MANID) |

## Catatan jujur
- Yang diverifikasi ialah build diagnostik, bukan biner data final itu sendiri (biner berbeda karena tambahan penyalinan hasil). Kode jalur layanan identik; pembuktian bahwa biner final menghasilkan tanda tangan sah bersifat tak langsung.
- Pada K2S byte keluaran `ds_sign` berurutan **r‖s**; pada rekaman pengembangan lama (`tools/dev/tahap_stop/ds_od.py`) pengurai memakai s‖r untuk tampilan eksperimen itu. Skrip B1 menguji kedua urutan secara otomatis dan melaporkan yang berlaku; tidak memengaruhi keabsahan.
- Host dalam keadaan setelah flash lewat ST-Link (domain debug aktif); fungsi tanda tangan tidak terpengaruh, tetapi sesi ini bukan sesi ukur arus.
- Cakupan: 30 tanda tangan per arsitektur pada satu papan dan satu chip; bukan uji kriptografis menyeluruh (tidak menguji distribusi nonce selain keunikan r pada K1S).

## Kalimat untuk naskah
*"Keabsahan kriptografis tanda tangan pada jalur layanan firmware varian Stop diverifikasi di komputer dengan implementasi verifikasi ECDSA P-256 independen: 30 dari 30 tanda tangan K1S dan 30 dari 30 tanda tangan K2S sah, dan seluruh tanda tangan yang sengaja dirusak ditolak."*
