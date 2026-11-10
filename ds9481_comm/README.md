# ds9481_comm — komunikasi PC → DS28E38 & DS2476 lewat adaptor USB DS9481P-300

Kode Python mandiri (tidak butuh kode Maxim, tidak butuh Visual Studio) yang
berbicara ke dua IC lewat **COM port virtual** milik DS9481P-300:

```
PC ──USB/COM──► DS9481P-300 ──1-Wire──► DS28E38
                            └──I2C────► DS2476
```

Adaptor hanya bisa berada di **satu mode** pada satu waktu. Kelas `DS9481`
mengganti mode otomatis (1-Wire ⇄ I2C) setiap kali kamu memakai chip lain.

## Pasang & jalankan

```
pip install pyserial
python test_ds28e38.py --port COM13   # hanya DS28E38 (1-Wire), 7 langkah
python test_ds2476.py  --port COM13   # hanya DS2476 (I2C), 7 langkah
# tambahkan --debug untuk melihat byte TX(>) / RX(<)
```

Dua skrip dijalankan **terpisah** dan masing-masing berhenti di langkah pertama
yang gagal, jadi penyebab masalah mudah dilokalisasi. Pasang hanya board yang
sedang dites ke adaptor. `test_ds2476.py` juga memindai seluruh alamat I2C
(`--no-scan` untuk melewati, `--addr 0x..` untuk alamat 8-bit lain).

Pertama kali DS9481P-300 dicolok, Windows meminta driver: arahkan ke folder
`DS9481P_device_driver` dari paket Maxim.

Skrip tes hanya **membaca** (status, halaman, RNG). Tidak ada yang ditulis/dikunci.

## Tanda tangan digital (`sign.py`) — cara termudah

```
python sign.py info                                  # ID chip + kunci publik
python sign.py sign "pesan yang mau ditandatangani"  # hasil: tanda_tangan.json
python sign.py sign --file dokumen.pdf               # tandatangani file
python sign.py verify tanda_tangan.json "pesan"      # cek, TANPA chip
python sign.py verify tanda_tangan.json --pub X_hex,Y_hex   # pakai kunci tepercaya
```

Kunci privat tetap di dalam DS28E38. Pesan di-hash SHA-256 dan hash itu dikirim
sebagai challenge; chip menandatangani SHA-256(`ROM | data halaman | challenge |
no.halaman | MANID`). `verify` menghitung ulang itu di PC dan memeriksa ECDSA P-256
dengan kunci publik chip (halaman 4 dan 5). Opsi: `--page 0..5`, `--anon`
(ROM ID tidak ikut), `--out nama.json`, `--port COM13`, `--debug`.

Diuji di hardware asli: tanda tangan chip valid terhadap kunci publik di halaman
4/5; pesan yang diubah atau tanda tangan yang dirusak ditolak.

## Versi C

Folder [`c/`](c/README.md) berisi migrasi ke C (`sign.exe`: `info`, `sign`, `verify`) dengan
format JSON yang sama; hasilnya saling diverifikasi dengan `sign.py`.

## Pakai dari kode sendiri

```python
from ds9481 import DS9481, DS28E38, DS2476

with DS9481(port="COM5") as adapter:
    e38 = DS28E38(adapter)
    status = e38.detect()                    # status + ROM number + MANID
    page0 = e38.read_memory(0)
    r, s = e38.compute_read_page_auth(3, challenge=bytes(32))   # tanda tangan ECDSA
    msg = e38.signed_message(3, e38.read_memory(3), bytes(32))  # pesan yg ditandatangani

    coproc = DS2476(adapter)                 # adaptor otomatis pindah ke I2C
    coproc.detect()
    rnd = coproc.read_rng(16)
```

## Isi folder

| File | Fungsi |
|---|---|
| `ds9481/transport.py` | COM port (pyserial): 8N1, DTR on, RTS off, timeout `40 ms·n + 205 ms` |
| `ds9481/onewire.py` | Master 1-Wire (mode COMMAND/DATA, reset, blok, strong pull-up, READ/SKIP/MATCH ROM) |
| `ds9481/i2c.py` | Master I2C (`S Q W R N P H V`, 115200 baud) |
| `ds9481/adapter.py` | Cari port, deteksi adaptor, pindah mode 1-Wire ⇄ I2C |
| `ds9481/ds28e38.py` | Perintah DS28E38: bingkai XPC `0x66` + CRC16 |
| `ds9481/ds2476.py` | Perintah DS2476: `cmd · length · param` + SMBus |
| `sign.py` | CLI tanda tangan: `info` / `sign` / `verify` |
| `ds9481/ecdsa_verify.py` | Verifikasi ECDSA P-256 murni Python |
| `ds9481/crc.py` | CRC8 & CRC16 Dallas |
| `test_ds28e38.py`, `test_ds2476.py` | Skrip uji hardware, satu per chip |
| `tests/` | Tes offline dengan simulator adaptor (tanpa hardware) |

Tes: `python -m unittest discover -s tests -v`

## Perintah yang tersedia

**DS28E38**: `read_status`, `read_memory`, `write_memory`, `read_rng`,
`decrement_counter`, `compute_read_page_auth` (tanda tangan ECDSA, mode anonim),
`detect`, `signed_message`.
**DS2476**: `read_memory`, `write_memory`, `write_buffer`, `read_page_protection`,
`read_rng`, `decrement_counter`, `compute_read_page_auth` (HMAC / ECDSA), `detect`.

## Operasi permanen — sengaja dikunci

`set_page_protection` dan `generate_key_pair` **tidak bisa dibatalkan**
(proteksi WP/RP/DC dan kunci ECDSA). Keduanya melempar `PermissionError`
kecuali kamu memberi `i_understand_permanent=True`. `Device Disable` DS28E38
tidak disertakan sama sekali (mematikan chip selamanya).

## Batasan yang jujur

* **Belum diuji dengan hardware asli.** Kode ini mengikuti kode referensi Maxim
  (`1wire_ds9481.c`, `i2c_ds9481p_ds9400.c`, `ds28e38.c`, `ds2476.c`). Tes yang ada
  memakai simulator yang kutulis dari kode itu, jadi hanya membuktikan bingkai
  dan CRC konsisten, bukan bahwa adaptor/chip asli menerimanya.
* Pindah mode 1-Wire ⇄ I2C meniru Maxim (kirim `0xE5` / `'C','O'`, buka ulang
  port, scan). Perilaku port setelah pindah mode (apakah nomor COM berubah) belum
  terverifikasi; gunakan `--debug` bila gagal.
* Pindah ke I2C menunggu ±4 detik (waktu start mikrokontroler adaptor).
* Satu DS28E38 di bus 1-Wire (`select="skip"`). Untuk banyak perangkat pakai
  `select="match"` setelah `detect()`; algoritma search ROM belum dibuat.
* Verifikasi tanda tangan ECDSA (SHA-256 + kunci publik) **tidak** dilakukan di
  sini; `signed_message()` memberi pesan yang harus di-hash, lalu verifikasi
  dengan pustaka ECDSA P-256 pilihanmu (mis. `tools/p256.py` di proyek ini).
* Perintah yang belum diport: set/auth SHA-2 & ECDSA write, ECDH, multi-block
  hash, verify signature di DS2476 (koprosesor), key generation DS2476.
