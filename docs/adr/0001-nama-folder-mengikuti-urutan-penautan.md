# Nama folder firmware dipilih agar biner tetap identik

Data akhir 18 Nov sah hanya bila `firmware.bin` logger dan host identik dengan yang di-flash saat sesi. PlatformIO menautkan
objek `src/` menurut urutan path, jadi memindah atau menamai ulang folder bisa menggeser tata letak Flash walau kodenya sama.
Kami memilih memindah firmware secara fisik (final/dev, logger satu folder) dengan nama yang menjaga urutan lama
(`logger/f103/` sebelum `logger/modul/`, karena "f" < "m"), dan membuktikannya dengan `tools/4_verifikasi/cek_biner.py`
(sha256 sama dengan `tools/4_verifikasi/biner_acuan.json` dan `data/manifes_firmware_stop.md`).

Ditolak: menaruh kode bersama logger di `lib/` (objek pustaka ditautkan di grup terpisah, biner berubah), `lib_archive = no`
(mengubah penautan `Wire`), dan nama `logger/common/` (tertaut sebelum `f103/`). Jangan "merapikan" nama-nama ini tanpa
menjalankan gerbang biner.
