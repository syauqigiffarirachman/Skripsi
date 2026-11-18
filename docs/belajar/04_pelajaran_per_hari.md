# 04. Pelajaran per Hari (10-17 Nov 2026)

Format tiap hari: **Dikerjakan**, **Mengapa**, **Bug/temuan**, **Pelajaran**.

---

## 10 Nov: Chip DS28E38 bisa diajak bicara

- **Dikerjakan.** Pustaka Python (`ds9481_comm/`) untuk bicara ke DS28E38 dan DS2476 via adaptor USB DS9481P-300;
  `sign.py` (lihat ID, tanda tangani, verifikasi tanpa chip); versi C (`ds9481_comm/c/`) yang tanda tangannya
  saling cocok dengan versi Python; firmware STM32 awal.
- **Mengapa.** Sebelum membandingkan, harus bisa membuktikan chip bekerja dan tanda tangannya valid secara independen.
- **Pelajaran.** Operasi permanen dan tidak bisa dibatalkan (mis. mengunci memori chip) **sengaja dikunci** di kode.
  Pada perangkat keras sekali-tulis, kunci pengaman adalah bagian desain.
  Verifikasi silang dua implementasi (Python vs C) menangkap salah paham protokol lebih cepat.

## 11 Nov: Mempercepat chip, menyiapkan pembanding, alat ukur

- **Dikerjakan.** DS28E38: tunggu tetap 520 ms diganti polling, alamat ROM disimpan, hasil ~190 ms (dari ~600 ms).
  Baseline Mbed TLS "heavy" (4,9 s), micro-ecc (~260 ms). Pin penanda TRIG/SLEEP/WINDOW. Emulator hitung siklus.
- **Mengapa.** Hasil perbandingan sangat bergantung pada pustaka software yang dipilih (4,9 s vs 0,26 s). Perlu
  keputusan sadar dan dinyatakan di laporan.
- **Temuan.** Papan macet bila antrean RX USB penuh (HardFault). Solusi saat itu: attacker kirim satu permintaan per
  kesiapan papan. Akhirnya digantikan UART + DMA yang tidak punya masalah ini.
- **Pelajaran.** "Tidak ada acuan tunggal yang benar." Angka perbandingan adalah fungsi dari konfigurasi lawannya,
  jadi konfigurasi harus ditulis eksplisit.

## 12 Nov: Dari paper ke kode, K1 tahap 1, attacker tahap 1

- **Dikerjakan.** Membaca paper v2 dan v3 menjadi daftar kebutuhan. Aturan **build from base**. K1 tahap 1:
  ECDSA standar (RFC 6979 + optimasi upstream) 276 ms, 30/30 valid. Attacker F030C8: pembangkit frame TIM3.
- **Bug dan temuan.**
  1. `Serial` alias `Serial2` di varian F030 generik (dua objek satu periferal, konsol diam).
  2. `Serial1` tidak aktif otomatis; butuh `-DENABLE_HWSERIAL1`.
  3. Tik palsu di awal mode (refresh paksa memicu ISR).
  4. `stm32flash` yang macet menahan handle COM sampai VS Code di-restart penuh.
- **Pelajaran.** Verifikasi laju memakai `millis()` milik board itu sendiri, bukan waktu PC, karena latensi USB
  melebarkan waktu PC. **Ukur dengan jam dari sisi yang diukur** (atau saksi independen).

## 13 Nov: Penerima, hardening K1, perburuan latensi, RAM, WFI, DMA

Hari terpadat.

1. **Penerima frame tahap 1** lulus. Sebanyak 100 frame awal hilang karena `setup()` menunggu USB 10 s.
   Pelajaran: `setup()` tidak boleh memblokir menunggu USB.
2. **Hardening K1** (hedged + zeroize): sign 276 ke 279 ms (+0,8 %).
3. **Comb mati**: 279 ms menjadi 655 ms. **Kesimpulan awal yang SALAH** ("comb tidak mengubah latensi") dicabut: build
   pertama memakai library lama karena config tidak dipantau SCons. Aturan: `-t clean` setelah mengubah config.
4. **Dampak tiap opsi** dengan build bersih (tabel di `01_konsep_dasar.md` A4).
5. **RAM puncak** K1 9.644 B vs K2 5.368 B (metode cat stack identik untuk keduanya).
6. **K2 dibangun ulang dari base**: ROM search sekali (hemat ~23 ms), tanpa retry, 200,9 ms.
7. **WFI + pin status + S_cpu** (K1 dan K2): K1 A 36 %, K2 A 93 %; barrage K1 0,3 %, K2 64,7 %.
   Temuan: SOF USB membangunkan WFI 1000x/s, CYCCNT tetap jalan saat WFI, M3 menunda ISR satu instruksi (`ISB`).
8. **DMA + resync**: bug DMA "terkunci" ditemukan lewat uji paksa (lihat `03_penjelasan_kode.md` 3.3).
9. **Logger L1**: ST-Link `X` gagal konek berhari-hari. Bukti gagalnya mengikuti set kabel, bukan board.
   Koreksi kesimpulan sendiri: "tegangan target terbaca 3,2x V" **bukan** bukti koneksi.

**Pelajaran 13 Nov.**
- Setiap kesimpulan awal yang salah dicatat dan dicabut. Itu kekuatan logbook.
- Bug yang hanya muncul pada jalur langka butuh uji paksa.
- Jangan membandingkan dua firmware dengan metode ukur berbeda (mis. K2 lama mengukur stack setelah `printf`).

## 14 Nov: Logger, F030 sementara lalu F103, INA226, USB klon

- **Logger sementara F030C8**, lalu F103 pesanan tiba. Refaktor ke `src/logger_common/` (kode bebas-chip) supaya
  berpindah chip murah.
- **INA226**: alamat 0x44 (bukan 0x40), laju 495,75/s (bukan ~450), VBUS belum tersambung sehingga akurasi arus belum diuji.
- **`cycles()` mundur di F030** (tanpa DWT, dari SysTick + COUNTFLAG): rekam berakhir seketika. Diganti pola
  baca-ulang (`tik ms, VAL, tik ms` sampai sama). Uji monotonik 300.000 panggilan beruntun, mundur = 0.
  Catatan: dugaan penyebab COUNTFLAG tidak dibuktikan; yang terbukti hanya versi baru monoton.
- **USB klon F103**: UID nol, nomor seri kosong, Windows menolak. Perbaikan `usb_serial_fix()`. Jalur IN bekerja,
  jalur OUT gagal, perintah dikirim lewat baud.
- **L4 input-capture**: TIM2 F103 ternyata 16 bit (bukan 32 bit), diperluas 64 bit dengan pencacah overflow.
  Penanganan balapan overflow vs capture: bila UIF aktif dan nilai capture < 0x8000, overflow dianggap sudah terjadi.

**Pelajaran.** Asumsi spesifikasi harus diverifikasi (TIM2 32 bit adalah dugaan, ternyata keliru).
Catatan jujur: bagian L4 hari itu ditulis dari kode, **tanpa hasil uji**, dan ditandai begitu. Jangan dibaca "lulus".

## 15 Nov: Hari validasi dan pengambilan data

1. **L4 gagal lalu lulus**: HIGH 2 us, duty 0 %. Dugaan "klon F103 tidak bisa capture tepi turun" **salah**.
   Penyebab sebenarnya kabel PB1 host masih menancap di PA0. Setelah dicabut: HIGH 2999,944 us, nol selisih.
2. **Galat jam host**: -5.465 ppm (-0,547 %), 11 menit rekaman, tren +0,9 ppm (tidak bergeser).
3. **INA226**: VBUS disambung (bus 5,04 V), offset -50 uA, **shunt efektif ~0,080-0,083 ohm** (dibuktikan dengan dua resistor).
4. **Jalur kendali run** (host_link, logger L5/L6, `run_bench.py`) lulus 13 run. Uji galat 13/13 (host dihentikan
   lewat OpenOCD, breakpoint di `>DONE`, `g_ready` dinolkan lewat SWD).
5. **DBGMCU_CR**: OpenOCD menyalakan DBG_SLEEP, arus tidur naik 0,2-0,7 mA, bertahan sampai POR.
   Prosedur: `run_bench.py nodebug`.
6. **Pengacakan per run** lulus teknis lalu **tidak dipakai** (butuh ST-Link; mengubah arus). Diganti blok.
7. **Integrasi 8/8**, **pilot 16/16** (CV arus <= 0,07 %).
8. **Metrik TL** ditambahkan, model kejenuhan terbukti (selisih <= 0,2 poin).
9. **Tahap D**: 120 run. Dua gangguan: baris CSV hilang (USB CDC logger membuang data bila paket belum diambil PC dalam
   3 ms, perbaikan `-DUSB_CDC_TRANSMIT_TIMEOUT=250` build L6B1), dan pergeseran arus +0,78 mA setelah cabut-pasang
   USB (host terjebak setengah-reset, hilang dengan cabut-colok daya).
10. **Analisis**: `analisis_d.py`. Hasil utama: latensi -69,2 %, I_dyn -78 % / -37 %, TL 15 % ke 49 %.

**Pelajaran 15 Nov.**
- **Kalibrasi lewat dua resistor** (rasio R_A/R_B = 1,509 vs nominal 1,500): kesalahan berbentuk faktor skala tetap,
  bukan hambatan seri tetap. Teknik inferensi yang bagus.
- **Ketidakpastian tidak simetris**: pergeseran antar sesi (sistematis) jauh lebih besar dari derau antar run (acak).
  Karena itu desain blok + pengurangan IDLE, bukan sekadar memperbanyak run.
- **Aturan sah/gugur ditetapkan sebelum melihat hasil akhir** (selang cuplik < 5 ms, late <= 10), dengan bukti
  bahwa run "late" tidak berbeda arusnya (selisih <= 0,008 mA).

## 16 Nov: Refaktor aman, analisis sesuai paper v4

- **Refaktor host** ke `src/host_common/`; verifikasi **biner identik bit per bit** (`firmware.bin` sama,
  ELF berbeda hanya info debug). Lanjut ke perapian per board (tahap 1-5) dengan gerbang yang sama.
- **Hal yang diam-diam menggeser tata letak Flash**: kode `.cpp` bersama di `lib/`, `lib_archive = no`, urutan path
  berkas di `src/`. Ketiganya dicoba lalu dibatalkan.
- **Analisis ditulis ulang** sesuai paper v4 sub-bab 3.6, lalu **disederhanakan** (Welch utama, Mann-Whitney cek).
  Tambah ketidakpastian gabungan U_gab (u_acak, u_kal, u_blok, u_idle).

**Pelajaran.** Merapikan kode bukan alasan untuk mengubah perilaku. Gerbang objektif (hash biner) menggantikan
"rasanya sama".

## 17 Nov: Rencana deep sleep (Stop mode), langkah 1-5 dan awal 6-7

- Keputusan: Stop ditambahkan sebagai varian, tahap D tetap. Lantai Stop ~2 mA diterima (LED PWR dipertahankan).
- **S0a (LSE)**: kristal 32,768 kHz start di 23/23 uji, start dingin 3,5-4,1 s, selisih -47 ppm tetap terhadap logger.
  Reset biasa **bukan** uji start dingin (kristal masih bergetar); dibuat varian dengan LSE mati 15 s.
- **S0 (lantai Stop)**: 1,985 mA tanpa ST-Link dan setelah POR bersih; dengan ST-Link 2,567 mA.
  Domain debug menyumbang ~0,58 mA.
- **Daya balik**: host tetap berdaya lewat kabel sinyal setelah USB-C dicabut. Mitigasi: resistor seri 10 kohm.
  Rail terkunci ~1,92 V (ambang POR/PDR ~1,9 V).
- Langkah 6 (alarm RTC) dan 7 (bangun dari frame attacker) lulus malam itu. Detail lengkapnya dibahas di catatan 18 Nov.

**Pelajaran.** Untuk mengukur arus rendah, "apa yang ikut mengalir" (LED, ST-Link, daya balik lewat I/O)
bisa lebih besar daripada chip yang diukur. Inventarisasi dulu semua jalur arus.

---

## Pola kesalahan yang berulang (checklist debug)

| Gejala | Cek lebih dulu |
|---|---|
| Hasil ukur aneh | kabel fisik, lalu jam/tegangan, baru kode |
| Hasil lama tetap muncul setelah ubah config | build bersih (`-t clean`) |
| Perangkat hilang/tidak konek | tukar satu variabel per percobaan (kabel, dongle, board) |
| Bug hanya di jalur langka | uji paksa (misalnya sengaja geser 1 byte) |
| Angka bergeser antar sesi | catat tegangan USB, gunakan baseline IDLE blok yang sama |
| Dua alat ukur beda | tambah saksi ketiga; jangan memilih yang sesuai harapan |
