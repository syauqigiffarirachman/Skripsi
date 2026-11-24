# Prosedur B1 (keabsahan tanda tangan firmware final) dan B2 (sensitivitas konfigurasi software)

Kode dan skrip sudah siap (18 Nov 2026). Yang tersisa hanya langkah fisik: flash, POR, jalankan. Tidak ada yang mengubah firmware data final;
build diagnostik memakai flag `-DDIAG_SIG` dan env terpisah, jadi biner `host_k1_stop` / `host_k2_stop` tetap sama (lihat `data/manifes_firmware_stop.md`).

Aturan bangku (dari logbook 18 Nov, bagian 8.1): setiap ganti firmware lewat ST-Link, lakukan **POR penuh** (cabut USB-C, ST-Link, kabel sinyal + GND, tunggu 10 s,
pasang GND dulu, USB-C terakhir) dan jangan membuka OpenOCD di antara run. Jalur logger → host (PA10) masih putus, jadi dipakai env `*_auto`.
Port: logger COM18 (sesuaikan), attacker COM24.

## B1. Verifikasi tanda tangan firmware final (±10 menit)

Yang dibuktikan: tanda tangan yang keluar dari jalur layanan K1S dan K2S yang sebenarnya (Stop, bangun, penerima frame) sah menurut verifikasi ECDSA P-256 di komputer.

1. Flash K1S diagnostik (host tidur Stop, gunakan skrip penangkap):
   `python tools/1_akuisisi/flash_stop.py .pio/build/host_k1_stop_diag_auto/firmware.elf` → POR penuh.
2. `python tools/4_verifikasi/b1_verifikasi.py K1S --logger COM18 --attacker COM24`
   (menunggu satu siklus host ±30 s, menyalakan attacker mode A saat `>START`, 59 permintaan, lalu verifikasi 30 tanda tangan pertama).
   Yang diharapkan: `30/30 sah`, `30/30 dirusak ditolak`, `semua r berbeda`, `HASIL: LULUS`.
3. Ulang untuk K2S: flash `.pio/build/host_k2_stop_diag_auto/firmware.elf`, POR penuh, lalu `python tools/4_verifikasi/b1_verifikasi.py K2S --logger COM18 --attacker COM24`.
   Pemeriksaan tambahan: ROM dari firmware = ROM chip pada rekaman pengembangan.
4. Rekaman mentah ada di `data/dev/b1/`. Jika `LULUS` untuk kedua arsitektur, butir B1 selesai; catat di logbook dan Bab 4 (menutup butir 14 Bagian 9 rekap 18 Nov).
   Jika `GAGAL`, jangan ubah apa pun; salin keluaran dan laporkan.

Bila verifikasi diulang dari rekaman yang sama: `python tools/4_verifikasi/b1_verifikasi.py K1S --analyse data/dev/b1/<berkas>.csv`.

## B2. Sensitivitas konfigurasi software, comb aktif (±15 menit)

Yang diukur: latensi dan energi per tanda tangan K1S bila tabel comb Mbed TLS **aktif** (`MBEDTLS_ECP_FIXED_POINT_OPTIM=1`), sebagai batasan atas klaim "82 % lebih cepat".
Build sudah diuji: Flash 49 064 B (74,9 %), RAM statis 7 148 B (34,9 %), muat. Pemakaian heap runtime untuk tabel comb belum diuji di papan (jika ragu, perhatikan `gagal` di `>END`).

1. Flash: `python tools/1_akuisisi/flash_stop.py .pio/build/host_k1_stop_comb_auto/firmware.elf` → POR penuh.
2. Tiga run skenario A dan tiga run skenario B sudah cukup (ini dev, bukan data final):
   `python tools/dev/tahap_stop/k1s_run.py A --catatan comb` (ulangi 3×), lalu `python tools/dev/tahap_stop/k1s_run.py B --catatan comb` (3×). Keluaran masuk `data/dev/k1s/`.
   Catatan: tanda tangan pertama sesudah boot lebih lambat (tabel comb dihitung dan di-cache); rerata latensi sudah memuatnya, tetapi lat_min menunjukkan tanda tangan "panas".
3. Yang dilaporkan: latensi (rerata dan min), `E_lyn_mJ`, S_cpu, TL pada barrage. Bandingkan dengan K1 final (654,9 ms; 41,0 mJ; TL 15,0 %).
   Perkiraan sebelumnya (catatan 13 Nov): ±278,6 ms, sehingga keunggulan waktu DS28E38 turun dari 82 % ke ±58 %.
4. Setelah itu minta saya memperbarui bagian 5 butir 9 dan Bab 4 dengan angka terukur.

## Setelah B1/B2

Pulihkan host ke firmware final hanya bila akan mengambil data final lagi; data skripsi yang ada tidak terpengaruh.
Jika host tidur Stop dan SWD tak merespons, pemulihan: BOOT0=1 + POR + `python tools/dev/tahap_stop/pulihkan_host.py`, lalu BOOT0=0 + POR.
