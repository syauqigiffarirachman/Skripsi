# 01. Konsep Dasar (mode teach)

Format tiap konsep: **Apa**, **Analogi**, **Di proyek ini**, **Mengapa begitu**, **Cari lebih lanjut**.

---

## A. Kriptografi

### A1. Tanda tangan digital dan ECDSA P-256

**Apa.** Pemilik kunci privat `d` membuat tanda tangan `(r, s)` atas sebuah pesan. Siapa pun yang punya kunci publik
`Q = d x G` bisa memeriksa keasliannya, tetapi tidak bisa memalsukannya. P-256 (secp256r1) adalah kurva elips NIST dengan
kunci 256 bit.

**Analogi.** Stempel pribadi yang cetakannya tidak pernah keluar dari laci. Orang lain hanya bisa mencocokkan hasil
cap dengan contoh resmi (kunci publik).

**Rumus singkat** (cukup tahu bentuknya, hafal alurnya):
1. Hash pesan jadi `z` (di sini digest SHA-256 32 byte).
2. Pilih nonce acak `k`, hitung titik `R = k x G`, ambil `r = x(R) mod n`.
3. `s = k^-1 (z + r d) mod n`. Hasilnya tanda tangan `r || s` (64 byte).
4. Verifikasi: `u1 = z/s`, `u2 = r/s`, titik `u1 G + u2 Q`, koordinat x harus sama dengan `r`.

**Di proyek ini.** Operasi berat ada di langkah 2: perkalian skalar `k x G` di kurva elips, ratusan ribu perkalian
bilangan 256 bit. Inilah yang membuat software butuh 650 ms di 48 MHz, sedangkan chip DS28E38 butuh sekitar 107 ms.

**Cari lebih lanjut.** "ECDSA explained", "elliptic curve scalar multiplication", "secp256r1 parameters".

### A2. Nonce `k`: mengapa paling berbahaya

**Apa.** Jika `k` bocor, atau dipakai dua kali untuk dua pesan, kunci privat `d` bisa dihitung. Kasus nyata: kunci
PS3 Sony bocor karena `k` konstan.

Tiga cara menghasilkan `k`:

| Cara | Prinsip | Kelemahan |
|---|---|---|
| Acak murni | RNG baik | RNG buruk berarti `d` bocor. Mikrokontroler ini tidak punya TRNG |
| RFC 6979 (deterministik) | `k` dari HMAC(d, digest) | Rentan serangan fault injection (hasil sama dua kali) |
| **Hedged** (dipakai K1) | HMAC-DRBG di-seed `entropi \|\| d \|\| digest` | Hasil berbeda tiap sign, tidak bisa dibandingkan byte-per-byte dengan referensi |

**Di proyek ini.** 12 Nov: K1 awalnya RFC 6979. 13 Nov: diganti hedged. Jika entropi ternyata lemah, `k` paling buruk
setara RFC 6979 karena `d || digest` ikut di seed. Sumber entropi (`harvest_entropy`): derau ADC PA0 yang mengambang,
`CYCCNT`, `micros()`, UID chip, penghitung, semuanya di-SHA-256. Kode sendiri menyebut: **bukan TRNG**.

**Konsekuensi uji.** Tanda tangan K1 tidak identik dengan referensi, jadi `tools/k1_check.py` memeriksa dua hal
lain: setiap tanda tangan valid saat diverifikasi di PC, dan semua `r` unik.

**Cari lebih lanjut.** "nonce reuse ECDSA", "RFC 6979", "hedged signatures".

### A3. Blinding dan zeroize (hardening K1)

- **Blinding**: mengacak koordinat titik atau skalar saat menghitung, supaya konsumsi daya tidak berkorelasi dengan `d`.
  Murah dan sudah ada di Mbed TLS. Drbg yang sama dipakai untuk nonce dan blinding.
- **Zeroize**: menimpa memori rahasia dengan nol setelah dipakai. Alasan memakai `mbedtls_platform_zeroize`, bukan
  `memset`: kompiler boleh menghapus `memset` yang hasilnya "tidak dipakai lagi". Fungsi zeroize ditulis agar tidak
  bisa dioptimasi hilang.
- Di K1, `d` tidak lagi variabel global. `d` dimuat dari Flash hanya selama `sign_digest`, lalu dihapus.

### A4. Opsi konfigurasi Mbed TLS dan pengaruhnya (data terukur 13 Nov)

| Opsi | Arti | Efek bila dimatikan |
|---|---|---|
| `MBEDTLS_HAVE_ASM` | perkalian bignum pakai assembly ARM | +5 % (687 vs 655 ms). Kompiler M3 sudah memakai `umull/umlal` sendiri |
| `MBEDTLS_ECP_NIST_OPTIM` | reduksi modulo khusus prima P-256 | **6,6x lebih lambat** (4.332 ms). Pengungkit terbesar |
| `MBEDTLS_ECP_FIXED_POINT_OPTIM` | cache tabel presomputasi titik basis G (comb) | 2,35x lebih lambat (278 -> 655 ms) |
| `MBEDTLS_ECP_WINDOW_SIZE` | ukuran jendela perkalian | tiap kenaikan menambah RAM dan kecepatan |

Mengapa reduksi NIST cepat: prima P-256 `p = 2^256 - 2^224 + 2^192 + 2^96 - 1` berbentuk khusus, sehingga mod p cukup
dengan penjumlahan/pengurangan blok 32 bit, tanpa pembagian bignum.

**Keputusan proyek.** K1 memakai konfigurasi 1c (comb mati, 655 ms). Alasan: baseline "heavy" (4,9 s) tidak
adil, tetapi konfigurasi paling cepat (278 ms) menyembunyikan keunggulan hardware. Angka-angka ini harus disebut
terang-terangan di bab hasil: comb on/off mengubah gambaran dari "DS28E38 19 % lebih cepat" menjadi "66 % lebih cepat".

**Jebakan.** Config dimuat lewat `-DMBEDTLS_CONFIG_FILE`, sehingga SCons tidak melihatnya sebagai dependensi. Ubah
config tanpa `pio run -t clean` berarti library lama tetap dipakai. Hasil 13 Nov sempat salah karena ini.

### A5. Secure element DS28E38

**Apa.** Chip kecil 1-Wire yang menyimpan kunci privat ECDSA P-256 dan menandatangani di dalamnya. Kunci tidak pernah
keluar. Fitur fisiknya (ChipDNA PUF, die shield) hanya dirujuk dari datasheet, **tidak diuji** di skripsi ini.

**Analogi.** Notaris di dalam brankas. Anda menyerahkan dokumen lewat lubang, lima menit kemudian keluar stempelnya.
Anda tidak pernah memegang stempelnya.

**Di proyek ini.** Satu transaksi: reset 1-Wire, Skip ROM, kirim perintah `0x66` (XPC), `len`, `cmd=sign`,
halaman 0 + challenge 32 byte, CRC16, lalu tunggu chip menghitung (~107 ms), lalu baca 64 byte `s || r` + CRC.
Catatan kecil: chip mengirim `s` dulu lalu `r`, kode menukarnya jadi `r || s`.

---

## B. Komunikasi

### B1. 1-Wire

Satu kabel data open-drain dengan resistor pull-up. Master menarik garis rendah dengan durasi tertentu:
pulsa pendek berarti bit 1, panjang berarti bit 0. Presence pulse = "ada perangkat".
Skip ROM (`0xCC`) dipakai karena hanya ada satu chip.

**Mengapa penting untuk energi.** Karena master menghasilkan waktunya sendiri (bit-banging) dan library OneWire
mematikan interupsi saat bit-banging, fase kirim (~24 ms) dan baca (~46 ms) CPU **tidak bisa tidur**.
Hanya penantian chip (~107-130 ms) yang bisa diisi WFI. Hasilnya CPU aktif ~71 ms dari 202 ms per sign.
Efek sampingnya: jitter timer TIM4 di K2 +-50 us, di K1 hanya 4 us.

**Overdrive**: mode 1-Wire berkecepatan tinggi. Percobaan 11 Nov gagal (library tidak mendukung), 18 Nov berhasil.
Bahan malam ini.

### B2. UART, DMA, dan "satu frame = satu bangun CPU"

UART kirim byte demi byte. Tanpa DMA, tiap byte membangunkan CPU (36 kali per frame). Dengan DMA, hardware menyalin
byte ke RAM sendiri, dan CPU hanya diinterupsi **sekali** saat 36 byte lengkap (Transfer Complete). Itu penting untuk
metrik tidur: paper mensyaratkan satu frame = satu bangun.

Frame attacker 36 B: `[0]=0xA5` penanda awal, `[1..2]` nomor urut, `[3..34]` isi, `[35]` XOR semua byte sebelumnya.
Nomor urut memungkinkan menghitung frame hilang dari selisihnya. XOR menangkap bit rusak (bukan keamanan, hanya
deteksi galat).

### B3. USB CDC dan klon STM32

USB CDC membuat papan tampak sebagai port serial (COM). Dua jebakan nyata di proyek:
1. Logger klon punya UID nol, core mengisi nomor seri USB dari UID, jadi Windows menolak enumerasi. Solusi: patch
   string nomor seri.
2. Jalur OUT (PC ke logger) klon gagal. Solusi cerdik: kirim perintah lewat **nilai baud** (SET_LINE_CODING), karena
   jalur kontrol EP0 tetap bekerja. `baud = 1.000.000 + cmd x 1000 + arg`.

---

## C. Mikrokontroler dan waktu

### C1. Mode tidur Cortex-M3 (STM32F103)

| Mode | Yang berhenti | Bangun | Arus (papan ini) |
|---|---|---|---|
| Run | tidak ada | - | ~13-19 mA @48 MHz |
| **Sleep (WFI)** | hanya clock inti | interupsi apa pun | ~11,5-12 mA (tahap D) |
| **Stop** | semua clock 1,8 V (PLL, HSI, HSE) | EXTI/RTC alarm | ~2 mA (didominasi LED PWR) |

Tahap D memakai Sleep, karena itu "IDLE" masih 11,6 mA: PLL, bus, dan periferal tetap menyala. Stop adalah varian yang
ditambahkan 17 Nov. Bangun dari Stop mengembalikan jam ke HSI 8 MHz, PLL harus dinyalakan ulang.

### C2. Pola WFI yang bebas race (inti K1/K2 jendela)

```
__disable_irq();              // PRIMASK=1
if (!g_pending && !g_end) {   // periksa kerja baru DI DALAM penutup interupsi
    ... matikan SysTick ...
    __WFI();                  // tetap bangun oleh interupsi pending walau PRIMASK=1
    ... nyalakan SysTick ...
    __enable_irq();           // ISR yang menunggu berjalan DI SINI
}
```

**Masalah yang dipecahkan (lost wakeup).** Jika flag diperiksa lalu interupsi datang tepat sebelum `WFI`, CPU
tidur padahal ada kerja menunggu. Dengan PRIMASK=1, interupsi yang datang tidak dijalankan, tetapi tetap berstatus
pending, dan WFI langsung bangun karenanya. Setelah itu `__enable_irq()` membiarkan ISR berjalan.

**`__ISB()` setelah `__enable_irq()`**: Cortex-M3 bisa menunda masuk ISR satu instruksi, sehingga pembacaan flag
persis setelahnya bisa terlewat. Temuan 13 Nov.

**SysTick dimatikan** selama tidur (tik 1 ms akan membangunkan CPU 1000 kali/s). `uwTick` dikoreksi sebesar durasi
tidur supaya timeout HAL (mis. `analogRead`) tetap benar.

### C3. DWT CYCCNT dan jam yang tidak sempurna

`CYCCNT` menghitung siklus clock inti, tetap berjalan saat WFI, tetapi berhenti di Stop. Latensi sign =
`(cycles_akhir - cycles_awal) / 48 MHz`. Selisih bilangan bulat tak bertanda aman terhadap satu kali limpahan.

**Masalah besar yang ditemukan.** Host memakai HSI (osilator RC internal, tanpa kristal). Terukur terhadap kristal
logger: **-5.465 ppm (-0,547 %)**, jadi HSI efektif 47,739 MHz. Konsekuensi: semua waktu host harus dikoreksi dengan
jam logger. Latensi K1: 652,2 ms (nominal) menjadi 655,5 ms (nyata). Karena itu T_obs selalu diambil dari logger.

**Analogi.** Dua jam tangan, satu murah (host) dan satu presisi (logger). Untuk stopwatch lomba, pakai jam presisi.

### C4. Timer berantai: waktu 32 bit 1 us tanpa interupsi

TIM2 (16 bit) dibagi sampai 1 MHz; saat overflow, TRGO memicu TIM3 (mode clock eksternal) yang menjadi 16 bit atas.
Gabungan `(TIM3 << 16) | TIM2` = penghitung 32 bit tiap 1 us (habis dalam 71 menit). Tidak ada ISR, jadi tidak
membangunkan CPU, dan tetap berjalan saat WFI. `hw_us()` membaca ulang bila bagian atas berubah di tengah pembacaan.

---

## D. Pengukuran

### D1. INA226 dan shunt

Sensor arus/daya I2C. Arus mengalir lewat resistor kecil (shunt); INA226 mengukur tegangan di ujungnya
(`V = I x R`, resolusi 2,5 uV per LSB). Dengan R = 0,1 ohm, 1 LSB = 25 uA.

Hal-hal penting yang ditemukan:
- Alamat nyata 0x44 (paper menulis 0x40).
- Laju cuplik nyata 495,75/s (bukan 454/s): osilator internal modul ini ~8 % lebih cepat dari nominal. Maka integrasi
  energi **wajib** memakai cap waktu nyata (integrasi trapesium), bukan selang nominal.
- Shunt berkode R100 ternyata efektif 0,080-0,083 ohm. Dibuktikan dengan dua resistor beban berbeda, dan
  faktor skalanya tetap (linear). Dampak: nilai absolut mA/mW bergeser ~20 %, **perbandingan antar kelompok tidak
  berubah**. Data mentah (tegangan shunt) disimpan, sehingga tinggal dihitung ulang dengan `--rshunt`.
- Offset -2 LSB (-50 uA) dikurangkan.

### D2. Metrik

| Simbol | Rumus | Makna |
|---|---|---|
| `S_cpu` | `T_sleep / T_obs` | porsi waktu CPU tidur |
| `P_idle` | rata-rata daya run IDLE (tanpa permintaan) di blok yang sama | dasar papan |
| `P_dyn` | `P_avg - P_idle` | daya tambahan akibat beban |
| `E_lyn` | `P_dyn x T_obs / N_lyn` | energi dinamis per tanda tangan selesai |
| `I_dyn`, `Q_lyn` | versi arus (bukan daya) | lebih stabil terhadap tegangan USB |
| `TL` | `N_lyn / N_dtg` | tingkat layanan |
| `t_sign` | durasi sign (CYCCNT, dikoreksi jam) | latensi |
| `Theta` | `N_lyn / T_obs` | throughput |
| `Theta_max` | `1 / t_sign` | kapasitas = ambang jenuh |

**Mengapa dikurangi IDLE.** Papan itu sendiri (regulator, LED, pull-up) menarik arus tetap ~11-12 mA. Yang ingin
dibandingkan adalah biaya beban kerja, bukan papan. Selain itu arus IDLE bergeser antar sesi ~1 % mengikuti tegangan
USB; mengurangkan IDLE blok yang sama menghapus pergeseran itu (E_lyn dua sesi beda hanya 0,1-0,4 %).

### D3. Desain eksperimen

Faktorial 2x2: arsitektur (software, hardware) x skenario (A normal 1/s, B barrage 10/s):
K1 (SW,A), K2 (HW,A), K3 (SW,B), K4 (HW,B), masing-masing 30 run x 60 s = 120 run.

**Mengapa urutan diacak penuh dibatalkan.** Mengacak per run butuh flash ulang, dan flash butuh ST-Link terpasang.
ST-Link terbukti mengubah arus terbaca 0,03-0,11 mA, dan OpenOCD menyalakan DBGMCU_CR (jam inti tetap jalan saat WFI,
+0,2-0,7 mA, bertahan sampai power-on reset). Jadi diganti **blok per arsitektur** (pola ABBA) dengan urutan A/B
diacak di dalam blok dan IDLE mengapit tiap blok.

**Aturan keutuhan data.** Run yang gagal pemeriksaan otomatis (baris CSV hilang, checksum, selang cuplik > 5 ms,
galat I2C, frame rusak, S_cpu host vs logger > 0,5 poin) **digugurkan dan diganti**, bukan ditambal.

### D4. Statistik yang dipakai

| Alat | Untuk apa | Intuisi |
|---|---|---|
| Welch t-test | bandingkan dua rerata, varians boleh beda | selisih rerata dibagi galat baku gabungan |
| Mann-Whitney U | cek tanpa asumsi normal | membandingkan peringkat |
| Koreksi Holm | 6 perbandingan sekaligus | menekan peluang "kebetulan signifikan" |
| Bootstrap IK 95 % | rentang ketidakpastian | resampel 10.000 kali dari data sendiri |
| Selisih-dari-selisih (interaksi) | apakah manfaat HW berbeda antara A dan B | (K4-K3) dibanding (K2-K1) |

**Catatan penting.** Sebaran antar run kecil sekali (CV 0,01-0,07 %), jadi p-value pasti sangat kecil dan itu
tidak informatif. Yang dilaporkan: ukuran efek dan persen gain. `TL` dan jumlah layanan hampir deterministik
(varians ~0): dilaporkan sebagai nilai tetap + validasi model, bukan t-test.

### D5. Model kejenuhan (cara menjelaskan hasil barrage)

Jika sign butuh `t` detik dan permintaan datang `lambda` per detik:
- `lambda < 1/t`: semua dilayani, `TL = 100 %`, CPU masih sempat tidur.
- `lambda > 1/t`: `TL ~ 1/(lambda t)`; server jenuh.

Software: `1/0,6555 = 1,53/s`. DS28E38: `1/0,2022 = 4,95/s` (3,2 kali). Skenario B (10/s) membuat keduanya jenuh, tetapi
**jenuh tidak sama dengan terjaga**: DS28E38 jenuh namun CPU masih tidur 64,7 % karena menunggu chip dalam WFI.
Inilah inti temuan "race-to-sleep".
