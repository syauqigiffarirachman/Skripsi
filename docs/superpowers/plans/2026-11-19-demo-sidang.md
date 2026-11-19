# Demo Sidang Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Satu skrip PC (`tools/demo_sidang.py`) yang menuntun demo empat run (K1S/K2S x A/B, jendela 60 s) dengan banner HOST SIAP -> BISA MULAI MENYERANG -> jendela host, tampilan arus langsung, dan ringkasan akhir.

**Architecture:** Skrip tipis di atas `tools/run_bench.py` (mengimpor `Logger`, `host_id`, `wait_host`, `flash_host`, `analyse`, `parse`, `TICK_HZ`, `SHUNT_LSB_V`; tidak menyalin). Logika murni (tampilan langsung, urutan sesi, tabel, nama berkas) dan orkestrasi run dibuat dapat diuji tanpa perangkat dengan logger palsu dan `sleep`/`out` yang disuntikkan. Perangkat nyata hanya disentuh di Task 4, dengan izin peneliti.

**Tech Stack:** Python 3 (`.venv-analisis`), pyserial, `unittest` (gaya `tests/test_metrik.py`), OpenOCD lewat `flash_host`.

**Spec:** `docs/superpowers/specs/2026-11-19-demo-sidang-design.md`

## Global Constraints

- HANYA berkas baru: `tools/demo_sidang.py`, `tests/test_demo_sidang.py`, `docs/demo_sidang_panduan.md`. Jangan ubah firmware, `platformio.ini`, `tools/run_bench.py`, atau `data/` yang sudah ada (spec: bukan tujuan).
- Jendela host 60 s (`--secs` bawaan 60); perintah logger 4 = run S<detik> dengan attacker nyata; perintah 3 = tanya identitas host.
- Analisis memakai `run_bench.analyse` dengan `rshunt=0.100`, `offset=-2.0` (sama dengan `data/analisis/stop_final`).
- Urutan sesi tetap `K1S.A, K1S.B, K2S.A, K2S.B` (satu kali flash di depan penguji).
- Banner persis: `HOST SIAP` dan `BISA MULAI MENYERANG`. Hitung mundur dan jeda tunggu bawaan 5 s, bisa diatur (`--hitung`, `--jeda`).
- Hasil mentah di `data/demo/<YYYYmmdd_HHMMSS>/NNN_<ARCH>_<skenario>.csv` (pola yang dibaca `run_bench.py ringkas`).
- Tes dijalankan: `.venv-analisis/Scripts/python -B -m unittest discover -s tests -v` (tanpa perangkat).
- Aksi yang membuka port COM, flash, `pio run`, atau OpenOCD HANYA atas izin peneliti (VS Code sedang berjalan). Commit hanya bila peneliti meminta.

## Review Focus

1. Host tidak menjawab `?` (kabel logger->host PA10 putus): pesan jelas, perintah 4 TIDAK dikirim. (Task 2)
2. Run gagal di tengah sesi (logger timeout / NOSTART): sesi lanjut, baris tabel bertanda gagal, tidak crash. (Task 2)
3. Ctrl+C saat run: baud logger dikembalikan ke 115200 agar port tidak macet. (Task 2)
4. ELF K1S/K2S belum dibangun, atau host berisi arsitektur lain: ditolak sebelum run dengan petunjuk `mode k1|k2`. (Task 2, 3)
5. Port attacker dipakai serial monitor saat `--auto-attacker`: pesan galat jelas, bukan jejak tumpukan. (Task 3)

---

### Task 1: Logika murni (tampilan langsung, urutan, tabel, berkas)

**Files:**
- Create: `tools/demo_sidang.py`
- Create: `tests/test_demo_sidang.py`

**Interfaces:**
- Consumes: `run_bench.TICK_HZ`, `run_bench.SHUNT_LSB_V` (arus mA = `(shunt_raw - offset) * SHUNT_LSB_V / rshunt * 1e3`).
- Produces (di `tools/demo_sidang.py`):
  - `URUTAN_SESI: list[tuple[str, str]]` = `[("K1S","A"),("K1S","B"),("K2S","A"),("K2S","B")]`
  - `class LiveMeter` dengan `__init__(self, rshunt: float = 0.100, offset: float = -2.0, tick_hz: int = run_bench.TICK_HZ)` dan `feed(self, line: bytes) -> list[str]`
  - `tabel_ringkas(rows: list[dict]) -> str`
  - `simpan_run(folder: Path, urut: int, arch: str, scen: str, raw: bytes) -> Path`
  - `CATATAN_STLINK: str`

- [ ] **Step 1: Tulis tes yang gagal** di `tests/test_demo_sidang.py` (tambahkan `sys.path` ke `tools/` seperti `test_metrik.py`, lalu `import demo_sidang, run_bench`):
  - `test_urutan_sesi_satu_flash`: `sum(a[0] != b[0] for a, b in zip(URUTAN_SESI, URUTAN_SESI[1:])) == 1`.
  - `test_livemeter_arus_per_detik`: umpan baris `s,<t>,84,0` untuk 500 cuplikan per detik selama 2 detik (t = k * TICK_HZ // 500) plus satu cuplikan di detik ke-2; hasil `feed` mengandung tepat dua baris tampilan (detik 0 dan 1) berisi `I=` yang `assertAlmostEqual` dengan `84 * run_bench.SHUNT_LSB_V / 0.100 * 1e3` (dua desimal).
  - `test_livemeter_baris_host_penting_saja`: `h,100,>START,60` -> keluar baris berisi `>START`; `h,200,>L,1,2,3` -> daftar kosong; `h,300,>END,...` dan `h,400,>DONE` -> keluar.
  - `test_tabel_ringkas_empat_baris`: dari empat dict berkunci `arch, scen, I_avg_mA, arrived, served, N_rusak, N_lwt, lat_mean_ms_nyata, TL_pct` (+ satu baris dengan kunci `masalah="NOSTART"`), keluaran memuat `K1S`, `K2S`, `A`, `B`, dan kata `GAGAL` hanya pada baris bermasalah; baris tanpa kunci opsional tidak membuat `KeyError`.
  - `test_simpan_run_nama_cocok_ringkas`: `simpan_run(tmp, 3, "K2S", "B", b"x")` menghasilkan `003_K2S_B.csv` dan `stem.split("_", 2)[2] == "B"`; isi berkas == `b"x"`.
- [ ] **Step 2: Jalankan, pastikan gagal**
  Run: `.venv-analisis/Scripts/python -B -m unittest tests.test_demo_sidang -v`
  Expected: gagal karena `demo_sidang` belum ada (ImportError).
- [ ] **Step 3: Implementasi** semua yang ada di blok Produces di `tools/demo_sidang.py`. `LiveMeter`: kumpulkan cuplikan `s,` per detik logger (`t // tick_hz`), keluarkan baris `t=<detik> s  I=<mA> mA  tepi PB1=<n>` saat detik berganti; baris `h,` hanya diteruskan bila mengandung `>START`, `>END`, `>DONE`, atau `>ERR`; `e,` hanya menambah penghitung tepi. `CATATAN_STLINK` = satu kalimat: ST-Link host terpasang selama demo sehingga arus diam terbaca sedikit lebih rendah (0,03-0,11 mA, catatan 15 Nov) dan angka dapat sedikit berbeda dari tabel skripsi.
- [ ] **Step 4: Jalankan tes, pastikan lulus**
  Run: `.venv-analisis/Scripts/python -B -m unittest tests.test_demo_sidang -v`
  Expected: 5 tes PASS.
- [ ] **Step 5: Commit** (hanya bila peneliti meminta): `tools/demo_sidang.py tests/test_demo_sidang.py`.

### Task 2: Orkestrasi satu run dan satu sesi

**Files:**
- Modify: `tools/demo_sidang.py`
- Modify: `tests/test_demo_sidang.py`

**Interfaces:**
- Consumes: Task 1 (`LiveMeter`, `simpan_run`, `URUTAN_SESI`); `run_bench.host_id(lg) -> str|None`, `run_bench.wait_host(lg, arch=None, maks=25) -> str|None`, `run_bench.flash_host(stlink, arch)`, `run_bench.analyse(text, rshunt, offset) -> (dict, list[str])`, `Logger.command(cmd, arg, timeout, on_line=None) -> bytes`, `Logger.s.baudrate`.
- Produces:
  - `class HostError(Exception)`; `class HostTidakMenjawab(HostError)`; `class HostSalahArsitektur(HostError)`
  - `jalankan_run(lg, arch: str, scen: str, secs: int = 60, hitung: int = 5, jeda: int = 5, *, out=print, sleep=time.sleep, on_start=None, analisis=run_bench.analyse, rshunt: float = 0.100, offset: float = -2.0) -> tuple[bytes, dict, list[str]]`
  - `ganti_mode(lg, arch: str, stlink: str = "$", *, out=print, flash=run_bench.flash_host, wait=run_bench.wait_host) -> str` (mengembalikan baris `>ID`; `HostTidakMenjawab` bila `wait` mengembalikan None)
  - `jalankan_sesi(lg, folder: Path, *, secs=60, hitung=5, jeda=5, stlink="$", tunggu=input, out=print, sleep=time.sleep, flash=run_bench.flash_host, wait=run_bench.wait_host, analisis=run_bench.analyse, on_start_factory=None, rshunt=0.100, offset=-2.0) -> list[dict]` (satu dict per run: kunci hasil `analyse` + `arch`, `scen`, `file`, dan `masalah` berupa string)

- [ ] **Step 1: Tulis tes yang gagal** dengan `FakeLogger` (atribut `s` berisi `baudrate=115200`; `command(cmd, arg, timeout, on_line=None)` mencatat panggilan, untuk cmd 3 mengembalikan `b"h,1,>ID,K1S,8,48000000,1\r\n#CMD,SELESAI\r\n"`, untuk cmd 4 memanggil `on_line` pada baris `h,5,>START,...` lalu mengembalikan CSV sintetis) dan `analisis` palsu `lambda t, r, o: ({"I_avg_mA": 2.0, "arrived": 60, "served": 60, "N_rusak": 0}, [])`:
  - `test_urutan_banner_dan_perintah`: `out` terkumpul; assert indeks `HOST SIAP` < indeks `BISA MULAI MENYERANG`; panggilan logger berurutan `(3, 0)` lalu `(4, 60)`; `sleep` dipanggil `[1]*hitung + [jeda]` kali dengan nilai itu.
  - `test_host_tidak_menjawab_tidak_kirim_run`: cmd 3 mengembalikan tanpa baris `>ID` -> `HostTidakMenjawab`; tidak ada panggilan `(4, ...)`.
  - `test_host_salah_arsitektur`: host menjawab `K2S` padahal diminta `K1S` -> `HostSalahArsitektur` dengan pesan memuat `mode k1`.
  - `test_baud_dikembalikan_saat_ctrl_c`: cmd 4 melempar `KeyboardInterrupt` -> setelah pengecualian merambat, `lg.s.baudrate == 115200`.
  - `test_sesi_lanjut_bila_satu_run_gagal`: `FakeLogger` melempar `TimeoutError` pada run kedua -> `jalankan_sesi` mengembalikan 4 dict, dict kedua `masalah` tidak kosong, ketiga dan keempat berhasil; `flash` palsu dipanggil tepat satu kali (dengan `"K2S"`); `tunggu` palsu dipanggil sebelum tiap run; empat berkas `NNN_*.csv` tertulis untuk run yang berhasil di `tmp`.
- [ ] **Step 2: Jalankan, pastikan gagal**
  Run: `.venv-analisis/Scripts/python -B -m unittest tests.test_demo_sidang -v`
  Expected: 5 tes baru gagal (AttributeError/ImportError), 5 tes Task 1 tetap lulus.
- [ ] **Step 3: Implementasi** di `tools/demo_sidang.py`. `jalankan_run`: (1) `ident = run_bench.host_id(lg)`; None -> `HostTidakMenjawab` (pesan menyebut kabel logger->host PA10); `ident.split(",")[1] != arch` atau `[4] != "1"` -> `HostSalahArsitektur` (pesan: ketik `mode k1`/`mode k2`); (2) cetak `HOST SIAP` lalu hitung mundur `hitung` kali `sleep(1)`; (3) cetak `BISA MULAI MENYERANG` dengan petunjuk "ketik `a`" (A) atau "ketik `b`" (B) di serial monitor attacker, lalu `sleep(jeda)`; (4) `lg.command(4, secs, secs + 20, on_line)` dengan `on_line` meneruskan ke `LiveMeter.feed` (cetak hasilnya via `out`) dan memanggil `on_start()` sekali pada `>START`; blok `try/finally` mengembalikan `lg.s.baudrate = 115200`; (5) `analisis(raw.decode(errors="replace"), rshunt, offset)`. `jalankan_sesi`: iterasi `URUTAN_SESI`; saat arsitektur berubah dari run sebelumnya (atau run pertama bila host belum berisi arsitektur itu) panggil `ganti_mode`; setiap run didahului `tunggu("Enter untuk run ...")`; tangkap `HostError`, `TimeoutError`, `RuntimeError` per run menjadi `masalah`; setelah run cetak `analisis` dan ingatkan "ketik x di attacker".
- [ ] **Step 4: Jalankan tes, pastikan lulus**
  Run: `.venv-analisis/Scripts/python -B -m unittest tests.test_demo_sidang -v`
  Expected: 10 tes PASS.
- [ ] **Step 5: Commit** (hanya bila diminta): `tools/demo_sidang.py tests/test_demo_sidang.py`.

### Task 3: CLI interaktif, praperiksa, mode `--kering`, attacker otomatis

**Files:**
- Modify: `tools/demo_sidang.py`
- Modify: `tests/test_demo_sidang.py`

**Interfaces:**
- Consumes: Task 2 (`jalankan_run`, `jalankan_sesi`, `ganti_mode`, `HostError`), `run_bench.ELF`, `run_bench.Logger`, `run_bench.Attacker(port).send(ch)`.
- Produces:
  - `periksa_elf(archs: list[str]) -> list[str]` (daftar pesan untuk ELF yang belum ada; kosong = aman)
  - `buat_parser() -> argparse.ArgumentParser` dengan opsi `--logger` (bawaan `COM18`), `--stlink` (`$`), `--secs` (60), `--hitung` (5), `--jeda` (5), `--rshunt` (0.100), `--offset` (-2.0), `--auto-attacker PORT` (opsional), `--kering`
  - `main(argv: list[str] | None = None) -> int`
  - Perintah interaktif: `ping`, `mode k1|k2`, `run A|B`, `sesi`, `ringkas`, `bantuan`, `keluar`

- [ ] **Step 1: Tulis tes yang gagal:**
  - `test_periksa_elf_melaporkan_yang_hilang`: dengan `run_bench.ELF` diganti sementara (patch `dict`) ke jalur yang tidak ada -> pesan memuat nama arsitektur dan `pio run -e`; ke berkas yang ada -> daftar kosong.
  - `test_kering_mencetak_urutan_tanpa_perangkat`: `main(["--kering"])` mengembalikan 0, keluaran (tangkap stdout) memuat empat baris urutan `K1S A`, `K1S B`, `K2S A`, `K2S B` dan kata `flash 1 kali`; tidak ada pembukaan port (patch `run_bench.Logger` agar melempar bila dipanggil).
  - `test_auto_attacker_port_terpakai_pesan_ramah`: patch `run_bench.Attacker` melempar `serial.SerialException("PermissionError")` -> `main(["--logger","X","--auto-attacker","COM99"])` mengembalikan kode non-nol dan mencetak pesan yang menyebut menutup serial monitor, tanpa jejak tumpukan.
- [ ] **Step 2: Jalankan, pastikan gagal**
  Run: `.venv-analisis/Scripts/python -B -m unittest tests.test_demo_sidang -v`
  Expected: 3 tes baru gagal, 10 lainnya lulus.
- [ ] **Step 3: Implementasi** di `tools/demo_sidang.py`. `main`: bila `--kering` cetak urutan dan hitung flash lalu return 0 sebelum membuka perangkat; selain itu `periksa_elf(["K1S","K2S"])` (pesan dicetak, return 2 bila ada), buka `run_bench.Logger(args.logger)`, bila `--auto-attacker` buka `run_bench.Attacker` (galat `serial.SerialException` -> pesan ramah, return 2) dan beri `on_start=lambda: at.send(mode)` (mode `a`/`b` sesuai skenario) lewat `on_start_factory`; lingkaran perintah membaca `input("demo> ")`. `run A|B` memakai arsitektur dari `host_id`; `sesi` membuat folder `data/demo/<YYYYmmdd_HHMMSS>/`, menulis `sesi.txt` (host, rshunt, offset, `CATATAN_STLINK`, hash ELF via `run_bench.sha_short`), memanggil `jalankan_sesi`, mencetak `tabel_ringkas` dan `CATATAN_STLINK`. `HostError` pada perintah tunggal dicetak sebagai pesan, bukan jejak tumpukan.
- [ ] **Step 4: Jalankan tes, pastikan lulus**
  Run: `.venv-analisis/Scripts/python -B -m unittest discover -s tests -v`
  Expected: seluruh tes demo (13) dan tes lama PASS.
- [ ] **Step 5: Commit** (hanya bila diminta): `tools/demo_sidang.py tests/test_demo_sidang.py`.

### Task 4: Uji bangku nyata dan panduan hari-H (BUTUH IZIN PENELITI)

**Files:**
- Create: `docs/demo_sidang_panduan.md`
- Modify: `docs/superpowers/specs/2026-11-19-demo-sidang-design.md` (tambah bagian "Hasil uji bangku")

**Interfaces:**
- Consumes: skrip dari Task 3; ELF `.pio/build/host_k1_stop/firmware.elf` dan `.pio/build/host_k2_stop/firmware.elf` (tidak di-build ulang tanpa izin; bila hilang, minta izin `pio run -e host_k1_stop -e host_k2_stop`).
- Produces: panduan satu halaman (daftar periksa hari-H) dan angka terukur (lama flash, hasil 20 run B).

- [ ] **Step 1: Minta izin dan konfirmasi perangkat** ke peneliti: nomor COM logger dan attacker hari itu, ST-Link host `$` terpasang, kabel logger->host PA10 tersambung, VS Code tidak sedang memegang port. Jalankan `ping` (`python tools/demo_sidang.py --logger COMxx`, perintah `ping`). Expected: `>ID,<K1S|K2S>,8,...,1`.
- [ ] **Step 2: Satu run A** (attacker diketik manual `a`), lalu `python tools/run_bench.py analyse data/demo/<sesi>/001_*.csv` pada CSV yang sama. Expected: angka ringkasan demo SAMA dengan keluaran `analyse`; `N_rusak=0`.
- [ ] **Step 3: Sesi penuh** (`sesi`): catat lama flash K1S->K2S dan total waktu dari Enter pertama sampai tabel. Expected: empat run selesai; total dalam 7-12 menit (spec); catat bila meleset.
- [ ] **Step 4: Pra-uji frame di tengah jalan:** 20 run mode B berturut-turut dengan attacker sudah berjalan sebelum jendela (alur demo). Expected: `N_rusak = 0` di seluruh 20 run. Bila ada rusak: catat persentase, aktifkan cadangan `--auto-attacker`, uji ulang 20 run B dengan cadangan itu, dan laporkan ke peneliti sebelum menyatakan alur manual layak.
- [ ] **Step 5: Tulis `docs/demo_sidang_panduan.md`** (bahasa Indonesia, maksimal satu halaman): urutan periksa sebelum penguji datang (flash ke K1S, `ping`, serial monitor attacker dibuka pada COM yang benar, ST-Link terpasang), perintah selama demo (`sesi`, `a`/`b`/`x`), apa yang dikatakan ke penguji tentang prosedur yang sedikit berbeda dan catatan ST-Link, dan langkah pemulihan (host tidak menjawab, run gagal, host terkunci -> `tools/pulihkan_host.py`). Tambahkan hasil Step 2-4 ke spec.
- [ ] **Step 6: Commit** (hanya bila diminta): panduan dan spec.

---

## Self-Review

- **Cakupan spec:** alur banner/jeda/jendela (Task 2); sesi empat run satu flash (Task 1-2); ringkasan + catatan ST-Link (Task 1, 3); hasil mentah di `data/demo` (Task 1, 3); `--auto-attacker` cadangan (Task 3); `--kering` (Task 3); kriteria selesai: dry-run (Task 3), sesi nyata + 20 run B + kecocokan dengan `analyse` (Task 4). Kolom pembanding paper dari `stop_final` SENGAJA ditunda (spec: "ditunda bila sulit"; keluaran `deskriptif.csv` berkelompok tanpa label skenario yang jelas) — masuk daftar tambahan setelah demo berjalan.
- **Konsistensi tipe:** `jalankan_run` -> `(bytes, dict, list[str])` dipakai `jalankan_sesi`; nama `HostTidakMenjawab`/`HostSalahArsitektur` sama di Task 2 dan 3; `URUTAN_SESI` dari Task 1.
- **Proporsi:** satu berkas skrip + satu berkas tes; badan fungsi tidak ditulis, hanya tanda tangan, tes, dan nilai spec.
