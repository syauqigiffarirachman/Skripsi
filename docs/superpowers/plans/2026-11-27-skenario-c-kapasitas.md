# Skenario C (kapasitas, ρ = 1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Menambah skenario serangan C (periode attacker = latensi tanda tangan rata-rata, ρ = 1) dan mengambil 30 run sah per arsitektur.

**Architecture:** Attacker mendapat mode periode bebas (`p<us>`). Uji pendahuluan (run A dan B) menghasilkan t̄_sign dan faktor jam attacker, lalu
disimpan ke `data/kapasitas_periode.json`. `run_bench.py` memakai berkas itu untuk skenario `C`. P_idle diambil dari IDLE `stop_final`.
Firmware host tidak berubah.

**Tech Stack:** PlatformIO + STM32duino (attacker F030C8), Python 3.12 (`run_bench.py`, unittest stdlib), pandas/scipy di `.venv-analisis`.

**Spec:** `docs/superpowers/specs/2026-11-27-skenario-c-kapasitas-design.md`

## Global Constraints

- sha256 `.bin` host tidak berubah: `host_k1_stop` `2ca6e8d0d861…`, `host_k2_stop` `5b4e017827e3…`.
- Perilaku attacker A (1/s) dan B (10/s) tidak berubah; frame 36 B tetap (`lib/bench_proto/proto_attacker_host.h`).
- Tidak ada run IDLE baru; P_idle/I_idle dari baris `scen == "IDLE"` di `data/analisis/stop_final/runs.csv`, per arsitektur.
- rshunt 0,0821, offset -2,83 (bawaan `kalibrasi_param`).
- Uji Python: `.venv-analisis/Scripts/python -B -m unittest discover -s tests -v`. Nilai harapan dihitung tangan dulu (aturan `tests/test_metrik.py`).
- Komentar kode berbahasa Indonesia. Commit hanya bila diminta peneliti. Flash papan hanya dengan ST-Link yang dipasang peneliti.

## Review Focus

- Perintah `p` dengan angka di luar 1.000–10.000.000 µs, atau tanpa angka: attacker menolak dan tidak mulai mengirim.
- Karakter `a`/`b`/`x` saat sedang mengetik `p…`: perintah lama tetap bekerja (pengetikan `p` dibatalkan).
- `kapasitas_periode.json` tidak ada atau tidak memuat arsitektur yang terpasang: `run_bench` berhenti sebelum run dengan pesan jelas.
- Sesi uji pendahuluan tanpa run B sah: faktor k tidak dihitung, dan alat berhenti (bukan memakai k = 1 diam-diam).
- Toleransi frame hilang (`arrived >= 100`) tidak berlaku untuk K1S-C (±92 frame/60 s): frame hilang di K1S-C menggugurkan run.

---

### Task 1: Attacker mode C (periode bebas)

**Files:**
- Modify: `src/attacker/main.cpp` (enum `Mode`, `modeStart`, `printStatus`, `printHelp`, `loop`)
- Modify: `src/attacker/README.md`, `tools/4_verifikasi/biner_acuan.json` (acuan `attacker`)

**Interfaces:**
- Produces: perintah konsol `p<periode_us>` + `\r` atau `\n` → balasan `[attacker] mulai mode C (<periode_us> us)`;
  `s` → `[attacker] mode=C berjalan=ya seq=… t_ms=… periode_us=<n>`; di luar rentang → `[attacker] periode ditolak`.

- [ ] **Step 1:** Tambah `MODE_C = 3` dan `static uint32_t g_period_us`; `timerStartUs(uint32_t us)` memakai `Ticker.setOverflow(us, MICROSEC_FORMAT)`. Fungsi `modeStart(MODE_A/MODE_B)` tidak disentuh; buat `modeStartPeriod(uint32_t us)` terpisah.
- [ ] **Step 2:** Parser: `p` membuka penyangga angka (maks 8 digit); digit ditambahkan; `\r`/`\n` menutup dan memanggil `modeStartPeriod` bila 1.000 ≤ n ≤ 10.000.000, selain itu `periode ditolak`; huruf lain membatalkan penyangga lalu diproses seperti biasa.
- [ ] **Step 3:** `pio run -e attacker` → SUCCESS. Catat sha `.bin` lama dan baru di `biner_acuan.json` (lama dipindah ke catatan).
- [ ] **Step 4 (butuh ST-Link, peneliti):** flash `pio run -e attacker -t upload` dengan hanya ST-Link attacker tercolok. Uji konsol: `p116200` → mulai mode C; `s` menunjukkan `periode_us=116200`; `p5` → ditolak; `x` → berhenti.
- [ ] **Step 5 (bangku):** `run_bench.py run A 60` dan `run B 60`: N_dtg = 59–60 dan 600–601 (sama dengan 18 Nov).

### Task 2: Alat periode kapasitas

**Files:**
- Create: `tools/1_akuisisi/periode_kapasitas.py`
- Test: `tests/test_kapasitas.py`

**Interfaces:**
- Produces: `hitung_periode(rows: list[dict]) -> dict` dengan rows = baris `ringkasan.csv` sesi uji pendahuluan (kolom `arch, scen, masalah, lat_mean_ms_nyata, lat_n, N_dtg, T_obs_log_s`). Hasil per arsitektur:
  `{"t_sign_ms": rerata tertimbang lat_n dari run A sah, "n_sign": int, "lambda_B": ΣN_dtg/ΣT_obs run B sah, "k": 10/lambda_B, "periode_kirim_us": round(t_sign_ms*1000/k), "periode_nyata_us": round(t_sign_ms*1000)}`.
- Produces: berkas `data/kapasitas_periode.json` = `{"<arch>": {...hasil..., "sesi": [nama sesi]}}`; CLI `python tools/1_akuisisi/periode_kapasitas.py data/runs/<sesi> [...]` (gabung beberapa sesi, arsitektur yang sama ditimpa sesi terakhir).

- [ ] **Step 1: Uji gagal dulu.**
  `test_periode_dasar`: run A sah K2S `lat_mean_ms_nyata=116.0, lat_n=60` dan `116.6, lat_n=40`; run B sah `N_dtg=600, T_obs_log_s=60.0` dan `602, 60.2` → `t_sign_ms = (116*60+116.6*40)/100 = 116.24`, `lambda_B = 1202/120.2 = 10.0`, `k = 1.0`, `periode_kirim_us = 116240`.
  `test_jam_attacker_lambat`: B `N_dtg=594, T_obs=60.0` → `lambda_B = 9.9`, `k = 1.0101…`, t_sign 654.9 ms → `periode_kirim_us = round(654900/1.010101) = 648351`.
  `test_run_bermasalah_diabaikan`: baris dengan `masalah` tak kosong tidak dihitung.
  `test_tanpa_B_gagal`: tanpa run B sah → `ValueError`.
- [ ] **Step 2:** Jalankan → FAIL (modul belum ada). **Step 3:** implementasi. **Step 4:** Jalankan → PASS.

### Task 3: `run_bench.py` skenario C + IDLE luar

**Files:**
- Modify: `tools/1_akuisisi/run_bench.py` (`SCEN`, `do_run`, `energi_sesi`, `KELOMPOK`, `main`)
- Test: `tests/test_kapasitas.py`

**Interfaces:**
- Consumes: `data/kapasitas_periode.json` (Task 2).
- Produces: `SCEN["C"] = (4, "PERIODE")` (logger tetap cmd 4 = jendela permintaan nyata); `perintah_attacker(scen, arch, periode: dict) -> str | None` mengembalikan `"a"`, `"b"`, `None`, atau `f"p{periode[arch]['periode_kirim_us']}\n"`; `KeyError`/`SystemExit` berpesan bila arsitektur tak ada.
- Produces: `idle_luar(path: Path) -> dict[str, tuple[float, float]]` = rerata (P_avg_mW, I_avg_mA) baris IDLE tanpa masalah per arsitektur di `runs.csv` stop_final; kunci dinormalkan ke `K1S`/`K2S` (runs.csv menulis `K1`/`K2`).
- Produces: `energi_sesi(rows, idle_luar=None)` memakai idle_luar bila sesi tidak punya IDLE arsitektur itu, dan menandai `P_idle_sumber = "sesi" | "stop_final"`.
- Produces: kolom ringkasan run C `periode_us` (nyata, dari JSON) dan `rho_nyata = lambda_eff_rps * t_sign_ms/1000` (t_sign_ms dari JSON). KELOMPOK: `("K1S","C"): "K5S"`, `("K2S","C"): "K6S"`.
- CLI: `--periode data/kapasitas_periode.json` (bawaan), `--idle-dari data/analisis/stop_final/runs.csv` (bawaan kosong = tidak dipakai; sesi C memakainya).

- [ ] **Step 1: Uji gagal dulu.**
  `test_perintah_attacker`: A→`"a"`, B→`"b"`, IDLE→`None`, C K2S dengan `periode_kirim_us=116240`→`"p116240\n"`; C untuk arsitektur tak ada di JSON → `SystemExit`.
  `test_idle_luar`: runs.csv sintetis IDLE K2 `P=12.0, I=2.4` dan `13.0, 2.5` → `{"K2S": (12.5, 2.45)}`.
  `test_energi_sesi_idle_luar`: run C K2S `P_avg_mW=42.5, I_avg_mA=8.3, T_obs_log_s=60, N_lyn=516`, idle_luar `(12.5, 2.45)` → `P_dyn_mW = 30.0`, `E_lyn_mJ = 30*60/516 = 3.48837`, `P_idle_sumber = "stop_final"`.
  `test_rho`: `lambda_eff_rps = 8.6, t_sign_ms = 116.24` → `rho_nyata = 0.999664`.
- [ ] **Step 2:** FAIL. **Step 3:** implementasi (do_run mengirim string perintah dari `perintah_attacker`; skenario lain tetap sama). **Step 4:** PASS, dan seluruh uji lama tetap lulus (55).
- [ ] **Step 5:** `run_bench.py plan C:2 --tanpa-idle --kering` mencetak urutan tanpa papan.

### Task 4: Uji pendahuluan dan sesi data C (bangku)

**Files:** `data/runs/<sesi>/`, `data/kapasitas_periode.json`, `data/tahap_kapasitas.txt`, `logbook/2026-11-27.md`

- [ ] **Step 1:** K2S (terpasang): `plan A:5,B:2 --secs 60 --tanpa-idle` (dijalankan 27 Nov). K1S: flash `host_k1_stop` (ST-Link `$`), POR, `nodebug`, cabut ST-Link, lalu plan yang sama.
- [ ] **Step 2:** `periode_kapasitas.py <sesi K2S> <sesi K1S>` → JSON; bandingkan t̄_sign dengan 654,9 / 116,2 ms.
- [ ] **Step 3:** Task 1 Step 4–5 (flash dan uji attacker).
- [ ] **Step 4:** Blok K1S (15 C) | K2S (15 C) | K2S (15 C) | K1S (15 C) dengan `plan C:15 --tanpa-idle --idle-dari data/analisis/stop_final/runs.csv`. Blok ke-2 dan ke-3 berurutan (flash 2 kali). Tambah run sampai 30 sah per kelompok.
- [ ] **Step 5:** Tulis `data/tahap_kapasitas.txt` (format `tahap_stop.txt`: `blokN <sesi>`) + sha host dan attacker; logbook 27 Nov.

### Task 5: Analisis skenario C

**Files:**
- Create: `tools/3_analisis/analisis_kapasitas.py` → `data/analisis/kapasitas/` (`deskriptif.csv`, `banding.csv`, `ringkasan.md`)
- Test: `tests/test_kapasitas.py`

**Interfaces:**
- Consumes: `analisis_d.muat(sesi, rshunt, offset, stop=True)` untuk sesi `tahap_kapasitas.txt` dan `tahap_stop.txt`; fungsi `deskriptif`, `boot_mean`, dan pola Welch + Mann-Whitney + Holm dari `analisis_d.py`.
- Produces: tabel per arsitektur × skenario (A, C, B) untuk `lat_mean_ms_nyata, rho_nyata, TL_pct, R_lwt_pct, Theta_sps, S_cpu_log, I_avg_mA, P_avg_mW, E_per_sign_mJ` (+ sekunder `P_dyn_mW, E_lyn_mJ`); perbandingan K5S vs K6S, K5S vs K3S, K6S vs K4S (Holm atas 3 × metrik utama).

- [ ] **Step 1:** Uji gagal: `test_holm_tiga` (p = 0,01, 0,04, 0,03 → disesuaikan 0,03, 0,06, 0,06). **Step 2–4:** FAIL → implementasi → PASS.
- [ ] **Step 5:** Jalankan pada data nyata; baca `ringkasan.md` dan periksa ρ_nyata 0,98–1,02 dan TL masuk akal sebelum dilaporkan.
