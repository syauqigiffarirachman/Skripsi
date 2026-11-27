# Desain: Skenario C — serangan tepat di kapasitas maksimum (ρ = 1)

Tanggal: 27 Nov 2026. Status: menunggu persetujuan peneliti.

## 1. Maksud dan alasan

Revisi dosen penguji setelah seminar proposal (26 Nov):
1. Apa urgensi mengukur latensi tanda tangan?
2. Tambah skenario serangan **tepat di kapasitas maksimum** arsitektur software dan hardware, untuk melihat perilaku
   papan saat diserang tepat di batas kemampuannya.
3. Perjelas skenario pengujian dan serangan.

Skenario yang ada: A = 1 req/s (ρ = 0,65 SW; 0,12 HW) dan B = 10 req/s (ρ = 6,6 SW; 1,2 HW). Keduanya bukan "tepat di
kapasitas". Skenario C mengisi celah itu: attacker mengirim permintaan dengan **periode = latensi tanda tangan rata-rata**
arsitektur yang diserang, sehingga tiap permintaan baru tiba tepat saat tanda tangan sebelumnya selesai (λ = 1/t_sign,
ρ = 1).

Kaitan dengan revisi 1: latensi menentukan kapasitas layanan (Θmax = 1/t_sign), fraksi waktu CPU terjaga (ρ = λ·t_sign),
dan energi per tanda tangan. Latensi adalah besaran yang **menentukan titik serangan maksimum** itu sendiri, jadi bukan
sekadar metrik pembanding.

## 2. Sasaran, bukan sasaran, anti-sasaran

**Sasaran**
- Ukur ulang latensi tanda tangan K1S dan K2S di papan sekarang (uji pendahuluan).
- Attacker bisa mengirim pada periode bebas (bukan hanya 1 dan 10/s); A dan B tetap persis seperti semula.
- Ambil 30 run sah per arsitektur pada skenario C (K1S-C, K2S-C), 60 s per run.
- Analisis C dibandingkan dengan A dan B (data `stop_final`, 18 Nov) per arsitektur, dan K1S-C dibandingkan dengan K2S-C.

**Bukan sasaran**
- Tidak mengambil run IDLE baru (keputusan peneliti): P_idle diambil dari data IDLE terakhir (`stop_final`).
- Tidak mengubah firmware host (K1S, K2S); sha256 biner harus sama dengan acuan.
- Tidak membuat loop tertutup (sinyal "selesai" dari host) dan tidak menyapu laju (0,8–1,2× kapasitas).
- Tidak merevisi naskah di pekerjaan ini; teks Bab 3/4 dan jawaban dosen menyusul sebagai tugas terpisah.

**Anti-sasaran** (gagal walau teknisnya jalan)
- Perilaku attacker pada A atau B berubah: data 18 Nov tidak lagi sebanding.
- Laju C yang dikirim ternyata bukan ρ ≈ 1 (misalnya karena galat jam HSI attacker) tetapi tidak ketahuan.
- Run C diambil dengan host dalam keadaan debug (ST-Link menempel, DBGMCU_CR aktif): arus tidak sah.

## 3. Batasan

- Firmware host final: `host_k1_stop` sha `2ca6e8d0d861…`, `host_k2_stop` sha `5b4e017827e3…` (`tools/4_verifikasi/biner_acuan.json`).
- Attacker F030C8 berjalan di HSI 48 MHz (galat bisa sekitar ±1 %); pewaktu TIM3 16-bit.
- Jalur frame attacker → host 115200 baud, frame 36 B (±3,1 ms per frame).
- Host menyimpan maksimal satu permintaan tertunda; kelebihannya tercatat terlewat (`stop_window.h:112-118`).
- Ganti arsitektur = flash host lewat ST-Link `$`, lalu POR penuh dan `run_bench.py nodebug`; ST-Link dicabut saat mengukur.
- Kriteria keabsahan run sama dengan Bab 3: frame rusak 0, frame hilang < 0,25 % (khusus beban tinggi), tidak ada baris
  data hilang, tidak ada galat checksum/I²C.

## 4. Pendekatan

| | Cara | Untung | Rugi |
|---|---|---|---|
| **1 (dipilih)** | Laju tetap per arsitektur: periode attacker = t̄_sign hasil ukur | Firmware host tidak berubah, data setara 120 run lama; cukup ubah attacker | Latensi bergetar (K2 ±115–121 ms), jadi TL bisa sedikit < 100 %; perlu kalibrasi jam attacker |
| 2 | Loop tertutup: host memberi sinyal "selesai", attacker langsung menembak | Paling harfiah "tepat saat selesai" | PB1 tidak bisa dipakai di K2 (CPU tidur Stop di tengah tanda tangan), jadi perlu pin dan firmware host baru; sebanding dengan data lama melemah |
| 3 | Sapuan laju 0,8–1,2× kapasitas | Menunjukkan titik tekuk | Run jauh lebih banyak; N per titik < 30 |

Peneliti memilih cara 1 (27 Nov), dengan ρ = 1.

## 5. Rancangan

### 5.1 Uji pendahuluan latensi (papan sekarang)
Per arsitektur: **5 run A + 2 run B, 60 s, tanpa IDLE** (`--tanpa-idle`). *(Keputusan saya: jumlah run.)*
- Dari run A diambil latensi rata-rata nyata (`lat_mean_ms_nyata`, waktu kristal logger), dari ±295 tanda tangan per
  arsitektur. Ini menjadi t̄_sign skenario C. Rata-rata dipakai, bukan maksimum, karena Θmax = 1/t̄_sign adalah definisi
  kapasitas di Bab 3.
- Run B dipakai untuk mengukur jam attacker (5.3), sekaligus sebagai jembatan ke data B 18 Nov.
- Bila t̄_sign berbeda jauh dari 18 Nov (654,9 / 116,2 ms), dilaporkan; periode C tetap memakai nilai hari ini.

### 5.2 Attacker: mode C (periode bebas)
- Perintah konsol baru `p<periode_us>` diakhiri Enter, misalnya `p654900`. Attacker mengirim frame yang sama (format
  `proto_attacker_host.h`) setiap periode itu. `s` menampilkan mode C dan periodenya.
- `a`, `b`, `x`, `s`, `f` tidak berubah. Kode A/B dibiarkan; mode C ditambahkan di sampingnya.
- Pewaktu: HardwareTimer TIM3 dengan format mikrodetik (prescaler diatur otomatis). Resolusi ≤ 0,1 % untuk periode
  100 ms–1 s; dicek saat implementasi.
- Biner attacker berubah, jadi acuan sha di `biner_acuan.json` diperbarui dan sha lama disimpan di catatan.
  Uji ulang A/B sesudah flash: N_dtg per 60 s harus sama dengan sebelumnya. *(Keputusan saya.)*
- Flash attacker lewat ST-Link (ST-Link lain dicabut) atau bootloader CH340, sesuai `platformio.ini` `[board_attacker]`.

### 5.3 Kalibrasi jam attacker
λ_B,nyata = N_dtg / T_obs_log dari 2 run B per arsitektur (±600 frame per run, resolusi ±0,2 %).
Faktor k = 10 / λ_B,nyata. Periode yang dikirim = t̄_sign / k, sehingga periode nyata (jam kristal logger) = t̄_sign.
*(Keputusan saya: kalibrasi lewat run B, tanpa alat tambahan.)*
Di tiap run C, ρ_nyata = (N_dtg / T_obs_log) × t̄_sign dihitung dan dilaporkan.

### 5.4 `run_bench.py`
- Skenario baru `C`. Periode per arsitektur dibaca dari berkas `data/kapasitas_periode.json`, yang ditulis langkah 5.1
  dan 5.3 dan memuat t̄_sign, k, periode terkirim, serta sesi sumbernya. Skrip mengirim `p<periode>` ke attacker
  setelah `>START`, sama seperti `a`/`b` sekarang.
- Kolom ringkasan baru: `periode_us`, `rho_nyata`.
- `ringkas` dapat mengambil P_idle/I_idle dari data IDLE `stop_final` (opsi `--idle-dari`), karena sesi C tidak punya
  IDLE.

### 5.5 Protokol sesi data C
- Empat blok berselang-seling seperti 18 Nov, dimulai dari arsitektur yang sedang terpasang: misalnya
  K2S | K1S | K1S | K2S, masing-masing 15 run C. Flash hanya dua kali. *(Keputusan saya: meniru pola ABBA Bab 3.)*
- Sesudah tiap flash: POR penuh, `nodebug`, ST-Link host dicabut.
- Run diambil sampai tiap kelompok punya tepat 30 run sah; run gugur dicatat beserta alasannya.
- Manifes `data/tahap_kapasitas.txt` mencatat sesi, sha biner host dan attacker, rshunt 0,0821, offset -2,83.

### 5.6 Analisis
Skrip baru `tools/3_analisis/analisis_kapasitas.py` (venv `.venv-analisis`), yang membaca data C dan `stop_final`:
- Tabel per arsitektur × skenario (A, C, B): t_sign, ρ_nyata, TL, R_lwt, Θ, S_cpu, I_avg, P_avg, energi per tanda tangan.
- Uji (sama dengan Bab 3: Welch + Mann-Whitney + Holm, bootstrap 10.000):
  K1S-C vs K2S-C (pengaruh arsitektur di kapasitas), dan C vs B per arsitektur (biaya frame kelebihan di atas kapasitas).
- Metrik **utama** = besaran mutlak yang tidak butuh IDLE (I_avg, P_avg, S_cpu, TL, latensi). P_dyn/E_lyn
  (butuh P_idle dari `stop_final`) dilaporkan sebagai sekunder dengan catatan risiko 6.1.

## 6. Risiko

1. **IDLE lama dipakai ulang.** Papan sama dengan 18 Nov (konfirmasi peneliti 27 Nov: tidak ada papan baru, hanya kabel
   ditata ulang). IDLE K2S 27 Nov = 2,43 mA (rshunt 0,0821), sedangkan IDLE `stop_final` terkalibrasi = 2,451 mA (K1) dan
   2,447 mA (K2) (`data/analisis/stop_final/deskriptif.csv`). Selisih ±0,02 mA (±0,1 mW). Angka 2,20 mA yang sempat
   disebut berasal dari `ringkasan.csv` 18 Nov yang belum dikalibrasi, jadi bukan pembanding. IDLE tetap dipantau lewat
   I_tidur run C; bila bergeser > 0,1 mA dari 2,45 mA, dilaporkan.
2. **Penataan ulang kabel** bisa mengubah sedikit kondisi ukur dibandingkan 18 Nov. Run A/B di 5.1 memberi pembanding
   langsung ke data A/B lama; selisihnya dilaporkan.
3. Getaran latensi K2 dan galat jam attacker membuat ρ nyata sedikit di atas atau di bawah 1. Mitigasi: kalibrasi 5.3
   dan pelaporan ρ_nyata per run.
4. Frame hilang di K2 saat beban tinggi (sudah dikenal dari 18 Nov) bisa menggugurkan sebagian run; siapkan cadangan
   run seperti 18 Nov.

## 7. Diserahkan ke pembangun
- Detail implementasi perintah `p` di attacker (parsing, batas periode yang diterima).
- Format persis `kapasitas_periode.json` dan nama kolom baru.
- Gambar/plot untuk Bab 4 (menyusul bersama revisi naskah).
