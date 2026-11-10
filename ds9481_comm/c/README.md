# Versi C — tanda tangan digital DS28E38 lewat USB (DS9481P-300)

Migrasi dari Python ke C (Windows, MinGW-w64/gcc). Fungsi sama dengan `../sign.py`
dan **format berkas JSON-nya sama**, jadi hasil C bisa diverifikasi Python dan sebaliknya.
Tanpa library eksternal: SHA-256 dan ECDSA P-256 ditulis sendiri.

## Build & pakai
```
mingw32-make            # menghasilkan sign.exe   (MSYS2: export PATH=/c/msys64/ucrt64/bin:$PATH)
mingw32-make test       # tes offline: SHA-256, CRC, ECDSA (vektor dari tanda tangan nyata chip)

sign.exe info                                  # ID chip + kunci publik
sign.exe sign "pesan"                          # -> tanda_tangan.json
sign.exe sign --file dokumen.pdf --page 2      # tandatangani file, ikat ke halaman 2
sign.exe verify tanda_tangan.json "pesan"      # cek TANPA chip
sign.exe verify tanda_tangan.json --pub X_hex,Y_hex   # pakai kunci publik tepercaya
```
Opsi: `--port COM13` (tanpa ini COM1..COM256 dipindai), `--debug` (hex dump TX/RX),
`--page 0..5`, `--anon`, `--out nama.json`. Kode keluar: 0 sukses, 3 tanda tangan tidak valid, 4 pesan tidak sama.

## Peta Python → C
| Python (`ds9481/`) | C (`src/`) |
|---|---|
| `transport.py` | `serial_win32.c/.h` (Win32 COM) |
| `onewire.py` | `onewire.c/.h` |
| `ds28e38.py` | `ds28e38.c/.h` |
| `crc.py` | `crc.c/.h` |
| `ecdsa_verify.py` + `hashlib` | `p256.c/.h` (Montgomery, Jacobian) + `sha256.c/.h` |
| `sign.py` | `sign.c` |

## Yang sengaja tidak dimigrasi
DS2476/I2C (fokus ke DS28E38) dan simulator uji Python. `set_page_protection` dan
`generate_key_pair` ada di `ds28e38.c` tetapi menolak jalan tanpa `confirm_permanent`
dan tidak dipanggil dari CLI.

## Catatan
* `serial_win32.c` satu-satunya file yang bergantung pada Windows (selain `sign.c` untuk
  argumen UTF-8). Untuk platform lain cukup tulis ulang 9 fungsi di `serial_win32.h`.
* Verifikasi ECDSA tidak constant-time (hanya memakai data publik).
* Port yang diberikan lewat `--port` dipakai apa adanya (tanpa fallback scan).
