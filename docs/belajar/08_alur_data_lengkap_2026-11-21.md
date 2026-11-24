# 08. Alur data lengkap: dari sensor sampai kesimpulan (diperbarui 21 Nov 2026)

Bahan belajar tentang SELURUH perjalanan satu angka: kabel dan firmware yang merekamnya, kalibrasi, perhitungan per run,
statistik, sampai kalimat di kesimpulan. Setiap bagian memuat: apa yang terjadi, **kode mana** (berkas:baris), **rumus**,
**library**, dan contoh angka terbaru. Angka mutlak di sini memakai kalibrasi final 21 Nov (R_eff 0,0821 Ohm, offset -2,83,
u_kal 1,5 %), jadi berbeda dari `07_dari_csv_ke_kesimpulan.md` (angka lamanya memakai shunt 0,100 Ohm dan sekarang usang).
Nomor baris benar per 21 Nov 2026; bila kode berubah, cari nama fungsinya.

Cara membaca: kerjakan berurutan A -> G. Setiap bagian bisa dibaca sendiri; tiap bagian diakhiri "uji pemahaman".

---

## 0. Peta satu halaman

```
 [A] PENGAMBILAN DATA (bangku, 1 run = 60 s)                      [B] KALIBRASI (sekali, 21 Nov)
 -------------------------------------------                      -------------------------------
 PC run_bench.py --baud-trick--> LOGGER (F103, INA226, TIM2)      beban nol -> offset X0
                                    | USART1 "S60"                resistor 1k/2k2 -> selisih dX
                                    v                             R_eff = dX . 2,5 uV / (V_bus/R)
 ATTACKER --frame 36 B--> HOST (F103: K1S software | K2S DS28E38)  cek silang multimeter CD800a
   (TIM3, 1 atau 10 /s)      tidur Stop, bangun karena frame/RTC       |
                                    | PB1 (status CPU) -> logger PA0   |  parameter (R_eff, X0, u_kal)
                                    | USART1 ">START .. >END .. >L"    v
 logger menulis CSV mentah: s (INA226), e (tepi PB1), h (baris host), #SUM
        |
        v   data/runs/<sesi>/NNN_<ARCH>_<skenario>.csv  (139 berkas; 128 sah)
 [C] ANALISIS PER RUN  run_bench.analyse(text, rshunt, offset)  <-- parameter kalibrasi masuk DI SINI
        |  keutuhan (D) -> jendela t0..t1 -> S_cpu, T_obs, latensi, E, I, P -> N_dtg/N_lyn/TL
        v
 [E] PER SESI  energi_sesi(): P_idle dari run IDLE sesi itu -> P_dyn, I_dyn, E_lyn, Q_lyn
        v
 data/analisis/stop_final/runs.csv  (satu baris per run)
        |
 [F] STATISTIK  analisis_d.py: deskriptif, ketidakpastian, SW vs HW (bootstrap, Welch, Holm),
        |        barrage, interaksi DD, layanan, duty-cycle, Bland-Altman, leave-one-block-out
        +--> analisis_sensitivitas.py, dekomposisi_energi.py, proyeksi_baterai.py, gambar_bab4_stop.py
        v
 [G] KESIMPULAN  aturan keputusan (Holm < 0,05; DD IK tidak memuat 0; TL vs model <= 1 poin)
        --> ringkasan.md --> Tabel/Gambar Bab 5 --> Bab 6
```

**Tiga jalur data yang tidak boleh tertukar**
| Jalur | Dari | Satuan mentah | Dipakai untuk |
|---|---|---|---|
| Waktu | kristal 72 MHz logger (`e,<tik>`, `s,<tik>`) dan RTC 32,768 kHz host | tik | T_obs, S_cpu, E, durasi |
| Arus/tegangan | INA226 (`s,<tik>,<shunt>,<bus>`) | satuan register | I, V, P, E, Q |
| Layanan | host lewat USART1 (`>END`, `>L`) | hitungan, siklus | N_dtg, N_lyn, latensi, frame rusak/hilang |

Kelompok: **K1** software normal (K1S, 1/s), **K2** DS28E38 normal (K2S), **K3** software barrage (10/s), **K4** DS28E38 barrage.
"S" pada K1S/K2S = varian Stop (deep sleep). `analisis_d.py --stop` membuang huruf S (`analisis_d.py:147-149`).

---

## 1. Perangkat lunak dan library per lapisan

| Lapisan | Bahasa / alat | Library atau fasilitas | Dipakai di |
|---|---|---|---|
| Build firmware | PlatformIO (VS Code), `platform = ststm32`, `framework = arduino` (`platformio.ini:30-31`), core STM32duino | `Arduino.h`, `HardwareTimer`, `Wire` (I2C), `Serial1/2`, `SerialUSB`; akses register langsung (`GPIOB->BSRR`, `RTC->CRL`, `PWR->CR`, `DMA1_Channel6`) | semua firmware |
| Kripto software (K1S) | C++ | **Mbed TLS 3.6.7** (`lib/mbedtls`): `mbedtls_ecp_group_load`, `mbedtls_ecdsa_sign`, `mbedtls_hmac_drbg_*`, `mbedtls_sha256_*`; konfigurasi `include/mbedtls_k1_config.h` | `src/host/final/k1_stop/main.cpp` |
| Kripto hardware (K2S) | C++ | tanpa library: **driver 1-Wire buatan sendiri** (bit-banging PA15, overdrive) `src/host/final/bersama/ds28e38_od.h` | `k2_stop/main.cpp` |
| Sensor arus | C++ | I2C 400 kHz lewat `Wire`; driver `src/logger/modul/ina226.*` | logger |
| Protokol bersama | C++ header-only | `lib/bench_proto/proto_attacker_host.h` (frame 36 B), `proto_host_logger.h` (perintah dan balasan USART1) | attacker, host, logger |
| Pemrograman | Python + OpenOCD (`subprocess`) | `~/.platformio/packages/tool-openocd`, `tools/flash_stop.py`, `tools/pulihkan_host.py` | bangku |
| Kendali dan perekaman PC | Python 3 | **pyserial** (`serial.Serial`), `argparse`, `csv`, `hashlib`, `random`, `datetime`, `pathlib` | `tools/run_bench.py` |
| Statistik | Python 3, venv `.venv-analisis` | **NumPy** (bootstrap, persentil), **pandas** (tabel, groupby), **SciPy** `stats.ttest_ind` (Welch), `stats.mannwhitneyu`, `stats.t.ppf`, **statsmodels** `multipletests` (Holm) | `tools/analisis_d.py` |
| Gambar | Python 3 | **matplotlib** (backend Agg, 300 dpi) | `tools/gambar_bab4_stop.py`, `analisis_d.py` |
| Verifikasi independen | Python murni | `tools/p256.py` (ECDSA P-256 dari nol), `hashlib` | `tools/b1_verifikasi.py` |
| Desain PCB | KiCad 9 | ERC/DRC (PCB logger/attacker belum difabrikasi); berkas proyek KiCad tidak ada di repositori ini | (di luar repo) |
| Uji alat | Python `unittest` (hanya pustaka standar) | `.venv-analisis/Scripts/python -B -m unittest discover -s tests -v` | `tests/` |

Versi dikunci di `requirements-analisis.txt` (Python 3.12.10): **numpy 2.5.4, scipy 1.18.1, pandas 3.1.1, statsmodels 0.15.0, matplotlib 3.11.2, pyserial 3.5**,
supaya angka Bab 4/5 dapat direproduksi.

Lingkungan build yang dipakai data akhir: `host_k1_stop`, `host_k2_stop`, `logger`, `attacker` (`platformio.ini`). Mengubah
`platformio.ini` menghapus `.pio/build`; bangun ulang dulu sebelum flash. Firmware dibuktikan identik lewat sha256
`firmware.bin` (`data/manifes_firmware_stop.md`, `tools/cek_biner.py`).

---

## 2. Flow A: pengambilan data (satu run = satu berkas CSV)

### A0. Persiapan sebelum blok (keadaan bersih)
1. Bangun dan flash firmware host: K2S/K1S memakai `tools/flash_stop.py` (menangkap CPU saat terjaga karena SWD mati selama Stop;
   `run_bench.py:97-112`, `flash_host()`); firmware lain memakai OpenOCD `program ... verify reset exit`.
2. **POR penuh**: cabut USB-C, ST-Link, kabel sinyal dan GND 10 detik; pasang GND dulu, sinyal, USB-C terakhir. Alasan: sesi
   debugger menyalakan `DBGMCU_CR` yang menaikkan arus Stop ~0,58 mA pada shunt nominal (~0,71 mA terkalibrasi; lihat `docs/pinout_arsitektur_pengujian.md`);
   firmware menolkan `DBGMCU->CR = 0` sesudah jendela aman 5 s (`k1_stop/main.cpp:168`).
3. Urutan run direncanakan oleh `run_bench.py plan` (`run_bench.py:531-562`): acak urutan skenario dalam blok (`random.Random(seed)`),
   **IDLE di awal dan akhir blok** untuk P_idle dan drift. Empat blok pola ABBA: K2S | K1S | K1S | K2S (`data/tahap_stop.txt`).

### A1. PC memerintah logger tanpa jalur tulis USB (trik baud)
Jalur OUT USB logger (klon F103, UID nol) rusak, jadi perintah dikirim sebagai **nilai baud** (`SET_LINE_CODING`):

`baud = 1.000.000 + perintah x 1000 + argumen` (`run_bench.py:142-165`, `Logger.command`), lalu baud dikembalikan ke 115200.

| Perintah | Arti | Argumen |
|---|---|---|
| 3 | tanya identitas host (`?`) | - |
| 4 | jendela dengan permintaan NYATA attacker (`S<detik>`) | detik (60) |
| 7 | rekam saja tanpa host (dipakai kalibrasi) | detik |

Logger membaca `linecoding.bitrate` di `loop()` dan memanggil `exec_cmd` (`src/logger/f103/main.cpp:525-546`).
PC membaca aliran teks sampai baris `#CMD,SELESAI`. Library: **pyserial**.

### A2. Logger menjalankan run (`run_capture`, `logger/f103/main.cpp:318-460`)
Mesin keadaan: `PRE (100 ms rekam dulu) -> WAIT_START -> WINDOW -> POST -> END`.
1. `ina.init(CFG_CONT_1P1MS = 0x4127, CAL 0x0800)` (`ina226.cpp:26`): INA226 mode kontinu, konversi shunt dan bus **1,1 ms** masing-masing,
   tanpa perataan -> laju nyata **495,75 cuplikan/detik** (sifat modul, bukan MCU).
2. Tiap iterasi: polling flag `CVRF` (`ina.ready`) -> `readShuntBus` -> tulis `s,<tik>,<shunt>,<bus>` (baris 349-371).
   `<tik>` = waktu **awal pembacaan MASK yang menemukan flag**, dari timer 72 MHz (`now_ticks()`).
3. ISR input-capture TIM2 (PA0) mengisi cincin `ering` (128 entri) -> baris `e,<tik>,<1|0>` (baris 382-391). Tepi naik = host terjaga,
   tepi turun = host masuk Stop (PB1 host -> PA0 logger).
4. Baris dari host di USART1 diteruskan apa adanya sebagai `h,<tik>,<isi>` (baris 395-413). `>START`, `>END`, `>DONE` menggerakkan mesin keadaan.
5. Di akhir: `#SUM,...` (jumlah cuplikan, `late`, `dt_max`, galat I2C, jumlah dan checksum shunt/bus, jumlah tepi dan baris) dan
   `#RUN,END,<no>,<status>` (baris 440-459). **Checksum ini yang membuat PC bisa mendeteksi baris USB yang hilang.**

Pemulihan bus I2C: bila 3 baca gagal beruntun, `ina.recover()` (`ina226.cpp:57`).

### A3. Logger -> host -> run
Logger mengirim `S60` lewat USART1 (`main.cpp:420`). Host (`stop_link.h:168-199`, `hl_service`) memeriksa `HL_READY`, membalas
`>START,S60`, menunggu byte terakhir terkirim (`hl_tx_flush`, supaya t0 jendela setelah baris ini), lalu `cmd_window(60,...)`.
PC yang melihat `>START` di aliran langsung menyalakan attacker (`run_bench.py:471-475`: `a` = 1/s, `b` = 10/s).

### A4. Attacker membangkitkan permintaan (`src/attacker/main.cpp`)
- Frame 36 byte: `0xA5 | seq(2, little-endian) | 32 byte isi | XOR byte 0..34` (`buildFrame`, baris 64-74; format di `proto_attacker_host.h`).
- Pewaktu perangkat keras **TIM3** (`HardwareTimer`, baris 45, 86-92): 1 Hz (A) atau 10 Hz (B); ISR hanya menaikkan penanda
  (`onTick`, baris 77-84), pengiriman di `loop()`. Tick palsu pertama dibuang (`g_skipFirstTick`).
- Konsol CH340 (USART2): `a`, `b`, `x`, `s` (status), `f`. Attacker **tidak** diubah untuk varian Stop.

### A5. Host: mesin jendela dan tidur Stop (`src/host/final/bersama/stop_window.h`, `stop_common.h`)
`cmd_window` (`stop_window.h:90-146`):
1. `g_win_end_cnt = rtc_cnt() + secs x 1024` (RTC diprescale 1024 Hz, `RTC_PRL_VAL=31`); alarm RTC dipasang (`alarm_rearm`, baris 27-41).
2. `f0 = rtc_fine()` (1/32768 s); `PB1_HI()`; penerima frame dinyalakan.
3. Loop: bila `g_pending` ada permintaan -> `serve_one(&lat)`, `served++`, `hl_lat_add(lat)`; bila tidak -> `wait_event(true)` yang
   memanggil `stop_sleep()` (tidur Stop sungguhan, `stop_common.h:297`).
4. Di akhir: `PB1_LO()` (tepi turun = akhir jendela bagi logger), `t_obs = fine_to_us(f1-f0)`, `t_sleep = g_t_sleep`,
   `hl_clk_hz = measure_hclk()` (HCLK diukur terhadap RTC 2 s) lalu latensi dikonversi (baris 142-145).

**Penerimaan frame saat CPU tidur** (kunci desain): UART mati di Stop, jadi awal frame yang membangunkan CPU terlewat. Host mengenali
pola tepi byte pertama `0xA5` (sinkron bit di HSI 8 MHz, `SYNC_MIN_US=40`..`SYNC_MAX_US=85`), lalu menyalakan penerima ~17 us sebelum byte
kedua; DMA mengisi 36 byte (`stop_common.h:157-296`). `frame_valid()` memeriksa SOF dan XOR (baris 216); `frame_commit()` menghitung
`rx_lost` dari loncatan nomor urut (baris 238-250). **Frame rusak harus 0; frame hilang < 0,25 % pada barrage** (aturan keabsahan, D).

### A6. Host melayani satu permintaan
| Arsitektur | Kode | Isi | Diukur oleh |
|---|---|---|---|
| **K1S** software | `k1_stop/main.cpp:148-155 serve_one` -> `one_operation` (119-130) -> `sign_digest` (81-108) | entropi `harvest_entropy` (di luar ukur) -> HMAC-DRBG hedged -> `mbedtls_ecdsa_sign` (NIST P-256) | siklus **DWT CYCCNT** (`g_op_cyc`) |
| **K2S** DS28E38 | `k2_stop/main.cpp:70-104 serve_one` -> `ds_sign` (`ds28e38_od.h:208`) -> `xpc(0xA5,...)` | perintah *Compute and Read Page Authentication*: kirim 3,4 ms (aktif), **tidur Stop ~100 ms** (`wait_chip_ms`, `stop_window.h:71`; DQ ditahan tinggi oleh `ow_hold`, daya parasit), polling per bit sampai chip siap (~7 ms), baca 64 byte (5,6 ms) | **RTC** (`STOP_LAT_RTC`), karena DWT berhenti di Stop |

Rumus latensi (paper 3.1, 3.2): software `T_sign = (N_end - N_start) / f_CPU x 1000`; hardware `T_sign = (F_akhir - F_awal) / 32768 x 1000`.
Frekuensi inti HSI 8 MHz -> PLL 48 MHz menyimpang ~-0,5 %, karena itu `f_CPU` diukur (`measure_hclk`) dan dilaporkan di `>END` kolom `clk_hz`.

### A7. Host mengirim hasil
`hl_send_result()` (`stop_link.h:129-163`): `>END,<23 kolom>`, `>L,<latensi per tanda tangan, 10 per baris>`, (`>SIG` bila build diagnostik), `>DONE`.
Urutan kolom `>END` = `END_COLS` di `run_bench.py:65`: `arch, secs, p_ms, uart, t_obs_us, t_sleep_us, act_us, resid_us, arrived, served, gagal,
wfi, spurious, pin_salah, rx_ok, rx_bad, rx_lost, lat_n, lat_min, lat_mean, lat_max, lat_drop, clk_hz`.
Tidak ada transmisi serial dari host selama jendela, supaya tidak mengganggu pengukuran arus.

### A8. PC menyimpan berkas
`do_run` (`run_bench.py:466-485`) menulis `raw` ke `data/runs/<sesi>/NNN_<ARCH>_<skenario>.csv` dan memanggil `analyse` seketika.
`sesi.txt` mencatat host, seed, rencana, `rshunt`, `offset`, dan sha256 ELF/bin (`run_bench.py:579-586`; catatan B4).

**Isi satu berkas nyata** (`data/runs/20261118_115044_K2S/033_K2S_A.csv`):
```
#CMD,4,60            #RUN,BEGIN,213,4,60,72000000,4C364231     (perintah, detik, tik timer, build logger)
s,112502,497,4170    <- tik 112502; shunt mentah 497; bus mentah 4170
e,7316164,1          <- tepi naik PB1 pada tik 7316164 (host bangun)
h,7321278,>START,S60
h,4472196532,>END,K2S,60,1000,1,59999572,58910048,1089524,0,59,59,0,119,0,0,59,0,0,59,5499997,5546199,5569968,0,47766741
#SUM,213,4475324406,30813,0,154300,0,0,0,3492113,128589077,240,0,0,9,0     #RUN,END,213,OK
```
Baca `>END`: `t_obs_us=59.999.572`, `t_sleep_us=58.910.048`, `arrived=59`, `served=59`, `rx_bad=0`, `rx_lost=0`, `lat_mean=5.546.199` siklus pada
`clk_hz=47.766.741` -> **116,11 ms**.

### A9. Verifikasi independen yang memastikan data bukan sampah
| Pemeriksaan | Skrip | Apa |
|---|---|---|
| Tanda tangan firmware sah | `tools/b1_verifikasi.py` + `tools/p256.py` | build diagnostik (`-DIAG_SIG`) mengirim r||s 30 tanda tangan; PC memverifikasi ECDSA P-256 sendiri (tanpa Mbed TLS) dan menolak tanda tangan yang dirusak (`s xor 1`) -> 30/30 |
| Jalur galat | `tools/uji_galat_stop.py` | attacker mati/hidup/bolak-balik saat jendela berjalan; host tidak boleh crash |
| Hitung ulang satu run | `tools/hitung_ulang_satu_run.py` | menghitung ulang run dari CSV mentah tanpa `run_bench.py`; harus sama sampai 6 desimal |
| Ketertelusuran firmware | `tools/cek_biner.py`, `data/manifes_firmware_stop.md` | sha256 `firmware.bin` final |

**Uji pemahaman A**: (1) Mengapa PB1 diberi dua peran (S_cpu dan penanda jendela)? (2) Mengapa `>START` dikirim tuntas sebelum t0?
(3) Mengapa latensi K2S memakai RTC dan K1S memakai DWT? (4) Apa akibatnya bila checksum `sh_sum` tidak cocok?

---

## 3. Flow B: kalibrasi INA226 (21 Nov 2026)

### B1. Mengapa kalibrasi, dan di mana ia masuk
INA226 membaca **tegangan** di shunt (register `01h`, 2,5 uV/satuan) dan tegangan bus (register `02h`, 1,25 mV/satuan). Logger **tidak** memakai register arus
internal (`CAL=0x0800` hanya untuk register itu); CSV menyimpan angka mentah. Arus dihitung di PC:

```
I (mA) = (X - X0) x 2,5e-6 V / R_eff x 1000        V (V) = bus x 1,25e-3
```
`run_bench.py:410-412` (`conv`). Karena CSV menyimpan angka mentah, kalibrasi bisa diganti **tanpa mengulang pengukuran**: cukup
`--rshunt` dan `--offset` pada `analisis_d.py`. **Sejak 21 Nov 2026 default baris perintah = hasil kalibrasi** (`R_eff 0,0821`, offset `-2,83`, `u_rel 0,015`), diambil dari satu modul
`tools/kalibrasi_param.py` (konstanta `RSHUNT_EFEKTIF`, `OFFSET_LSB`, `U_REL_KAL`; nominal lama `RSHUNT_NOMINAL=0,100`, `OFFSET_NOMINAL=-2,0`). Default **fungsi** `run_bench.analyse(text, rshunt=0.100, offset=-2.0)` sengaja tetap nominal:
uji `tests/test_metrik.py` menghitung nilai harapan dengan tangan pada 0,100 Ohm. Mereproduksi hasil lama: `--rshunt 0.100 --offset -2.0 --u-rel 0.05`.

### B2. Metode (kode: `tools/kalibrasi_shunt.py`)
| Langkah | Perintah | Kode | Rumus |
|---|---|---|---|
| 1. Offset nol | `kalibrasi_shunt.py offset --detik 60` (IN- dilepas; V_bus tetap ada) | `cmd_offset` (76) | `X0 = rerata(X)` pada beban nol |
| 2. Baseline dan resistor | `titik --kelompok terjaga --nama B0` / `--nama R1k_1 --rload 1000` | `cmd_titik` (89), `rekam` (40: perintah logger 7) | rerata X, V_bus tiap rekaman 30 s |
| 3. Selisih | `kalibrasi_shunt.py selisih [--hanya-r 1000]` | `hitung_selisih` (113), `cmd_selisih` (145) | `dX = X_resistor - rerata(X_baseline sebelum, sesudah)`; `dI = V_bus / R_nominal`; **`R_eff = dX x 2,5 uV / dI`** |
| 4. Silang | multimeter SANWA CD800a (40 mA, +-(2,2 % + 5 digit)) | `catatan_multimeter_2026-11-21.md` | regresi arus absolut melalui nol (I dalam ampere): `R_eff = 2,5 uV x sum(X^2) / sum(X.I)` (`cmd_hitung`, 213) |
| 5. Ketidakpastian | `selisih` | `cmd_selisih` | `u_kal = akar(toleransi_R^2 + u_Vbus^2 + (sd/akar n)^2)` = 1,2 %, ditetapkan **1,5 %** |

Mengapa **selisih**: offset X0 hilang (hanya selisih dua rekaman), drift hilang (baseline mengapit B-R-B), dan acuan adalah V_bus/R dengan resistor 1 %
(lebih teliti daripada mengukur resistansi dengan multimeter yang tersedia, +-1,5 % + 5 digit). Resistor dipasang antara IN- dan GND logger,
**sisi beban** (supaya arusnya melewati shunt). Metode ini tidak butuh akurasi absolut multimeter.

Hasil (21 Nov, host K2S terjaga, 12-21 mA): 1 kOhm 0,08246 (sebaran 0,25 %), 2,2 kOhm 0,08170 (sebaran 2,30 %, kriteria < 1 % gagal, dilaporkan apa adanya),
rerata 6 selisih **0,0821 Ohm**; regresi multimeter 0,0833 Ohm (beda 1,4 %); offset awal -2,820, akhir -2,833 satuan.
Detail: `data/kalibrasi/kalibrasi_final_2026-11-21.md`.

### B3. Dampak dan aturan konversi
| Besaran | Efek kalibrasi |
|---|---|
| arus, daya, energi, muatan mutlak | x 1,218 (dan offset): `I_baru = 1,218 x I_lama + 0,025 mA` |
| arus dinamis, P_dyn, E_lyn, Q_lyn (sudah dikurangi IDLE) | x 1,218 saja |
| latensi, S_cpu, TL, throughput, p, **persentase** penghematan | tidak berubah (gain sama di kedua arsitektur; `u_kal` saling menghapus) |
| lantai IDLE | 1,99 -> **2,45 mA** (I_idle rerata 2,449 mA; 12,78 mW) |

Keterbatasan yang wajib disebut: kalibrasi hanya pada 12-21 mA (kelompok Stop tidak dikalibrasi; linear sampai ~2 mA diasumsikan), dilakukan 3 hari sesudah data (18 Nov),
sebaran 2,2 kOhm 2,3 %, pembanding multimeter hanya tiga titik.

**Uji pemahaman B**: (1) Tunjukkan bahwa offset hilang pada `dX`. (2) Dari X mentah 508 pada beban nol host, hitung I dengan 0,100 dan 0,0821 Ohm. (3) Mengapa `u_kal` tidak dipakai pada gain %?

---

## 4. Flow C: analisis per run (`run_bench.analyse`, `tools/run_bench.py:321-448`)

Satu berkas CSV -> satu baris `runs.csv`. Urutan di dalam fungsi:

| # | Langkah | Baris | Rumus / aturan |
|---|---|---|---|
| C1 | `parse(text)` memecah baris `s`, `e`, `h`, `#SUM`, `#RUN` | 198-231 | `>END` -> dict per `END_COLS` |
| C2 | Keutuhan aliran USB | 331-340 | `len(s)==sum.n`; `sum(shunt)==sh_sum`; `sum(bus)==bus_sum`; `len(e)==e_lines`; `i2c_err, rec, er_drop, seq_err, h_ovf == 0` |
| C3 | Aturan *late ringan* | 341-350 | selang cuplik terbesar < 5000 us dan <= 10 cuplikan terlambat -> sah (celah diisi interpolasi) |
| C4 | Jendela menurut **logger** | 357-367 | `t0` = tepi naik PB1 pertama setelah `>START` (kelonggaran 2 ms); `t1` = tepi turun terakhir sebelum `>END`; **`T_obs = (t1-t0)/72e6`** |
| C5 | **S_cpu** | 368-381 | `S_cpu_log = (jumlah lama PB1 rendah) / (t1-t0) x 100`; `S_cpu_host = t_sleep / t_obs x 100`; `clk_ppm = (T_host/T_log - 1) x 1e6` |
| C6 | Konsistensi | 387-398 | `|S_log - S_host| <= 0,5` poin; `|resid| <= 2 ms`; rusak = 0; hilang hanya pada barrage Stop dan `< 0,25 %` dari yang tiba (`lost_ok`); `gagal, pin_salah, lat_drop = 0` |
| C7 | **Latensi** | 400-407 | `lat_ms = lat_mean x 1000 / clk_hz`; `lat_ms_nyata = lat_ms x T_log / T_host` (koreksi jam host) |
| C8 | Layanan | `layanan()` 234-253 | lihat C10 |
| C9 | **Energi** | 409-431 | `I_k = (X_k - X0) x 2,5e-6 / R_eff`; `V_k = bus x 1,25e-3`; interpolasi linear di batas `t0,t1`; **`E = sum 1/2 (V_k I_k + V_k-1 I_k-1) dt`** (trapesium); `P_avg = E/T`; `I_avg = Q/T` |
| C10 | Aktif vs tidur | 434-447 | arus rata-rata saat PB1 tinggi (`I_aktif`) dan rendah (`I_tidur`); model `I_model = I_aktif D + I_tidur (1-D)`, `D = 1 - S_cpu/100` |

**Metrik layanan** (`run_bench.py:234-253`, paper 3.5.1 dan 3.5.2):
`N_lwt = N_dtg - N_lyn - N_ggl`; `TL = N_lyn / N_dtg x 100` (penyebut = yang terukur, mis. 59); `Theta = N_lyn / T_obs`; `lambda_eff = N_dtg / T_obs`;
`rho = lambda_eff x t_sign`; **`TL_model = min(1, 1/rho) x 100`** (hukum utilisasi U = X.S, tanpa antrean); `Theta_max = 1/t_sign`.

**Contoh dengan tangan (run K2S normal di atas, kalibrasi baru)**
- Cuplikan pertama: `s,112502,497,4170` -> `I = (497 + 2,83) x 2,5e-6 / 0,0821 = 15,22 mA`, `V = 4170 x 1,25e-3 = 5,2125 V`, `P = 79,3 mW` (host terjaga).
- `S_cpu_host = 58.910.048 / 59.999.572 = 98,184 %`; `S_cpu_log = 98,174 %` (selisih 0,011 poin).
- `T_obs_log = 60,00242 s`; `I_avg = 3,1368 mA`; `P_avg = 16,3617 mW`; `E = 981,74 mJ`.
- `P_idle` sesi ini = 12,7657 mW -> **`P_dyn = 16,3617 - 12,7657 = 3,5960 mW`**; **`E_lyn = 3,5960 x 60,00242 / 59 = 3,657 mJ`**.
- `TL = 59/59 = 100 %`; `rho = (59/60,00242) x 0,11612 = 0,114` -> `TL_model = 100 %`.

Periksa sendiri: `python tools/hitung_ulang_satu_run.py data/runs/20261118_115044_K2S/033_K2S_A.csv --rshunt 0.0821 --offset -2.83 --p-idle 12.766 --i-idle 2.449`.

---

## 5. Flow D: keabsahan, sesi, dan manifest

- **Keabsahan satu run**: `analyse` mengisi daftar `masalah`; kosong -> `sah=True` (`analisis_d.py:150`). 139 run di 4 sesi, **128 sah** (4 x 30 + 8 IDLE), **11 gugur** (semua K4, 2-4 frame UART hilang dari 598-600).
- **Pemeriksaan tingkat data** `pemeriksaan()` (`analisis_d.py:211-287`): 30 run sah per kelompok; IDLE sah di tiap blok; drift IDLE awal-akhir `<= 0,05 mA` (OK) / `<= 0,10` (PERIKSA); `S_cpu` log vs host `<= 0,5` poin;
  frame rusak = 0. Hasil: "MEMENUHI STANDAR". Drift IDLE terbesar kini **0,0054 mA** (blok 4), IDLE 2,442-2,451 mA.
- **Pencilan**: pagar Tukey 3 x IQR, **hanya dilaporkan, tidak dibuang**.
- **Manifest** `data/tahap_stop.txt` (4 baris: blok -> nama sesi) menentukan data apa yang dianalisis (`baca_manifest`, `analisis_d.py:79`).

## 6. Flow E: per sesi, IDLE sebagai nol (`energi_sesi`, `run_bench.py:256-284`)

`P_idle` = rerata `P_avg` run IDLE (sah) **arsitektur dan sesi yang sama** (mengapit blok), sehingga pergeseran antar-sesi (tegangan USB, suhu) ikut terkoreksi.
`P_dyn = P_avg - P_idle`; `E_dyn = P_dyn x T_obs`; **`E_lyn = E_dyn / N_lyn`**; `I_dyn = I_avg - I_idle`; **`Q_lyn = I_dyn x T_obs / N_lyn`** (muatan; tahan terhadap naik-turun tegangan USB).
Konsekuensi: P_dyn dan E_lyn hanya sah dalam satu sesi/varian dan tidak boleh dibandingkan lintas varian.

---

## 7. Flow F: analisis statistik, satu per satu (`tools/analisis_d.py`)

`muat()` (122-152) memproses semua CSV sesi, memanggil `analyse` + `energi_sesi`, dan menghasilkan `df`; `sah = df[sah & kelompok in K1..K4]` (30 run per kelompok).
Bootstrap memakai `numpy.random.default_rng(20261116)`, 10.000 resampel (`N_BOOT`).

| Bagian | Fungsi (baris) | Input -> proses -> keluaran | Rumus |
|---|---|---|---|
| **F1 Deskriptif** | `deskriptif` (171) | tiap kelompok x metrik -> `deskriptif.csv` | `rerata`, `sd (ddof=1)`, `CV = sd/rerata`, `IK95 = rerata +- t(0,975; n-1) sd/akar n` (`scipy.stats.t.ppf`) |
| **F2 Ketidakpastian** | `ketidakpastian` (291) -> `ketidakpastian.csv` | empat komponen | `u_acak` = setengah lebar IK; **`u_kal = u_rel x |rerata|`** (u_rel = 0,015; hanya metrik energi, kena `ENERGI`); `u_blok` = sd antar-rerata-blok; `u_idle` = drift IDLE terbesar (x V, x T/N bila perlu); `U_sis = akar(u_kal^2 + u_blok^2 + u_idle^2)`; **`U_gab = akar(U_sis^2 + u_acak^2)`** (JCGM) |
| **F3 SW vs HW** | `banding_sw_hw` (329) -> `banding_sw_hw.csv` | K1-K2 (Normal), K3-K4 (Barrage), tiap metrik | `delta = SW - HW` (atau `HW - SW` untuk S_cpu; `delta_hw_baik`, 196); **gain % = delta / rerata_SW x 100**; IK delta dan gain dari bootstrap persentil 2,5/97,5; **Welch** `scipy.stats.ttest_ind(equal_var=False)`; **Mann-Whitney** sebagai cek; **Holm** `statsmodels multipletests(method="holm")` atas 6 uji utama (lat, P_dyn, S_cpu x 2 skenario) |
| **F4 Pengaruh barrage (RM2)** | `pengaruh_barrage` (372) -> `pengaruh_barrage.csv` | per arsitektur: barrage - normal | `selisih = rerata_barrage - rerata_normal`, IK bootstrap, `relatif % = selisih/normal` |
| **F5 Interaksi (RM3)** | `interaksi` (385) -> `interaksi_dd.csv` | DD | **`DD = Delta_barrage - Delta_normal`** (Delta positif = HW lebih baik); IK bootstrap; tafsiran otomatis: IK memuat 0 -> "tidak terbukti berbeda"; `|DD| < 1 %` -> "praktis sama" |
| **F6 Layanan (H4)** | `tabel_layanan` (406) -> `layanan.csv` | per kelompok | rerata TL, TL_model, `rho`, `Theta`, `Theta_maks_model` |
| **F7 Duty cycle** | `model_duty_per_run` (420) -> `model_duty_per_run.csv` | konsistensi arus vs pin | `I_model - I_avg` per run (selisih maks 0,0096 mA) |
| **F8 Bland-Altman** | `bland_altman` (201) -> `bland_altman.csv` (**mode Stop: hanya baris S_cpu**, lihat jebakan 7) | S_cpu pin vs firmware (120 run); latensi pin vs DWT/RTC per tanda tangan (`episode_pin`, 89-119 -> `latensi_pin_vs_dwt.csv`) | `d = y - x`; bias = rerata d; **batas kesesuaian = bias +- 1,96 sd(d)** |
| **F9 Leave-one-block-out** | `leave_one_block_out` (430) -> `leave_one_block_out.csv` | hitung ulang gain tanpa tiap blok | rentang gain (ketahanan terhadap drift antar-blok) |
| **F10 Pencilan** | `pemeriksaan` bagian 5 | Tukey 3 x IQR | dilaporkan, tidak dibuang |
| **F11 Sensitivitas ambang** | `tools/analisis_sensitivitas.py` (**tanpa argparse; membaca `stop_final/runs.csv`**) | K4 dianalisis ulang pada ambang frame hilang 0; 0,15; 0,25; 0,35; 0,5; 1,0 % (17-41 run) dan K3 vs K4 dengan 41 run | Welch dihitung manual dengan `log10 p` (`welch`, baris 25-32) agar tidak jatuh ke 0; teks "< 1e-100" |
| **F12 Dekomposisi energi** | `tools/dekomposisi_energi.py` (`fase_run`, 28-75) | CSV mentah run K1/K2 normal | episode = pulsa PB1 tinggi (>= 1 ms) dipisah jeda < 200 ms; tiap cuplikan dikelompokkan **aktif** (PB1=1) / **menunggu** (PB1=0 di dalam episode; K2 = IC menghitung) / **tidur**; `E_dyn,fase = (E_fase - P_idle x dt_fase)`, dibagi N_lyn; jumlah fase harus = `E_lyn` utama (selisih 0,00 %) |
| **F13 Proyeksi baterai** | `tools/proyeksi_baterai.py` | `runs.csv` | `E_baterai = 1000 mAh x 3,7 V x 3600 = 13,32 kJ`; tanda tangan/baterai = `E_baterai/E_lyn`; umur dengan lantai = `1000/I_avg`; tanpa lantai = `1000/I_dyn` (ilustrasi, asumsi disetujui 20 Nov) |
| **F14 Gambar** | `tools/gambar_bab4_stop.py` (matplotlib), `analisis_d.gambar` | `deskriptif`, `ketidakpastian`, `layanan`, `interaksi`, CSV mentah | warna: Software biru, DS28E38 oranye, barrage berarsir, desimal koma, 300 dpi; batang galat = `U_gab` |

**Angka kunci terbaru (kalibrasi 21 Nov; `data/analisis/stop_final/`)**
| Metrik | K1 SW normal | K2 HW normal | K3 SW barrage | K4 HW barrage |
|---|---|---|---|---|
| Latensi (ms) | 654,88 | 116,23 | 654,91 | 116,30 |
| S_cpu tidur (%) | 35,08 | 98,17 | 0,38 | 83,97 |
| P_dyn (mW) | 48,85 | 3,60 | 74,52 | 31,30 |
| E_lyn (mJ/tanda tangan) | 49,98 | 3,67 | 49,44 | 3,65 |
| TL (%) | 100 | 100 | 15,04 | 85,50 |
| I_idle rerata | 2,449 mA (12,78 mW) | | | |

- Gain: latensi 82,25 % / 82,24 %; P_dyn 92,62 % [92,61; 92,64] / 58,00 % [57,97; 58,02]; E_lyn 92,67 % / 92,61 %.
- DD: S_cpu +20,51 [20,50; 20,52]; P_dyn -2,02 [-2,06; -1,99] mW; latensi -0,04 [-0,10; 0,02] (tidak terbukti).
- Welch dengan Holm: keenam `p_holm` < 1e-100 (yang terbesar 7e-103, sehingga ditulis "< 10^-100" di naskah); Mann-Whitney 3,0e-11 (nilai minimum untuk 30 vs 30 tanpa tumpang tindih).
- `rerata_sw / U_gab` terkecil = **66,6** (aturan persen layak: >= 3 terpenuhi).
- Dekomposisi K2: CPU aktif 18,6 ms/15,9 mA/1,30 mJ (35,5 %); **IC menghitung 100,9 ms/6,80 mA/2,29 mJ (62,4 %)**; tidur 0,075 mJ (2,0 %). K1: CPU aktif 99,9 %.

**Uji pemahaman F**: (1) Hitung gain P_dyn normal dari 48,85 dan 3,60. (2) Mengapa p-value sampai 1e-113 dan apa yang ditekankan sebagai gantinya? (3) Jelaskan `U_gab` K1 P_dyn = 0,73 mW (1,50 %): komponen mana yang dominan?
(4) Mengapa DD latensi tidak berarti? (5) Untuk apa leave-one-block-out?

---

## 8. Flow G: dari angka ke kesimpulan

### G1. Aturan keputusan (yang diputuskan KODE vs yang diputuskan MANUSIA)
| Keputusan | Aturan | Diputuskan oleh |
|---|---|---|
| H0a/H0b/H0c ditolak | `p_holm < 0,05` (`analisis_d.py:366-367`, kolom `keputusan`) | kode |
| Interaksi didukung | IK bootstrap DD tidak memuat 0 (`interaksi`, 395-400) | kode |
| H4 didukung | `TL_HW = TL_SW = 100 %` pada normal; `TL_HW > TL_SW` pada barrage; `|TL - TL_model| <= 1` poin (`layanan.csv`: -0,20 dan -0,33 poin) | kode + naskah (3.4.7) |
| Persentase boleh dilaporkan | `rerata_SW / U_gab >= 3` (`persen_layak`) | kode |
| Data lulus standar | `pemeriksaan()` -> "MEMENUHI STANDAR" | kode |
| Ambang frame hilang < 0,25 % | ditetapkan SETELAH melihat penyebab (peneliti); diuji sensitivitasnya (F11) | **manusia** (dilaporkan terbuka) |
| u_kal = 1,5 % | dari kalibrasi + penutup selisih 0,0821 vs 0,0833 Ohm | **manusia** |
| Data skripsi = varian Stop; Shapiro/ANOVA tidak dikerjakan; comb/varian Mbed TLS tidak dilaporkan; asumsi baterai 1000 mAh/3,7 V | keputusan 18-20 Nov | **manusia** |

### G2. Peta klaim -> angka -> berkas -> fungsi
| Klaim di naskah | Angka | Berkas dan kolom | Dihitung oleh |
|---|---|---|---|
| DS28E38 82,3 % lebih cepat (5,6x) | 654,88 vs 116,23 ms; rasio 0,1775 | `banding_sw_hw.csv` (Normal, `lat_mean_ms_nyata`, `G_pct`, `rasio_hw_sw`) | `analyse` 400-407; `banding_sw_hw` |
| Energi per layanan hemat 92,6 % (13,6x) | 49,98 vs 3,67 mJ | idem, `E_lyn_mJ` | `energi_sesi` 283; `banding_sw_hw` |
| Daya dinamis normal -92,6 %, barrage -58,0 % | 48,85 -> 3,60 mW; 74,52 -> 31,30 mW | idem, `P_dyn_mW` | `energi_sesi` 275 |
| Barrage: software hampir tak tidur | S_cpu 0,38 % vs 83,97 % | `deskriptif.csv`, `S_cpu_log` | `analyse` 368-381 |
| TL barrage 15,0 % vs 85,5 % | model 15,24 / 85,83 % | `layanan.csv` | `layanan()` 245-251 |
| DS28E38 mengerjakan 8,7x lebih banyak saat barrage | Theta 8,565 vs 0,983 | `layanan.csv`, `Theta` | `layanan()` 243 |
| Interaksi tidur +20,5 poin | 63,08 -> 83,59 | `interaksi_dd.csv` | `interaksi` 385-403 |
| Latensi tidak berubah oleh serangan | 0,03 ms (SW), 0,07 ms (HW) | `pengaruh_barrage.csv` | `pengaruh_barrage` 372 |
| Kesimpulan tidak bergantung ambang frame hilang | 0 % -> 1 %: P_dyn 31,31 -> 31,28 mW; TL 85,46 -> 85,53 % | `sensitivitas/toleransi_k4.csv`, `banding_k4_semua.csv` | `analisis_sensitivitas.py` |
| Energi IC menghitung 62 % dari K2 | 2,29 dari 3,67 mJ | `dekomposisi_energi.csv` | `dekomposisi_energi.py` |
| DS28E38 memperpanjang umur 3,8x (319 vs 85 jam, lantai 2,45 mA) | 13,6x tanda tangan/baterai | `proyeksi_baterai.csv` | `proyeksi_baterai.py` |
| Dua pengukur tidur sepakat | bias -0,014 poin, batas -0,055..+0,026 | `bland_altman.csv` | `bland_altman` 201 |
| Nilai mutlak mA/mW/mJ | R_eff 0,0821; u 1,5 % | `data/kalibrasi/kalibrasi_final_2026-11-21.md` | `kalibrasi_shunt.py` |

Arti persen: gain % = (SW - HW)/SW x 100 untuk metrik yang lebih kecil lebih baik; S_cpu dan TL dilaporkan sebagai **selisih poin persentase**.

### G3. Dari tabel ke naskah
`ringkasan.md` (dibuat `analisis_d.main` 596-675) merangkum semua tabel; Tabel dan Gambar Bab 5 diambil dari sana. Daftar perubahan naskah akibat kalibrasi:
`laporan/laporan_perubahan_skripsi_v4_kalibrasi_2026-11-21.md`.

### G4. Keterbatasan yang menentukan kalimat kesimpulan
Satu papan dan satu IC; K2S mencampur overdrive + polling + Stop; kalibrasi hanya 12-21 mA; lantai LED papan 2,45 mA (daya tingkat papan, bukan cip);
batas frame hilang ditetapkan setelah data terlihat; waktu tunggu antrean tidak diukur; keabsahan tanda tangan per run tidak diperiksa (diverifikasi lewat build diagnostik).
Kalimat "hardware offloading **mengurangi** dampak, **tidak mencegah**" muncul karena TL barrage K4 hanya 85,5 % (rho = 1,165).

---

## 9. Menjalankan ulang seluruh rantai (urutan wajib)

Parameter final: `R_eff = 0,0821`, `offset = -2,83`, `u_rel = 0,015` (sudah menjadi **default** semua skrip di bawah, jadi tidak perlu diketik; di sini ditulis eksplisit supaya terlihat).
Jalankan dari akar repo dengan `.venv-analisis/Scripts/python`. Verifikasi 21 Nov: `analisis_d.py --manifest data/tahap_stop.txt --stop` TANPA argumen kalibrasi menghasilkan `runs.csv`, `deskriptif.csv`, `ketidakpastian.csv`, `banding_sw_hw.csv`, `pengaruh_barrage.csv`, `interaksi_dd.csv`, `layanan.csv`, `leave_one_block_out.csv` yang identik dengan `stop_final/`.

```
# 1. (hanya bila data berubah) ringkas per sesi dengan kalibrasi final
python tools/run_bench.py ringkas data/runs/<sesi> --rshunt 0.0821 --offset -2.83
# 2. statistik utama -> data/analisis/stop_final/
python tools/analisis_d.py --manifest data/tahap_stop.txt --stop --rshunt 0.0821 --offset -2.83 --u-rel 0.015 --keluar data/analisis/stop_final
# 3. turunan (membaca stop_final/runs.csv)
python tools/analisis_sensitivitas.py                       # tanpa argumen
python tools/dekomposisi_energi.py --rshunt 0.0821 --offset -2.83
python tools/proyeksi_baterai.py
python tools/gambar_bab4_stop.py --shunt 0.0821
# 4. kumpulkan gambar dengan NOMOR naskah (menolak bila gambar lebih tua dari runs.csv atau kalibrasi tak cocok)
python tools/ekspor_gambar_naskah.py                         # -> data/analisis/stop_final/gambar_naskah/Gambar_5.x_*.png + README.md
```
**Urutan itu penting**: gambar dan tabel turunan harus dibuat SESUDAH `analisis_d.py` terakhir. Setelah mengganti kalibrasi, **lihat gambarnya** (jangan hanya menjalankan skrip): sumbu yang dikunci pada skala lama pernah memotong atau mengosongkan gambar.

**Jebakan yang sudah pernah terjadi**
1. **Default kalibrasi (diubah 21 Nov 2026).** Default baris perintah `--rshunt/--offset` (dan `--u-rel` di `analisis_d.py`) di `run_bench`, `analisis_d`, `dekomposisi_energi`, `gambar_bab4_stop`, `hitung_ulang_satu_run`, `demo_sidang`, `uji_galat_stop`, `k1s_run` kini memakai
   `tools/kalibrasi_param.py`. Yang TETAP nominal (0,100 / -2,0): default fungsi `analyse()` (untuk uji `tests/`) dan skrip uji tahap awal `s0_stop.py`, `s1_rtc.py`, `s2_rx.py`, `a3_clock.py` (angka tahap awal di Tabel 4.5/4.6/4.7 naskah dikonversi dengan `I_baru = 1,218 x I_lama + 0,025 mA`).
   Akibat: `run_bench.py ringkas` pada sesi lama, atau `analisis_d.py` tanpa argumen pada tahap D (`data/tahap_d.txt`, data pengembangan 15 Nov), sekarang mengeluarkan angka terkalibrasi; tulis `--rshunt 0.100 --offset -2.0` bila ingin angka lama. `sesi.txt` mencatat nilai yang dipakai.
   Juga: arus demo (`demo_sidang.py`) dulu memakai 0,100 sehingga selisih arus demo vs skripsi **bukan** soal kalibrasi; kini keduanya terkalibrasi.
   Sumbu gambar tidak lagi tetap: Gambar 4.4/4.5/4.6(b)/4.9/4.1 disesuaikan data (sebelumnya batas 21 mA, 50 mJ, 74 mW, 1,93-2,05 mA dibuat untuk skala nominal dan akan memotong atau mengosongkan gambar terkalibrasi).
2. `analisis_sensitivitas.py` tidak punya argparse: memanggilnya dengan `--help` justru menjalankannya (membaca dan menimpa berkas di `stop_final/sensitivitas/`).
3. `dekomposisi_energi.py`, `proyeksi_baterai.py`, `analisis_sensitivitas.py` menulis ke `stop_final/` yang tetap; salin hasil lama dulu bila ingin membandingkan (`stop_final_LAMA_rshunt0.100_offset-2.0/`).
4. Mengubah `platformio.ini` menghapus `.pio/build`; setelah flash wajib POR penuh sebelum mengukur arus.
5. Resistor kalibrasi dipasang di sisi beban (IN- atau pin 5V host ke GND), **bukan** IN+; multimeter seri jangan dilepas selagi host menyala (mereset host).
6. V_bus 0 V pada rekaman offset = IN+ tidak berdaya (USB sumber tercabut): rekaman tidak sah.
7. **Bland-Altman latensi pada mode `--stop` tidak masuk `bland_altman.csv`.** `muat()` mengisi `pins["arch"]` dengan `K1S/K2S`, sedangkan `main()` mencari `K1`/`K2`
   (`analisis_d.py:580-583`), jadi baris latensi tidak terbentuk; berkas hanya berisi baris S_cpu. Angka Tabel 5.4 naskah (K1 bias +9,29 ms sd 0,06; K2 +3,28 ms sd 0,26) cocok dengan hitungan ulang
   langsung dari `latensi_pin_vs_dwt.csv` (3.540 pasangan: K1S 9,285/0,058; K2S 3,283/0,256), jadi angkanya benar tetapi dibuat di luar `analisis_d.py`. Perbaikan kecil: cocokkan dengan `a + "S"` bila `STOP_MODE`.

---

## 10. Latihan akhir (rantai penuh)

1. Ambil `data/runs/20261118_124646_K1S/007_K1S_B.csv`: berapa kali PB1 naik-turun, dan berapa total detik rendah? Hitung `S_cpu` dengan tangan lalu bandingkan dengan `runs.csv`.
2. Dari `kalibrasi_final_2026-11-21.md` hitung ulang `R_eff` 1 kOhm: `dX = 173,2`, `V_bus = 5,252 V`, `R = 1000`.
3. Jalankan `analisis_d.py` ke folder uji dengan `--rshunt 0.100 --offset -2.0 --u-rel 0.05 --keluar data/analisis/uji_lama` (nilai nominal harus ditulis eksplisit sekarang) dan tunjukkan angka mana yang berubah dan mana tidak.
4. Jelaskan dalam tiga kalimat mengapa gain % tidak bergantung pada `R_eff` tetapi mJ bergantung.
5. Mengapa baseline B-R-B mengapit resistor? Apa yang terjadi pada hasil bila hanya ada baseline sebelum?
6. Sebutkan satu keputusan yang diputuskan manusia, bukan kode, dan di mana ia dilaporkan sebagai keterbatasan.

**Berkas rujukan utama**: `tools/run_bench.py`, `tools/analisis_d.py`, `tools/kalibrasi_shunt.py`, `tools/dekomposisi_energi.py`, `tools/analisis_sensitivitas.py`,
`tools/proyeksi_baterai.py`, `tools/gambar_bab4_stop.py`, `tools/hitung_ulang_satu_run.py`, `src/logger/f103/main.cpp`, `src/host/final/ (K1S/K2S) dan src/host/dev/tahap_stop/ (S0-S2) {stop_common,stop_window,stop_link}.h`,
`src/host/final/ (K1S/K2S) dan src/host/dev/tahap_stop/ (S0-S2) {k1_stop,k2_stop}/main.cpp`, `src/host/final/bersama/ds28e38_od.h`, `src/attacker/main.cpp`, `lib/bench_proto/*`, `data/kalibrasi/*`.
