# Panduan demo sidang (hari-H)

Status: 19 Nov 2026, sudah diuji di bangku (logger COM18, attacker COM24) dengan peniru ketikan a/x.
Hasil uji ada di bagian "Hasil uji bangku" pada dokumen rancangan.
Rancangan: `docs/superpowers/specs/2026-11-19-demo-sidang-design.md`. Skrip: `tools/demo_sidang.py`.

## Sebelum penguji datang
1. **ELF host harus ada** di `.pio/build/host_k1_stop/firmware.elf` dan `.pio/build/host_k2_stop/firmware.elf`.
   Per 19 Nov keduanya sudah dibangun dan biner IDENTIK dengan acuan data skripsi
   (`python tools/cek_biner.py banding host_k1_stop host_k2_stop`, ~3 menit).
2. Pasang: USB-C host (hanya daya), ST-Link host (`$`) terpasang (perlu untuk flash), logger USB, attacker CH340,
   kabel logger PA9 -> host PA10 dan PB1 host -> PA0 logger tersambung.
3. Catat nomor COM logger dan attacker (berubah bila kabel dicabut-pasang). Buka serial monitor attacker
   (115200) pada COM attacker. Jangan buka serial monitor pada COM logger (dipakai skrip).
4. `python tools/demo_sidang.py --logger COMxx` -> `ping` -> `mode k1` (flash ke K1S) -> `ping`
   (harus `>ID,K1S,...,1`).

## Selama demo
- Ketik `sesi`. Tiap run: Enter -> banner **HOST SIAP** -> hitung mundur -> banner **BISA MULAI MENYERANG**:
  ketik `a` (normal, 1/s) atau `b` (barrage, 10/s) di serial monitor attacker -> jendela 60 s, arus tampil
  per detik -> hasil. Setelah tiap run ketik `x` di attacker.
- Urutan: K1S.A, K1S.B, lalu skrip mem-flash host ke K2S otomatis (tekan Enter), K2S.A, K2S.B.
- Tabel akhir muncul otomatis; hasil mentah di `data/demo/<tanggal-jam>/`.
- Terukur: empat run 306 detik (5,1 menit) tanpa jeda penjelasan; flash K1S->K2S 12,5 detik; flash persiapan
  ke K1S 13,9 detik. Dengan penjelasan ke penguji perkirakan 10-12 menit.
- Baris tabel bertanda **PERIKSA** = run selesai tetapi ada catatan (mis. frame hilang di atas toleransi). Itu
  temuan yang sudah tercatat di skripsi (K2S barrage kehilangan ~1 frame/run); ulangi dengan `run B` bila perlu.

## Yang disampaikan ke penguji
- Prosedur demo sedikit berbeda dari pengambilan data: attacker dimulai SEBELUM jendela 60 s dibuka
  (pengambilan data: sesudah `>START`). Jendela, firmware, dan rumus analisis sama.
- ST-Link terpasang selama demo sehingga arus diam terbaca sedikit lebih rendah (0,03-0,11 mA); angka dapat
  sedikit berbeda dari tabel skripsi.

## Bila ada masalah
- "host tidak menjawab": periksa kabel logger PA9 -> host PA10 dan daya host; `ping` lagi.
- "host berisi ..., diminta ...": ketik `mode k1` atau `mode k2`.
- Run gagal (timeout/NOSTART): sesi lanjut ke run berikutnya; ulangi dengan `run A` / `run B`.
- Host terkunci (SWD mati): BOOT0=1 + power-on reset + `python tools/pulihkan_host.py`, lalu BOOT0=0 + POR.
- Cadangan bila frame rusak muncul pada alur manual: tutup serial monitor attacker, jalankan dengan
  `--auto-attacker COMyy` (attacker dimulai skrip tepat setelah `>START`, persis prosedur data).
- Pra-uji 20 run K2S.B dengan alur manual: frame rusak (`rx_bad`) 0 di semua run; alur manual layak.
