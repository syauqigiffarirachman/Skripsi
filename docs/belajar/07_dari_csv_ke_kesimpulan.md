# 07. Dari CSV mentah sampai kesimpulan: asal setiap angka

> **Catatan 21 Nov 2026:** angka mutlak di berkas ini (mA, mW, mJ, 1,99 mA, 40,10 mW, 41,03 mJ, u_kal 5 %, "belum dikalibrasi") memakai shunt nominal 0,100 Ohm dan SUDAH USANG.
> Setelah kalibrasi: R_eff 0,0821 Ohm, offset -2,83, u_kal 1,5 % (angka baru: `08_alur_data_lengkap_2026-11-21.md`). Persentase, rasio, latensi, S_cpu, TL, dan p-value tidak berubah.
> Bagian 7.1 (kalibrasi belum dilakukan) dan jawaban penguji tentang kalibrasi/arus demo juga perlu dibaca dengan 08.

Dibuat 19 Nov 2026. Untuk menjawab: "Apakah tabel dan perhitungan analisis benar dari rumus di paper? Bagaimana
alurnya sampai grafik? Dari kode mana angka di pembahasan dan kesimpulan?" Semua angka di bawah diambil dari
`data/analisis/stop_final/` (data skripsi = varian deep sleep/Stop) dan saya periksa ulang sendiri hari ini.

## 0. Jawaban singkat

**Ya, perhitungannya mengikuti rumus paper, dan saya buktikan secara independen.** Saya menghitung ulang tiga run
langsung dari baris CSV mentah, **tanpa** memakai `run_bench.py` (skrip `tools/3_analisis/hitung_ulang_satu_run.py`, hanya
pustaka standar Python). Hasilnya sama dengan `runs.csv` sampai enam angka desimal untuk semua metrik (T_obs, S_cpu,
I_avg, P_avg, E, latensi, TL, TL model, P_dyn, E_lyn, I_dyn). Saya juga menghitung ulang rerata kelompok, gain %, uji
Welch, dan koreksi Holm dari `runs.csv`; semuanya sama dengan `ringkasan.md`.

**Tiga hal yang harus Anda ketahui sebelum sidang** (detail di Bagian 7):
1. Paper (3.6.3) menyebut **Shapiro-Wilk dan ANOVA**, tetapi **kode tidak menjalankannya**. Saya hitung sekali hari
   ini (Bagian 7.1). Pilih: tambahkan ke naskah/pipeline, atau hapus dari kalimat paper.
2. **Angka mutlak (mA, mW, mJ) memakai shunt nominal 0,100 ohm**; shunt nyata ~0,080-0,083 ohm. Persentase, rasio,
   latensi, S_cpu, TL, dan p-value tidak terpengaruh. Angka mutlak belum final.
3. Teks asli PROPO FIX_5 tidak ada di repositori, hanya dokumen revisinya (`laporan/revisi_bab3_propo_fix5_...md`,
   `laporan/arsip/laporan_progres_revisi_paper_2026-11-15.md`). Rumus yang saya cocokkan adalah yang tertulis di dua
   dokumen itu. Butir yang hanya ada di teks asli perlu Anda cocokkan sendiri.

---

## 1. Peta alur (satu halaman)

```
 bangku ukur                  PC                                                        naskah
 -----------   CSV mentah    ----------------------------------------------------      ------
 logger  --->  data/runs/<sesi>/NNN_<ARCH>_<skenario>.csv   (s, e, h, #SUM, #RUN)
                  |
                  | run_bench.parse()  +  run_bench.analyse()      [per run, 1 CSV -> 1 baris]
                  |     T_obs, S_cpu, I_avg, P_avg, E, latensi, N_dtg/N_lyn, TL, ...
                  | run_bench.energi_sesi()   [pakai run IDLE sesi yang sama]
                  |     P_idle, P_dyn, I_dyn, E_lyn, Q_lyn
                  v
 data/analisis/stop_final/runs.csv        (139 run; 128 sah)    <- tools/3_analisis/analisis_d.py --stop
                  |
                  |  hanya run sah (masalah == "") -> 4 kelompok x 30 run
                  v
 deskriptif.csv  ketidakpastian.csv  banding_sw_hw.csv  pengaruh_barrage.csv  interaksi_dd.csv
 layanan.csv     model_duty_per_run.csv  bland_altman.csv  leave_one_block_out.csv  pemeriksaan.md
                  |  (dirangkum otomatis)           |  (dibaca skrip gambar)
                  v                                  v
 ringkasan.md  --> tabel Bab 4 / 5               tools/3_analisis/gambar_bab4_stop.py --> gambar_4_x.png --> Gambar 4.x
                                                  tools/3_analisis/dekomposisi_energi.py --> Gambar 4.12
```

Kelompok: **K1** = software normal (K1S, 1/s), **K2** = DS28E38 normal (K2S), **K3** = software barrage (10/s),
**K4** = DS28E38 barrage. Huruf "S" pada K1S/K2S = varian Stop; `analisis_d.py --stop` membuang huruf itu (baris 147-149).

## 2. Isi CSV mentah (sumber segalanya)

| Baris | Arti | Dibuat oleh |
|---|---|---|
| `s,<tik>,<shunt>,<bus>` | satu cuplikan INA226; `<tik>` jam logger 72 MHz; shunt dan bus berupa angka register | logger |
| `e,<tik>,<1 atau 0>` | tepi pin status host (PB1 -> PA0 logger): 1 = naik = CPU bangun, 0 = turun = CPU tidur | logger |
| `h,<tik>,>START,...` / `>END,...` / `>L,...` | baris yang host kirim lewat USART1, diteruskan apa adanya | host |
| `#SUM,...` | ringkasan logger (jumlah cuplikan, checksum, galat I2C, cuplikan terlambat) | logger |

`>END` dari host berisi (urutan di `run_bench.END_COLS`): arsitektur, detik, ..., `t_obs_us` (lama jendela menurut
jam host), `t_sleep_us` (lama tidur menurut host), `arrived`, `served`, `gagal`, `rx_bad`, `rx_lost`, `lat_mean`
(siklus), `clk_hz` (frekuensi inti terukur). Semua cap waktu `s` dan `e` memakai satu jam: kristal logger. Itu
sebabnya T_obs, energi, dan S_cpu "versi logger" dianggap acuan.

## 3. Langkah 1: perhitungan per run (`run_bench.analyse`, baris 321-448)

| Metrik | Rumus | Di paper | Baris kode |
|---|---|---|---|
| **T_obs** | `(t1 - t0) / 72.000.000`; t0 = tepi naik PB1 pertama setelah `>START`, t1 = tepi turun terakhir sebelum `>END` | 3.5.1 butir 2 ("T_obs menurut logger") | 361-367 |
| **S_cpu** | `waktu PB1 rendah / T_obs x 100` (logger); `t_sleep / t_obs x 100` (firmware) | 3.5.1 butir 3: S_CPU = Tsleep / Tobs x 100 % | 368-380 |
| **Latensi T_sign** | `lat_mean x 1000 / clk_hz` (ms), lalu `x T_obs_logger / T_obs_host` (koreksi jam) | 3.5.1 butir 1: Tsign = (Nend - Nstart)/fCPU x 1000, dikoreksi rasio jendela logger/papan. K2S memakai RTC (`STOP_LAT_RTC`, `k2_stop/main.cpp:64`) | 400-406 |
| **I_avg, P_avg, E** | `I = (shunt - offset) x 2,5 uV / R_shunt`; `V = bus x 1,25 mV`; E = integral V.I dt (trapesium) pada [t0, t1], interpolasi di kedua batas; `P_avg = E/T`, `I_avg = Q/T` | 3.5.1 butir 2 | 409-431 |
| **N_dtg, N_lyn, N_lwt** | langsung dari `>END`: `arrived`, `served`; `N_lwt = N_dtg - N_lyn - N_ggl` | 3.5.1 butir 4 | `layanan()` 234-253 |
| **TL** | `N_lyn / N_dtg x 100` (penyebut = yang terukur, mis. 59, bukan 60) | H4 | 245 |
| **TL model** | `min(1, 1 / (lambda_eff x t_sign)) x 100`, `lambda_eff = N_dtg / T_obs` | 3.5.2 (utilisasi rho) | 251 |
| **N_rusak** | `rx_bad + rx_lost` (frame rusak + frame hilang, dicatat bersama) | 3.5.1 butir 4 (rusak = 0; hilang < 0,25 % pada barrage) | 242, 391-398 |
| **P_dyn, I_dyn** | `P_avg - P_idle`, `I_avg - I_idle`; P_idle/I_idle = rerata run **IDLE** arsitektur sama **di sesi yang sama** | 3.5.1 butir 2e | `energi_sesi()` 256-284 |
| **E_lyn** | `P_dyn x T_obs / N_lyn` (sama dengan `(E - P_idle.T_obs)/N_lyn`, "E_req" di paper) | 3.5.1 butir 2e | 283 |
| **Q_lyn** | `I_dyn x T_obs / N_lyn` (muatan; tahan terhadap naik-turun tegangan USB) | tambahan | 284 |

### Contoh hitung dengan tangan (run K2S normal, `033_K2S_A.csv`, sesi `20261118_115044_K2S`)

Dari `>END`: `arrived=59`, `served=59`, `t_obs=59.999.572 us`, `t_sleep=58.910.048 us`.
- S_cpu (firmware) = 58.910.048 / 59.999.572 = **98,184 %**; versi pin logger = **98,174 %** (selisih 0,011 poin).
- T_obs (logger) = 60,0024 s; I_avg = integral arus / T = **2,5546 mA**; P_avg = **13,3247 mW**.
- P_idle = rerata run IDLE K2S **di sesi yang sama** = 10,3725 mW (rerata semua blok 10,367 mW), jadi
  **P_dyn = 13,3247 - 10,3725 = 2,952 mW** dan **E_lyn = 2,952 x 60,0024 / 59 = 3,002 mJ**.
- TL = 59/59 = **100 %**; model: lambda_eff = 59/60,0024 = 0,983/s, t_sign = 0,11612 s -> rho = 0,114 -> TL model 100 %.

Cara mengecek sendiri (butuh 5 detik):
```
python tools/3_analisis/hitung_ulang_satu_run.py data/runs/20261118_115044_K2S/033_K2S_A.csv ^
   --banding data/analisis/stop_final/runs.csv --sesi 20261118_115044_K2S
```
Kolom "hitung ulang" dibuat dari rumus di atas tanpa `run_bench.py`; kolom "runs.csv" dari pipeline. Hasil saya
hari ini: selisih relatif +0,0000 % pada semua metrik untuk tiga run (K2S normal, K1S barrage, K2S barrage).

## 4. Langkah 2: apakah run ini boleh dihitung? (keabsahan)

`analyse` mengisi daftar `masalah`; bila kosong, run **sah** (`runs.csv` kolom `sah`). Pemeriksaan:
cuplikan diterima = dikirim, checksum shunt/bus cocok, tepi diterima = dikirim, tanpa galat I2C, `late` hanya
"ringan" (selang terbesar < 5 ms, <= 10 kali), S_cpu logger vs firmware beda <= 0,5 poin, sisa konsistensi
<= 2 ms, **frame rusak = 0**, frame hilang hanya boleh pada barrage Stop dan < 0,25 % dari yang tiba, tanpa
`pin_salah`, `gagal`, `lat_drop` (baris 331-398).

Hasil (`pemeriksaan.md`): 4 sesi, 139 run, **128 sah** (4 x 30 + 8 IDLE), **11 gugur**; semua blok IDLE awal-akhir
selisih <= 0,004 mA. Aturan toleransi frame hilang < 0,25 % ditetapkan **setelah** melihat penyebabnya (frame
yang mulai tepat saat CPU pulih dari Stop); paper (revisi Bab 3, butir 26) sudah menyatakannya terbuka.

## 5. Langkah 3: dari 30 run ke angka kelompok (`tools/3_analisis/analisis_d.py --stop`)

| Keluaran | Fungsi (baris) | Isi | Untuk apa |
|---|---|---|---|
| `deskriptif.csv` | `deskriptif` (171) | n, rerata, sd, CV, median, IK 95 % (t), min, maks per kelompok-metrik | Tabel A (rerata +- sd) |
| `ketidakpastian.csv` | `ketidakpastian` (291) | `u_acak` (setengah lebar IK), `u_kal` (5 % x nilai, hanya metrik energi), `u_blok` (sd rerata antar-blok), `u_idle` (drift IDLE); `U_sis = akar(u_kal^2+u_blok^2+u_idle^2)`; `U_gab = akar(U_sis^2+u_acak^2)` | Sub-bab 3.6.5 (empat komponen), batang galat |
| `banding_sw_hw.csv` | `banding_sw_hw` (329) | selisih HW vs SW, IK 95 % bootstrap (10.000 resampel, seed 20261116), gain % = selisih / rerata SW x 100, uji **Welch t**, **Mann-Whitney** (cek), **koreksi Holm** atas 6 perbandingan | RM1, H1a-H1c |
| `pengaruh_barrage.csv` | `pengaruh_barrage` (372) | barrage - normal per arsitektur + IK | RM2 |
| `interaksi_dd.csv` | `interaksi` (385) | DD = Delta_barrage - Delta_normal + IK bootstrap | RM3 |
| `layanan.csv` | `tabel_layanan` (406) | N_dtg, N_lyn, TL, TL model, rho, Theta, Theta maks | H4 |
| `model_duty_per_run.csv` | `model_duty_per_run` (420) | cek `I_avg = I_aktif.D + I_tidur.(1-D)` | 3.5.1 butir 3 |
| `bland_altman.csv` | `bland_altman` (201) | bias dan batas kesesuaian 95 %: S_cpu pin vs firmware | 3.6.4 |
| `leave_one_block_out.csv` | `leave_one_block_out` (430) | hasil bila satu blok dibuang | ketahanan hasil |

Contoh lengkap, P_dyn normal (Software K1 vs DS28E38 K2), semua dari `runs.csv`:
- K1: P_avg 50,501 - P_idle 10,397 = **40,104 mW** (sd 0,042, n 30). K2: 13,326 - 10,367 = **2,959 mW**.
- Delta = 40,104 - 2,959 = 37,14 mW. **Gain = 37,14 / 40,104 x 100 = 92,62 %** (IK 95 % 92,61-92,64).
- Welch t: p = 1,1e-113; setelah Holm 2,3e-113 -> **tolak H0** (kolom `keputusan`).
- Ketidakpastian: `U_gab` = 5,0 % hampir seluruhnya dari `u_kal` (asumsi 5 % kalibrasi shunt); untuk **gain %**
  `u_kal` saling menghapus (penguatan sama di kedua arsitektur), jadi persennya tetap kokoh.

Mengapa p-value ekstrem (1e-113)? Sebaran kelompok sangat sempit (CV 0,01-0,1 %), jadi tidak ada tumpang tindih
nilai; paper (laporan progres, butir 33) sudah menyarankan menekankan **ukuran efek**, bukan p. Mann-Whitney
mentok di 3,0e-11 (nilai minimum untuk 30 vs 30).

## 6. Langkah 4: dari tabel ke gambar (`tools/3_analisis/gambar_bab4_stop.py`)

| Gambar | Isi | Data yang dibaca | Fungsi |
|---|---|---|---|
| 4.1, 4.1b | Jejak arus satu run contoh per kelompok | CSV mentah `data/runs/...` (4 run contoh di `CONTOH`) | `gambar_41`, `gambar_41_rinci` |
| 4.2 | Latensi sign | `deskriptif.csv` | `gambar_42` |
| 4.3 | Durasi tidur CPU | `deskriptif.csv`, `ketidakpastian.csv` | `gambar_43` |
| 4.4, 4.5 | Daya dinamis, energi per layanan | idem (batang galat = `U_gab`) | `gambar_44`, `gambar_45` |
| 4.6 | Interaksi cara x skenario | `interaksi_dd.csv` | `gambar_46` |
| 4.7 | TL terukur vs model utilisasi | `layanan.csv` | `gambar_47` |
| 4.8 | Throughput vs batas kapasitas | `layanan.csv` | `gambar_48` |
| 4.9 | Arus diam IDLE awal/akhir blok | `runs.csv` | `gambar_49` |
| 4.10 | Distribusi frame hilang K4 (41 run) | `runs.csv` | `gambar_410` |
| 4.11 | Bland-Altman S_cpu | `runs.csv`, `bland_altman.csv` | `gambar_411` |
| 4.12 | Dekomposisi energi (CPU aktif / IC menghitung / tidur) | CSV mentah via `tools/3_analisis/dekomposisi_energi.py` | `gambar_412` |

Jalankan ulang bila angka berubah (mis. setelah kalibrasi): `analisis_d.py --stop --rshunt <R> --offset <o> --u-rel <u>`,
`dekomposisi_energi.py --rshunt <R>`, `gambar_bab4_stop.py --shunt <R>`.

## 7. Asal angka di pembahasan dan kesimpulan

Sumber rancangan kesimpulan: `laporan/1_rekap_penelitian/rekap_logbook_17-18_nov_2026-11-18.md` Bagian 10.1.

| Klaim di naskah | Angka | Berkas dan kolom | Dihitung oleh |
|---|---|---|---|
| Latensi DS28E38 82,3 % lebih cepat (5,6x) | 654,88 vs 116,23 ms; rasio 0,1775 | `banding_sw_hw.csv` (Normal, `lat_mean_ms_nyata`, `G_pct`, `rasio_hw_sw`) | `analyse` baris 400-406; `banding_sw_hw` |
| Energi per layanan hemat 92,6 % | 41,03 vs 3,01 mJ | idem, `E_lyn_mJ` | `energi_sesi` 283; `banding_sw_hw` |
| Daya dinamis normal -92,6 %; barrage -58,0 % | 40,10 -> 2,96 mW; 61,18 -> 25,70 mW | idem, `P_dyn_mW` | `energi_sesi` 275 |
| Barrage: software hampir tak tidur | S_cpu 0,38 % vs DS28E38 84,0 % | `deskriptif.csv`, `S_cpu_log` | `analyse` 368-378 |
| TL barrage 15,0 % vs 85,5 % | 15,04 / 85,50 %; model 15,24 / 85,83 % | `layanan.csv` | `layanan()` 245-251 |
| DS28E38 mengerjakan 8,7x lebih banyak sign saat barrage | Theta 8,565 vs 0,983 sign/s | `layanan.csv`, `Theta` | `layanan()` 243 |
| Interaksi tidur CPU +20,5 poin | Delta normal +63,1 -> Delta barrage +83,6 | `interaksi_dd.csv` | `interaksi` 385-403 |
| Latensi tak berubah oleh serangan | selisih 0,03 ms (SW), 0,07 ms (HW) | `pengaruh_barrage.csv` | `pengaruh_barrage` 372 |
| H1a-c, interaksi didukung | p Holm <= 1e-100 | `banding_sw_hw.csv` (`p_holm`, `keputusan`) | `multipletests(..., "holm")` 366 |
| Lantai papan 1,99 mA | I_idle 1,991 / 1,988 mA | `deskriptif.csv` (K1-IDLE, K2-IDLE) | `analyse` + `deskriptif` |
| Kesesuaian dua pengukur S_cpu | bias -0,014 poin, batas -0,055..+0,026 | `bland_altman.csv` | `bland_altman` 201 |

Catatan arti persen: "gain %" = (SW - HW)/SW x 100 untuk metrik yang lebih kecil lebih baik; untuk S_cpu dan TL
dilaporkan sebagai **selisih poin persentase**, bukan persen (`satuan_delta` di `banding_sw_hw.csv`).

### 7.1 Kesenjangan antara paper dan kode (tulis jujur, jangan ditutupi)

| Butir | Paper | Kode | Saran |
|---|---|---|---|
| **Shapiro-Wilk** (3.6.3) | disebut | **tidak ada** | Saya hitung 19 Nov dari `runs.csv`: p < 0,05 pada beberapa kelompok (latensi K3 dan K4 p ~ 0; P_dyn K1 p = 0,010; S_cpu K1 p = 0,037); lainnya normal. Karena itu **Welch + Mann-Whitney** dipakai dan keduanya sepakat. Tambahkan ke naskah satu kalimat, atau masukkan ke pipeline. |
| **ANOVA dua arah** (3.6.3) | disebut | **tidak ada**; interaksi dijawab lewat DD bootstrap | Saya hitung 19 Nov: interaksi cara x skenario latensi p = 0,18 (tidak terbukti); P_dyn p = 6,9e-122; S_cpu p = 4,7e-297. **Sama dengan kesimpulan DD** (Bagian 5 `ringkasan.md`). |
| Kalibrasi shunt (Tabel 3.1 no. 7) | dikalibrasi terhadap multimeter | belum; `--rshunt 0,100`, `u_rel = 5 %` | Nyatakan angka mutlak "belum dikalibrasi"; persen tidak terpengaruh. |
| Batas frame hilang | rusak = 0 | rusak = 0, hilang < 0,25 % pada barrage | Sudah dijelaskan di revisi Bab 3 (butir 26). |
| `N_rusak` | "rusak" | = rusak + hilang | Selalu sebut keduanya terpisah (`rx_bad`, `rx_lost`). |

### 7.2 Keterbatasan yang pasti ditanyakan

1. **K2 mencampur overdrive + polling + Stop** (latensi 116 ms vs 192 ms pada kecepatan biasa): kontribusi tiap faktor tidak dipisah.
2. **P_dyn dan E_lyn hanya sah dalam satu sesi/varian** (dikurangi IDLE sesi itu); jangan dibandingkan lintas varian atau dengan tahap D.
3. Satu papan host dan satu chip DS28E38; jam 48 MHz (bukan 72 MHz).
4. Lantai 1,99 mA didominasi LED papan: daya rata-rata mencerminkan papan ini, bukan chip telanjang.
5. Keabsahan tanda tangan per run tidak diperiksa di PC (firmware hanya lapor sukses); dibuktikan di eksperimen pengembangan (B1: 269/269 valid).

## 8. Naskah menjawab penguji

**"Dari mana angka 92,6 % itu?"**
> "Rata-rata daya dinamis 30 run: software 40,10 mW, DS28E38 2,96 mW. Daya dinamis = daya rata-rata dikurangi daya diam
> (IDLE) pada sesi yang sama. Selisih 37,14 mW dibagi 40,10 = 92,6 %. Hitungannya ada di `run_bench.energi_sesi` dan
> `analisis_d.banding_sw_hw`; saya juga hitung ulang satu run dari CSV mentah tanpa kode itu dan hasilnya sama."

**"Mengapa p-value 1e-113? Bukankah itu mencurigakan?"**
> "Sebaran sangat sempit (CV di bawah 0,1 %) dan kedua kelompok tidak tumpang tindih sama sekali (kolom
> `tumpang_tindih` = tidak). Karena itu saya menekankan ukuran efek dan interval kepercayaan bootstrap, bukan p."

**"Mana uji normalitas dan ANOVA yang Anda tulis?"**
> Jawab sesuai keputusan Anda (Bagian 7.1): hasil Shapiro-Wilk menunjukkan sebagian kelompok tidak normal, karena itu
> saya memakai Welch dengan Mann-Whitney sebagai cek; interaksi diuji dengan bootstrap selisih-dari-selisih dan
> ANOVA dua arah memberi kesimpulan sama.

**"Mengapa angka arus Anda tidak dikalibrasi?"**
> "Shunt berkode 100 mOhm terukur ~82 mOhm. Angka mutlak mA/mW/mJ naik sekitar 20 % setelah dikoreksi, tetapi semua
> perbandingan (gain %, rasio, S_cpu, TL, p) tidak berubah karena penguatan yang sama berlaku pada kedua arsitektur.
> Ketidakpastian 5 % sudah dimasukkan sebagai `u_kal`."

**"Frame yang hilang apakah mengurangi hasil Anda?"**
> "Pada barrage DS28E38 hilang sekitar 1 frame per run (<0,25 %), karena frame yang mulai persis saat CPU pulih dari
> Stop tidak diterima. Ini dilaporkan, bukan disembunyikan. Saya menganalisis ulang K4 pada enam ambang (0 % sampai
> 1 %, 17 sampai 41 run): daya dinamis 25,68-25,70 mW, tidur CPU 83,96-83,97 %, TL 85,46-85,53 %, jadi kesimpulan
> tidak bergantung pada ambang (`tools/3_analisis/analisis_sensitivitas.py`, `laporan/2_hasil_dan_pembahasan/bahan_pembahasan_bab4_2026-11-18.md` bagian 1).
> Frame **rusak** nol."

**"Mengapa angka demo hari ini beda dengan tabel?"** (hasil uji bangku 19 Nov)
> "Jumlah layanan dan latensi sama (K2S normal 116,0 vs 116,2 ms; 60 dari 60 terlayani). Arus rata-rata demo sedikit
> lebih tinggi (K2S normal 3,12 vs 2,55 mA) karena ST-Link terpasang selama demo untuk mem-flash K1<->K2; pada data
> skripsi ST-Link dicabut dan daya direset bersih." *(penyebab pasti belum diuji; ucapkan sebagai dugaan.)*

**"Apakah demo sama dengan pengambilan data?"**
> "Firmware, jendela 60 detik, dan rumus analisis sama. Satu perbedaan prosedur: pada demo, penyerang sudah berjalan
> sebelum jendela dibuka; pada pengambilan data penyerang dimulai sesudah `>START`. Dari 20 run uji, frame rusak 0."

## 9. Latihan (untuk belajar sendiri)

1. Jalankan skrip hitung ulang pada `20261118_124646_K1S/007_K1S_B.csv`. S_cpu = 0,377 % dari T_obs 60,38 s berarti
   CPU tidur total sekitar 0,23 s dalam semenit penuh serangan. Lihat baris `e,` di CSV: kapan saja PB1 turun?
2. Dari `layanan.csv` hitung sendiri rho K4 = 1,165 dan TL model = 1/1,165 = 85,8 %; bandingkan dengan TL terukur 85,5 %.
3. Hitung gain % E_lyn dari `deskriptif.csv` (41,03 vs 3,01) dan cocokkan dengan `banding_sw_hw.csv`.
4. Ubah `--rshunt 0.082` pada `analisis_d.py --stop --keluar data/analisis/uji_r082` dan lihat mana yang berubah
   (mA/mW/mJ naik ~20 %) dan mana yang tidak (gain %, TL, S_cpu, p).
5. Jelaskan dengan kata sendiri mengapa `u_kal` tidak dipakai pada gain %.

**Berkas rujukan**: `tools/1_akuisisi/run_bench.py` (per run), `tools/3_analisis/analisis_d.py` (statistik), `tools/3_analisis/gambar_bab4_stop.py`
dan `tools/3_analisis/dekomposisi_energi.py` (gambar), `tools/3_analisis/hitung_ulang_satu_run.py` (pembuktian independen),
`laporan/2_hasil_dan_pembahasan/panduan_grafik_bab4_2026-11-18.md` (detail tiap gambar), `laporan/2_hasil_dan_pembahasan/ringkasan_bab4_varian_stop.md`.
