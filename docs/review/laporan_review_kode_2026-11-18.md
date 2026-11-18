# Laporan Review Kode: Skripsi ECDSA P-256 software vs DS28E38

Disusun: 18 Nov 2026 (siang). Acuan: `main` @ `9cc562d` **ditambah** perubahan yang belum di-commit saat itu
(`platformio.ini`, `src/host/deepsleep/stop_common.h`, `stop_window.h`, `tools/analisis_d.py`).
Mode: AUDIT (laporan dulu, tidak ada kode proyek yang diubah). Seluruh pemeriksaan hanya-baca, tanpa build, tanpa
membuka port, tanpa menyentuh `.pio/`, sesuai aturan "jangan mengganggu VS Code".

## 0. Alur yang dipakai (sesuai aturan masalah-di-kode)

| Langkah | Hasil |
|---|---|
| Brainstorming | Permintaan sudah jelas (laporan review menyeluruh sampai hari ini). Tidak ada pertanyaan yang mengubah isi laporan. Saya tetapkan sendiri dan tulis di sini: cakupan = firmware, protokol, alat PC, konfigurasi build, dan perubahan 18 Nov. |
| Diagnosing-superpowers | **Tidak dijalankan.** Skill itu membedah transkrip sesi Claude, bukan bug kode. Dicatat agar jelas bahwa ini bukan terlewat. |
| Mentor | Masukan ada di bagian 5 (satu langkah + satu kejutan). |
| Senior engineering partner | Seluruh temuan di bagian 3 dan urutan perbaikan di bagian 6. Tier rigor: **Tier 1 (riset, satu pengembang, data riil milik sendiri)**. Lantai keamanan tetap berlaku. |
| Keputusan | Bagian 6. Tidak ada perbaikan yang saya terapkan sebelum Anda memilih. |

## 1. Vonis

Kode intinya **sehat dan jujur secara metodologi**: pengukuran punya saksi independen, kegagalan dicatat dan dicabut
terang-terangan, dan ada gerbang biner identik. Tidak ada darurat keamanan (tidak ada rahasia sungguhan di repo).

Risiko terbesar bukan bug di jalur ukur tahap D, melainkan **jejak asal-usul data varian Stop (deep sleep)**:
sumber yang menghasilkan data pilot Stop berbeda dari sumber yang sedang ada di working tree, dan sesi tunggal tidak
mencatat hash biner (H1). Di samping itu, bangun-dari-Stop masih bersandar pada heuristik pewaktuan yang terus
ditambal (H2). Keduanya harus tuntas **sebelum** data final Stop (langkah 12) diambil.

## 2. Yang saya verifikasi secara mekanis (bukan dari dokumen)

| Pemeriksaan (baca-saja) | Hasil |
|---|---|
| `git status` / `git diff --stat` | 4 file termodifikasi belum di-commit; `docs/belajar/` dan 1 laporan baru belum terlacak |
| Waktu ubah file vs waktu sesi data (`stat`, `ls data/runs`) | `stop_common.h` diubah 11:13, `stop_window.h` 10:47; sesi K2S terakhir 11:01 |
| `sesi.txt` sesi K2S tunggal `20261118_110112_K2S` | **tidak ada** baris `elf_*` (hash biner) |
| `tools/run_bench.py:574-575` | hash ELF ditulis hanya bila `campur` (sesi campuran K1/K2) |
| `tools/biner_acuan.json` | berisi 11 env; **tanpa** `host_k1_stop`, `host_k2_stop`, `host_s*` |
| `platformio.ini` baris 29-31, 100, 201 | `platform = ststm32` tanpa versi; `OneWire@^2.3.8` rentang |
| Versi terpasang (`~/.platformio`) | platform 19.3.0, framework-arduinoststm32 4.21001.250617, toolchain 1.120301.0, OneWire 2.3.8 |
| `git grep` pola berbahaya di `*.py` (`shell=True`, `os.system`, `eval`, `exec`, `pickle`) | 0 temuan |
| `git grep` C berbahaya (`strcpy`, `sprintf`, `gets`, `scanf`) di `src/ include/ lib/bench_proto ds9481_comm/c` | 2 pemakaian: `ds9481_comm/c/src/sign.c:47` (aman, buffer `h[65]` untuk 32 byte = 64 + NUL), `eksperimen/monolit/main.cpp:268` (konstanta literal, aman) |
| `git grep TODO/FIXME/HACK` | 0 |
| Berkas uji | `test/` hanya templat; uji nyata hanya `ds9481_comm/tests/test_offline.py`, `ds9481_comm/c/tests/`, `tools/emu_cycles/test_*.c`; tidak ada uji untuk `run_bench.py` / `analisis_d.py`; tidak ada `.github/` |
| `requirements*.txt` | hanya `ds9481_comm/requirements.txt` (`pyserial>=3.5`); lingkungan analisis (`.venv-analisis`) tak punya berkas versi |
| Ukuran repo | 608 berkas terlacak, 420 di `data/` (166 MB total, terbesar 1,5 MB); remote publik |

**Tidak saya jalankan:** build, uji, OpenOCD, port serial, `pio` apa pun. Jadi tidak ada klaim "lulus build" atau
"lulus uji" di laporan ini.

## 3. Temuan (urut tingkat keparahan)

### HIGH

**H1. Asal-usul biner data Stop tidak terjamin.**
`src/host/deepsleep/stop_common.h` (diubah 11:13) dan `stop_window.h` (10:47) belum di-commit, sedangkan sesi
pilot `20261118_105112…110112_K2S` diambil pada 10:51-11:01 dan commit pilot `9cc562d` pada 10:12. Sesi tunggal tidak
menulis hash (`sesi.txt` hanya menulis `elf_*` untuk sesi campuran, `run_bench.py:574`), dan hash yang ditulis adalah
**ELF**, padahal logbook 16 Nov sendiri memutuskan membandingkan `firmware.bin` karena ELF berubah oleh info debug.
`biner_acuan.json` juga tidak mencakup env Stop.
**Dampak:** tidak bisa dibuktikan bahwa data Stop berasal dari kode yang tersimpan. Itu prinsip yang justru dijaga
ketat untuk tahap D. Gerbang ini menjadi pemblokir begitu data final Stop diambil (pemicu: langkah 12).
**Perbaikan (usulan, urut):**
1. Setelah Anda puas dengan perilaku `rx_idle_strict` dan penerima dini, **commit** semua perubahan Stop.
2. `run_bench.py`: untuk SEMUA sesi tulis `sha256` dari `.pio/build/<env>/firmware.bin` (bukan ELF), `git rev-parse HEAD`,
   dan penanda `dirty` (`git status --porcelain` tidak kosong). Dapat dilakukan tanpa mengubah isi firmware.
3. Sebelum data final: tag `stop-data-final` pada commit itu, lalu `cek_biner.py simpan` untuk env Stop.
4. Opsi tanpa menyentuh `run_bench.py`: skrip pasca-build di bawah ini menulis `provenance.json` di samping biner:

```python
# extra_script_provenance.py  (SKETSA, belum dijalankan; tidak mengubah isi firmware.bin)
Import("env")
import hashlib, json, subprocess, pathlib, datetime

def _tulis(source, target, env):
    b = pathlib.Path(str(target[0]))
    git = lambda *a: subprocess.run(["git", *a], capture_output=True, text=True).stdout.strip()
    info = {
        "env": env["PIOENV"],
        "bin_sha256": hashlib.sha256(b.read_bytes()).hexdigest(),
        "git_head": git("rev-parse", "HEAD"),
        "dirty": bool(git("status", "--porcelain", "--", "src", "lib", "include", "platformio.ini")),
        "waktu": datetime.datetime.now().isoformat(timespec="seconds"),
    }
    (b.parent / "provenance.json").write_text(json.dumps(info, indent=2))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _tulis)
```

Verifikasi sintaks `AddPostAction` dengan versi SCons yang terpasang sebelum dipakai.

**H2. Bangun dari Stop lewat heuristik pewaktuan bit belum andal.**
`stop_common.h` menunggu garis tinggi tak putus >= 11 us di akhir byte pertama `0xA5` (jendela sinkron 40-85 us
sejak bangun, `SYNC_MIN_US`/`SYNC_MAX_US`), lalu menyalakan USART+DMA untuk 35 byte sisanya. Hasil terdokumentasi: K2S barrage kehilangan/merusak 1-4 frame per 600
(`docs/rencana_deep_sleep.md`, langkah 9). Perubahan 18 Nov menambah dua lapis tambalan (`rx_idle_strict` 120 us, penerima
dini di HSI sebelum PLL siap). Setiap tambalan memperbaiki satu mode kegagalan, tetapi rancangan dasarnya tetap
bergantung pada jitter bangun yang tidak dijamin.
**Dampak:** `N_dtg`, TL, dan S_cpu varian Stop pada barrage bisa tercemar frame yang hilang karena bangun, bukan karena
host sibuk. Itu memutarbalikkan makna "terlewat" di model kejenuhan.
**Perbaikan, tiga opsi:**
1. *Paling sederhana:* attacker mengirim **satu byte pembuka** sebelum frame (mis. `0x55`) atau jeda garis-rendah singkat
   sebagai "bel", sehingga host punya waktu bangun deterministik. Perubahan ~3 baris di `src/attacker/main.cpp` plus
   `proto_attacker_host.h`. Menyalahi keputusan "attacker tidak diubah", jadi itu keputusan Anda. Biner attacker
   tidak ikut data tahap D (attacker tidak termasuk gerbang biner), tetapi catat di logbook.
2. *Tanpa mengubah attacker:* terima bahwa ada kehilangan, **laporkan `N_rusak + N_hilang` per run**, jadikan kriteria
   sah/gugur (mis. run dengan > 0,5 % frame hilang digugurkan dan diganti), dan tulis batasannya di Bab 5.
3. *Gabungan:* jalankan opsi 2 sekarang, evaluasi opsi 1 setelah data pilot ulang.
Saya sarankan opsi 3. Alasan: opsi 2 murah dan jujur, opsi 1 menutup akar masalahnya.

### MEDIUM

**M1. Toolchain tidak dipin.** `platformio.ini:30` `platform = ststm32`, `:100,:201` `OneWire@^2.3.8` (rentang).
Terpasang 19.3.0 / framework 4.21001.250617 / GCC 1.120301.0 / OneWire 2.3.8. `usb_serial_fix()`
(`src/logger/f103/main.cpp:495`) menulis ke array internal core `USBD_StringSerial`, yang bisa berubah antar versi core.
**Dampak:** klon baru atau pembaruan platform menghasilkan biner lain, gerbang hash gagal, dan patch USB logger bisa patah
tanpa pesan jelas. **Perbaikan:**
```ini
[env]
platform = ststm32@19.3.0
platform_packages =
  platformio/framework-arduinoststm32@4.21001.250617   ; verifikasi nama paket: `pio pkg show` (jangan disalin buta)
  platformio/toolchain-gccarmnoneeabi@1.120301.0
; lib_deps: paulstoffregen/OneWire@2.3.8
```
**Catatan:** mengubah `platformio.ini` membuat PlatformIO **menghapus semua folder build** (komentar kepala berkas).
Lakukan saat tidak ada build atau flash yang berjalan di VS Code, lalu bangun ulang dan jalankan `cek_biner.py banding`.

**M2. Alat analisis dan kendali run tidak punya uji dan lingkungannya tidak terkunci.**
`tools/analisis_d.py` (36 KB) menghasilkan semua angka Bab 4, dan `tools/run_bench.py` (30 KB) menentukan run sah/gugur.
Tidak ada uji unit, tidak ada CI, dan `.venv-analisis` tidak punya `requirements`. Versi pustaka hanya ada di logbook
(numpy 2.5.4, scipy 1.18.1, pandas 3.1.1, statsmodels 0.15.0, matplotlib 3.11.2).
**Dampak:** salah ketik di rumus tidak ketahuan, dan hasil tak bisa direproduksi di mesin lain.
**Perbaikan:**
1. `requirements-analisis.txt` dengan versi di atas (cek ulang dengan `pip freeze` di venv itu).
2. `tests/test_metrik.py`: data sintetis kecil dengan jawaban yang bisa dihitung tangan: `TL_model`, `E_lyn = P_dyn/Theta`,
   `S_cpu`, koreksi jam (`corr`), aturan sah/gugur. Pola uji: hitung dulu dengan kalkulator, tulis sebagai nilai tetap.
3. Satu uji "golden": jalankan `analisis_d.py` pada subset kecil data dan bandingkan `ringkasan.md` terhadap berkas acuan.
4. GitHub Actions ringan: `ruff` + `pytest` (tanpa PlatformIO).
Semua berkas di atas **baru** (aman terhadap VS Code).

**M3. Frekuensi HCLK terukur antar sesi Stop berbeda 2,3 %.** `sesi.txt` pilot 09:48 mencatat `47.781.656` Hz, sesi 11:01
`48.895.817` Hz (`>ID`, kolom 4). Pola tahap D adalah -0,55 % konsisten. Selisih ~2,3 % dalam satu jam belum terjelaskan
(suhu HSI? perubahan urutan kalibrasi akibat penerima dini? kesalahan di `measure_hclk()`?). Karena latensi Stop dikonversi
memakai nilai ini (`STOP_LAT_RTC`), ia harus dijelaskan sebelum data final. **Langkah:** catat `clk_hz` per run di CSV ringkasan
dan lihat sebarannya antar run dalam satu sesi; bila stabil dalam sesi tetapi berbeda antar sesi, itu soal HSI/suhu, bukan bug.

**M4. Dokumentasi menyimpang dari kode.** `README.md` tidak menyebut varian Stop, `flash_stop.py`, `k1s_run.py`, `s0*_*.py`,
`ds_od.py`. `docs/pinout_arsitektur_pengujian.md` (16 Nov) masih menulis R seri attacker->host ~1 kOhm (baris 30, 101, 115, 168),
padahal sejak 17 Nov 10 kOhm. Butir `[cek]` pull-up 1-Wire masih terbuka.
**Perbaikan:** tambah satu bagian "Varian Stop" di README, perbarui tabel alat dan pinout, hapus butir `[cek]` yang sudah
diukur. Dokumen yang menggambarkan wiring wajib ikut berubah ketika wiring berubah.

### LOW (catat, perbaiki bila lewat di sana)

- **L1. Baca di luar batas pada diagnostik.** `stop_common.h` (diff 18 Nov, `dbg_bad`): `((volatile uint32_t *)g_rx)[i]` untuk
  `i < 10` membaca 40 byte dari buffer 36 byte (`g_rx[FRAME_LEN]`), dan tanpa jaminan rata 4 byte. Hanya aktif dengan
  `-DDBG_RX`. Perbaikan: `static uint8_t g_d_bad_rx[FRAME_LEN]; memcpy(g_d_bad_rx, g_rx, FRAME_LEN);`.
- **L2. Konstanta ajaib `8` siklus/us** (`120UL * 8UL`, `8000UL * 8UL`, `cycles() ... / 8`) bergantung pada HSI 8 MHz.
  Benar sekarang, tetapi turunkan dari satu `#define HSI_MHZ 8` agar tidak diam-diam salah bila jam wake berubah.
- **L3. `rx_idle_strict()` memutar 120 us dengan interupsi dimatikan** (`wait_event`, `__disable_irq()` lebih dulu) tiap
  sebelum Stop. Biaya waktu aktif kecil (< 0,03 % pada 10 Hz) tetapi **harus disebut di metodologi Stop** karena tahap D
  tidak punya padanannya.
- **L4. Potongan baris di `hl_poll_line`** (`host_link.h:111`): bila baris melebihi 23 karakter, `n = 0` dan sisa karakter
  dibaca sebagai baris baru. Aman selama hanya logger yang menulis; perbaiki dengan penanda "buang sampai newline".
- **L5. Tulis blocking tanpa batas waktu** (`host_link.h:73-82`, `stop_link.h`): jika USART1 mati, host menggantung. Tambah
  batas siklus + hitung `g_hl_err`.
- **L6. Kanal perintah logger lewat nilai baud** (`logger/f103/main.cpp:561`): semua baud 1.000.000-1.999.999 dijalankan sebagai
  perintah. Membuka port dengan program lain pada 1 Mbaud memicu `#ERR,cmd`. Jinakkan dengan wajibkan `cmd >= 1`
  (sudah default ke `#ERR`), dan dokumentasikan di README logger.
- **L7. Jalur pemulihan I2C belum pernah diuji** (A1 ditunda). Ada di jalur ukur (`main.cpp:374-378`), tidak pernah terpicu
  (`rec = 0` di data tahap D). Pemicu naik tingkat: ada `rec > 0` di data final mana pun. Rencanakan uji cabut-SDA/SCL satu kali.
- **L8. `default_envs = sederhana_stlink`** (`platformio.ini:26`): tombol Build/Upload VS Code membangun dan meng-upload
  firmware eksperimen ke host `$`, menimpa firmware pengukuran. Pertimbangkan env yang paling sering dipakai atau komentar
  peringatan di atas baris itu. **Jangan diubah sekarang** karena VS Code memakainya.
- **L9. Kunci privat UJI di repo publik** (`k1_software/main.cpp:73`) sudah berlabel jelas. Tambahkan satu kalimat di README
  ("kunci uji, bukan rahasia") dan allowlist bila nanti ada pemindai rahasia, agar tidak jadi temuan palsu.

## 4. Kekuatan (terverifikasi)

| Kekuatan | Bukti |
|---|---|
| Satu definisi protokol, tak mungkin pengirim/penerima berbeda | `lib/bench_proto/*.h`, `static_assert(FRAME_LEN == 1+2+PAYLOAD+1)` |
| Pola tidur bebas race | `host/k1_software/main.cpp:310-330` (`disable_irq` -> cek flag -> `WFI` -> `enable_irq`, `ISB`) |
| Akuntansi waktu ganda yang saling mengunci | `act_cyc` (CYCCNT) vs `T_sleep` (timer 1 us), sisa 2-107 us |
| Saksi independen untuk S_cpu | pin PB1 -> logger berkristal; selisih <= 0,004 poin |
| Pengukuran RAM menolak data tak sah | `host_ram.h:72`: iterasi dengan heap tumbuh ditandai, bukan dipakai |
| Jalur galat diuji paksa | `tools/uji_galat.py`, 13/13; uji `K1_RX_TEST_MISALIGN` menemukan bug DMA yang tak terlihat di uji normal |
| Perubahan "aman" dibuktikan, bukan diyakini | `tools/cek_biner.py` + `biner_acuan.json`, hash `firmware.bin` identik bit per bit |
| Aturan sah/gugur ditetapkan dengan bukti | blok 3: run "late" tak berbeda arusnya (selisih <= 0,008 mA) |
| Kesalahan sendiri dicatat dan dicabut | logbook 13 Nov (comb), 14 Nov (COUNTFLAG), 15 Nov (kabel PB1) |
| Kode tidak punya pola injeksi/eksekusi dinamis | `git grep` 0 temuan (bagian 2) |
| Pustaka kripto vendor dengan lisensi dan versi tercatat | `lib/mbedtls/LICENSE`, `MBEDTLS_VERSION_STRING "3.6.7"` |
| Analisis deterministik | `analisis_d.py`: seed tetap (`SEED`), bootstrap 10.000 resampel |

## 5. Masukan mentor

**Satu langkah, dibuktikan di repo Anda:** pindahkan *provenance* ke **luar** firmware. Hash biner dan commit ditulis oleh
skrip pasca-build di samping `firmware.bin` (sketsa di H1). Isi biner tidak berubah, jadi gerbang `cek_biner.py` tetap
identik, sedangkan setiap sesi data bisa menunjuk ke bukti. Itu menutup H1 tanpa melanggar disiplin "biner identik".

**Satu kejutan:** aturan baru Anda ("jangan mengganggu VS Code") bisa dipaksakan **secara mekanis**, bukan hanya lewat
memori. Claude Code mendukung aturan izin (`deny`) di `.claude/settings.local.json`, mis. menolak perintah
`pio run*`, `pio device*`, `openocd*`, `git checkout*`, `git stash*`, `git reset*`. Aturan itu berlaku walau saya lupa, dan
tidak ikut ter-commit bila berada di berkas `.local`. Ini perubahan konfigurasi, jadi **saya tidak menerapkannya** tanpa
persetujuan Anda.

## 6. Urutan perbaikan yang disarankan (keputusan senior)

| # | Langkah | Siapa/Kapan | Menyentuh berkas yang sedang dipakai VS Code? |
|---|---|---|---|
| 1 | Commit perubahan Stop setelah Anda puas (H1.1) | Anda, saat jeda | ya, hanya Anda yang boleh |
| 2 | Berkas baru: `requirements-analisis.txt`, `tests/test_metrik.py`, `extra_script_provenance.py` (M2, H1.4) | saya, kapan saja | tidak |
| 3 | Putuskan opsi H2 (1/2/3) | Anda | tidak |
| 4 | Pin toolchain (M1) dan daftarkan skrip provenance di `platformio.ini` | Anda atau saya, **saat tidak ada build/flash berjalan** | ya (`platformio.ini`) |
| 5 | Perbarui README dan pinout (M4), jelaskan `clk_hz` (M3) | saya | README/docs: tidak mengganggu |
| 6 | Perbaikan LOW (L1-L9) sambil lewat | menyusul | sebagian ya |

Yang tidak saya sarankan: menulis ulang rancangan bangun-dari-Stop sekarang, atau menambah fitur baru sebelum H1 dan H2
beres. Alasan: bukti terbaik skripsi adalah data yang bisa ditelusuri, bukan fitur tambahan.

## 7. Cakupan dan batas

**Dibaca penuh:** `host/k1_software/main.cpp` (sampai jendela), `host/k2_hardware/main.cpp` (inti), `host_common/*`,
`bench_proto/*`, `logger/f103/main.cpp`, `logger_common/{ina226,edge_stats}.cpp`, `attacker/main.cpp` (sebagian),
`stop_common.h`/`stop_window.h` (+ diff 18 Nov), `platformio.ini`, `mbedtls_k1_config.h`, `analisis_d.py` (diff + pola).
**Hanya dipindai pola/ringkas:** `run_bench.py`, `cek_biner.py`, `ds9481_comm/**`, `host/deepsleep/{k1,k2}_stop`, `ds28e38_od.h`.
**Tidak ditinjau:** kebenaran statistik `analisis_d.py` baris demi baris, isi `lib/mbedtls` terhadap rilis upstream,
data CSV mentah, dan perilaku di perangkat keras (tidak ada pengujian yang dijalankan).
Temuan yang tidak bisa saya verifikasi dari berkas diberi label "belum terjelaskan" (M3) atau "usulan".

## 8. Tindak lanjut (18 Nov, siang): langkah 2 "berkas baru" selesai

Hanya berkas BARU yang dibuat. Tidak ada berkas yang sedang dipakai VS Code yang diubah, tidak ada build, flash, atau port.

| Berkas baru | Menutup | Status |
|---|---|---|
| `requirements-analisis.txt` | M2 (lingkungan analisis) | versi disalin dari paket terpasang di `.venv-analisis` (Python 3.12.10); bukan `pip freeze` |
| `tests/test_metrik.py` | M2 (uji alat) | 22 uji: `parse`, `layanan`, `energi_sesi`, `analyse` dengan log sintetis yang nilainya dihitung tangan |
| `tools/provenance.py` + `tests/test_provenance.py` | H1 (asal-usul biner) | 7 uji, memakai repo git sementara di folder temp |
| `extra_script_provenance.py` | H1 | **belum didaftarkan** di `platformio.ini` dan belum pernah dijalankan oleh PlatformIO |

Hasil: `python -B -m unittest discover -s tests` memberi **29 uji lulus** (dijalankan dengan `.venv-analisis`, tanpa pytest,
tanpa membuka port). Satu uji gagal pada percobaan pertama dan menemukan galat di `provenance.py` sendiri
(`strip()` memakan spasi awal status porcelain, nama berkas jadi `rc/a.cpp`). Sudah diperbaiki dan diuji ulang.

Bukti sampingan untuk H1: `provenance.kumpulkan()` yang dijalankan pada repo nyata (tidak menulis apa pun ke repo) melaporkan
`dirty: true` dengan `platformio.ini`, `stop_common.h`, dan `stop_window.h` sebagai berkas yang berubah.

Batas yang jujur:
- Uji `analyse` memakai data sintetis dengan satu bentuk run. Ia membuktikan rumus dan aturan sah/gugur, bukan kebenaran
  statistik `analisis_d.py` (belum ada uji untuk skrip itu).
- Uji tidak membuktikan hook SCons berjalan. Itu baru terbukti setelah didaftarkan dan satu env dibangun (lihat catatan di
  kepala `extra_script_provenance.py`).
- Pin versi tidak memverifikasi integritas unduhan (catatan di `requirements-analisis.txt`).
