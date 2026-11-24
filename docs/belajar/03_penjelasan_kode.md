# 03. Penjelasan Kode (mode teach)

Format: potongan kode, **apa**, **mengapa begitu**, **alternatif yang ditolak**, **kuncinya**.
Nomor baris merujuk berkas per 18 Nov 2026 pagi, bisa bergeser bila kode berubah.

---

## 1. K1 software: `src/host/dev/tahap_d/k1_software/main.cpp`

### 1.1 Mengumpulkan entropi (`harvest_entropy`, baris 97)

```cpp
for (int i = 0; i < 64 && ret == 0; i++) {
  uint32_t v[3] = { (uint32_t)analogRead(PA0), cycles(), micros() };
  ret = mbedtls_sha256_update(&h, (const unsigned char *)v, sizeof v);
}
... UID chip (12 B) ... penghitung sign ...
mbedtls_sha256_finish(&h, out);
```

- **Apa.** 64 kali: baca ADC pin PA0 yang sengaja dibiarkan mengambang, siklus CPU, dan mikrodetik; semua diumpankan
  ke SHA-256, ditambah UID unik chip dan penghitung. Keluarannya 32 B "benih".
- **Mengapa.** F103 tidak punya generator angka acak sejati (TRNG). Derau ADC pin mengambang + jitter waktu adalah
  sumber ketidakpastian fisik; SHA-256 meratakan distribusinya. UID dan penghitung memastikan dua benih tidak pernah
  sama walau derau kebetulan sama.
- **Alternatif ditolak.** `rand()` / xorshift (deterministik, tidak aman untuk nonce).
  RFC 6979 murni (aman tanpa RNG, tetapi tidak tahan fault injection).
- **Catatan jujur.** Kode memberi label "BUKAN TRNG". Jangan mengklaim entropi kriptografis penuh di skripsi.
- **Kunci.** Pengumpulan entropi ada **di luar** jendela ukur latensi, padahal DS28E38 sudah memakai TRNG internalnya di
  dalam jendela. Perbedaan ini harus disebut saat membandingkan (terukur: sekitar 4-6 ms, Bland-Altman K1 bias +6,06 ms).

### 1.2 Satu operasi tanda tangan (`sign_digest`, baris 126)

```cpp
memcpy(seed, ent, 32);  memcpy(seed+32, PRIVATE_KEY, 32);  memcpy(seed+64, digest, 32);
ret = mbedtls_hmac_drbg_seed_buf(&drbg, md, seed, sizeof seed);   // nonce hedged
ret = mbedtls_mpi_read_binary(&d, PRIVATE_KEY, 32);
ret = mbedtls_ecdsa_sign(&g_grp, &r, &s, &d, digest, 32, mbedtls_hmac_drbg_random, &drbg);
...
mbedtls_platform_zeroize(seed, sizeof seed);
mbedtls_hmac_drbg_free(&drbg);   // free = zeroize
mbedtls_mpi_free(&d); ...
```

- **Apa.** Benih DRBG = `entropi || d || digest` (96 B). DRBG yang sama jadi sumber nonce `k` **dan** blinding,
  karena `mbedtls_ecdsa_sign` memakai fungsi RNG yang diberikan untuk keduanya. Setelah selesai, semua rahasia dihapus.
- **Mengapa `__attribute__((noinline))`.** Fungsi ini dibungkus pembacaan `cycles()`. Jika kompiler meng-inline dan
  menggeser kode ke luar kurung `t0 ... cyc`, pengukuran bisa meleset. `noinline` menjaga batas pengukuran.
- **Mengapa `d` dimuat hanya di sini.** Meminimalkan waktu rahasia berada di RAM (bila RAM dibaca penyerang lewat
  fault/debug, jendelanya sempit).
- **Kunci.** Kunci privat uji memang statis di Flash (memodelkan ancaman software-based). Komentar kode menegaskan:
  bukan untuk produksi. Justru inilah motivasi memakai DS28E38 (kunci tidak pernah di STM32).

### 1.3 Perintah `R` dan `M`

`R[n]` menandatangani n kali dan mencetak `i, cyc_sign, ms, r||s`. Urutan per iterasi: ukur lalu cetak (pencetakan
di luar kurung ukur). `M[n]` mengukur RAM puncak: statik (`_end - _sdata`) + heap (`sbrk(0) - _end`) + stack
(cat memori dengan `0xA5A5A5A5`, jalankan satu operasi, lihat sampai mana yang tertimpa). Hasil: K1 9.644 B (47 %),
K2 5.368 B (26 %).

**Teknik "stack painting"**: stack tumbuh ke bawah dari `_estack`; menandai area bebas dengan pola khusus lalu
memeriksa titik terendah yang berubah memberi stack puncak tanpa debugger. **Cari:** "stack painting watermark".

---

## 2. Jendela pengamatan dan tidur: `cmd_window` (K1, baris 276)

### 2.1 Persiapan

```cpp
static HardwareTimer t4(TIM4);          // statis: tanpa alokasi heap
t4.setOverflow(secs * 1000000UL, MICROSEC_FORMAT);   // satu tembakan = akhir jendela
...
USB->CNTR = usb_cntr & ~(USB_CNTR_SOFM | USB_CNTR_ESOFM);   // mask SOF
```

- Timer `t4` dibuat `static` agar tidak memakai heap (heap dipakai Mbed TLS dan diukur terpisah).
- **Mask interupsi SOF USB.** Bila host tersambung ke PC lewat USB CDC, setiap 1 ms ada Start Of Frame yang
  membangunkan `WFI` (1000 bangun per detik, S_cpu idle hanya 99,14 %). Di bangku ukur sebenarnya USB-C hanya daya,
  jadi interupsi itu tidak ada; mask meniru kondisi itu saat pengujian lewat USB. Dipulihkan setelah jendela.

### 2.2 Loop utama (baris 310-330)

```cpp
for (;;) {
  __disable_irq();
  if (!g_pending && !g_end) {
    act_cyc += (uint32_t)(cycles() - seg);   // tutup segmen aktif
    PIN_STATUS_LO();                          // PB1 = 0: akan tidur
    if (GPIOB->IDR & GPIO_IDR_IDR1) pin_salah++;   // baca balik, harus 0
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    const uint32_t ts = hw_us();
    __DSB(); __WFI();
    const uint32_t tw = hw_us();
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    PIN_STATUS_HI();                          // PB1 = 1: bangun
    ...
    t_sleep += tw - ts; wfi_n++;
    uwTick += ...;                            // koreksi tik HAL
    __enable_irq(); __ISB();
    if (!g_pending && !g_end) spurious++;     // bangun tanpa kerja
    continue;
  }
  ... layani permintaan ...
}
```

- **Dua jam, satu kebenaran.** `act_cyc` dihitung dari CYCCNT (jam inti), `t_sleep` dari timer 1 us. Pemeriksaan
  konsistensi: `T_obs - T_sleep` harus sama dengan `act_cyc` (sisa 2-107 us pada jendela 6-60 s). Dua jam
  independen yang cocok menandakan akuntansi benar.
- **`__DSB()` sebelum `WFI`.** Memastikan penulisan sebelumnya (mis. ke GPIO) selesai sebelum tidur.
- **`spurious`.** Bangun tetapi tidak ada pekerjaan baru. Nilai 0 menandakan perilaku tidur bersih.
- **Kunci.** Struktur "periksa-dalam-penutup-lalu-tidur" menjadi template yang sama di K2 (`sleep_while`) dan varian
  Stop (`wait_event`).

---

## 3. Penerima frame: `src/host/dev/tahap_d/bersama/host_rx.h`

### 3.1 DMA sekali per frame

```cpp
DMA1_Channel6->CPAR = (uint32_t)&USART2->DR;      // sumber: register data UART
DMA1_Channel6->CMAR = (uint32_t)g_rx;             // tujuan: buffer 36 B
DMA1_Channel6->CCR  = DMA_CCR_MINC | DMA_CCR_TCIE;// naikkan alamat tujuan, interupsi saat selesai
```

DMA menyalin tiap byte UART ke `g_rx[]`. Setelah 36 byte, interupsi TC berbunyi **sekali**. CPU hanya bangun untuk
satu frame, bukan 36 byte.

### 3.2 Validasi di ISR (`DMA1_Channel6_IRQHandler`)

```cpp
for (i < FRAME_LEN-1) x ^= g_rx[i];
if (g_rx[0] == FRAME_SOF && x == g_rx[FRAME_LEN-1]) {
  seq = g_rx[1] | (g_rx[2] << 8);
  if (g_rx_have) g_rx_lost += (uint16_t)(seq - (uint16_t)(g_rx_last_seq + 1));
  ... g_arrived++; g_pending = true;
}
```

- `(uint16_t)(seq - (last+1))` menangani limpahan nomor urut 65535 ke 0 dengan benar.
- `g_pending` adalah **satu** flag. Dua frame yang datang saat host sibuk hanya menyalakannya dua kali = satu sign
  tambahan. Inilah mekanisme "terlewat".

### 3.3 Bug DMA terkunci (pelajaran terbesar 13 Nov)

Jika frame tidak valid (geser 1 byte), kode harus **resync**: buang byte sampai garis diam (IDLE) lalu pasang ulang DMA.
Versi awal gagal total: semua frame berikutnya `bad`, karena byte pertama tiap frame = byte XOR frame sebelumnya.

**Akar masalah.** Byte ke-36 tiba saat kanal DMA nonaktif, permintaan DMA dari USART "terkunci", dan saat kanal
dipasang ulang DMA membaca `DR` basi. **Perbaikan:** matikan `DMAR` (`CR3 &= ~DMAR`) selama resync, nyalakan lagi
**sesudah** `rx_arm()`.

**Pelajaran lebih besar.** Bug ini tidak muncul di uji normal (`bad = 0`). Ia hanya terlihat karena ada uji paksa
(`-DK1_RX_TEST_MISALIGN`, sengaja menggeser 1 byte). **Uji jalur galat, bukan hanya jalur bahagia.**

---

## 4. K2 hardware: `src/host/dev/tahap_d/k2_hardware/main.cpp`

### 4.1 Satu transaksi XPC (`xpc`, baris 96)

```cpp
if (!ds.reset()) return 1;            // presence pulse
ds.skip();                             // Skip ROM
const uint8_t hdr[3] = { XPC_CMD, plen+1, cmd };
for (i<3) ds.write(hdr[i], 1);         // power=1: jalur ditahan HIGH (pull-up kuat)
for (i<plen) ds.write(par[i], 1);
ds.read_bytes(crc, 2);                 // CRC16 dari chip
ds.write(0xAA, 1);                     // "lepas": chip mulai menghitung
if (crc cocok? ... ) return 2;
wait_ms(dly_ms);                       // tunggu chip
(void)ds.read();                       // byte dummy
len = ds.read() + 2;  result = ds.read();  ...  baca data + CRC
```

- **Mengapa `power=1`.** DS28E38 selalu parasit (hanya IO + GND). Saat menghitung ia butuh energi dari garis data,
  sehingga garis ditahan tinggi dengan pull-up kuat selama ~107 ms. Konsekuensi: **arus tidur K2 saat barrage 15,3 mA
  > 12,8 mA** saat normal; chip menarik arus walau CPU tidur.
- **Kode galat bertingkat** (1 presence, 2 CRC perintah, 3 panjang, 4 CRC balasan, 5 byte hasil) memudahkan diagnosis.
- **Tanpa retry otomatis** (keputusan K2 tahap 1): kegagalan dicetak dan dihitung. Retry menyembunyikan masalah
  dan menggembungkan latensi.
- **`wait_ms` ada di luar jendela dan di dalam jendela:** di luar = `delay()`, di dalam = tidur `WFI` dibangunkan TIM1.

### 4.2 Tidur menunggu chip (`wait_ms`, baris 264)

```cpp
g_wait_done = false;
g_t1->setOverflow(ms * 1000UL, MICROSEC_FORMAT);
g_t1->resume();
while (!g_wait_done) sleep_while(idle_for_chip);
```

`sleep_while(idle)` menerima **fungsi pemeriksa** (`idle_for_request` atau `idle_for_chip`). Satu mekanisme tidur
dipakai untuk dua alasan bangun. Akuntansi tidur terpusat, jadi `S_cpu` menghitung keduanya dengan cara sama.

**Latensi di dalam jendela 201,25 ms vs mode `R` 200,90 ms.** Selisih 0,3 ms = ongkos konfigurasi dan bangun TIM1.

### 4.3 Perbedaan yang harus dinyatakan di laporan

| Hal | K1 | K2 |
|---|---|---|
| Yang di-hash | digest SHA-256 langsung | chip meng-hash ROM, halaman, challenge, MANID |
| Entropi | di luar jendela | di dalam chip (TRNG) |
| Tidur dalam sign | tidak ada | WFI saat tunggu chip |
| RAM puncak | 9.644 B | 5.368 B |

---

## 5. Attacker: `src/attacker/main.cpp`

- **Frame dibangun statis**: isi `0x30 + i` agar mudah dikenali saat debug. Isi tidak dipakai host.
- **ISR hanya menaikkan flag** (`onTick`: `g_sendFlag = true`), pengiriman UART dilakukan di `loop()`. Prinsip: ISR
  singkat, kerja berat di luar ISR.
- **`g_skipFirstTick`**: `HardwareTimer::refresh()` memaksa update event yang langsung memicu ISR, sehingga frame
  pertama terkirim di t=0, bukan t=T. Flag ini membuang tik palsu pertama.
- **Bug makro Serial** (12 Nov): varian generik F030C8 memetakan `Serial` ke `Serial2`. Menulis `Serial` polos
  lalu membuat objek lain di pin yang sama = dua objek berebut satu periferal. Solusi: selalu eksplisit
  (`#define Console Serial2`, `#define HostLink Serial1`).

---

## 6. Protokol sebagai satu sumber kebenaran: `lib/bench_proto/`

`proto_attacker_host.h` memuat `FRAME_LEN`, `FRAME_SOF`, `FRAME_BAUD` + `static_assert(FRAME_LEN == 1+2+PAYLOAD+1)`.

- `static_assert` membuat **kompilasi gagal** bila tata letak tidak konsisten, jauh lebih murah daripada
  menemukannya saat run.
- Dipakai oleh attacker (pengirim) dan host (penerima): tidak mungkin keduanya berbeda tanpa disengaja.
- `LINK_HAS_PREFIX(s, p)` memanfaatkan `sizeof(p) - 1` pada literal string untuk membandingkan awalan tanpa `strlen`.

---

## 7. Konfigurasi Mbed TLS: `include/mbedtls_k1_config.h`

- Hanya modul minimum: bignum, ecp, ecdsa, asn1, hmac_drbg, md, sha256, kurva P-256 saja. Hasilnya Flash ~47 KB dari 64 KB.
- `MBEDTLS_NO_PLATFORM_ENTROPY`: tidak ada sumber entropi OS di bare-metal; RNG disediakan firmware.
- `MBEDTLS_CHECK_RETURN_WARNING`: hasil fungsi yang diabaikan menjadi peringatan kompilasi. Hardening murah.
- `#if !defined(K1_NO_ASM)` dst: saklar eksperimen pengaruh per opsi tanpa menyentuh baseline.
- Komentar kode mencatat **jebakan build** (perlu `-t clean`) tepat di tempat ia bisa menjebak orang berikutnya.
  Itu praktik dokumentasi yang baik.

---

## 8. Stop mode (sampai 17 Nov): `src/host/final/bersama/stop_window.h`

Tujuan: ganti Sleep (WFI) dengan Stop agar IDLE turun dari 11,6 mA ke ~2 mA.

**Satu alarm RTC, dua tugas.** Alarm RTC dipakai untuk (1) akhir jendela dan (2) selesai menunggu chip (K2S).
`alarm_rearm()` memilih target terdekat, lalu menilai ulang saat alarm menyala:

```cpp
if (!g_win_over && (int32_t)(now - g_win_end_cnt) >= 0) g_win_over = true;
if (g_sign_wait && (int32_t)(now - g_sign_ready) >= 0) g_sign_wait = false;
```

- `(int32_t)(a - b) >= 0` adalah idiom perbandingan waktu **tahan limpahan**: valid selama selisih < 2^31.
  Cari: "wraparound-safe timer comparison".
- Cek ulang `rtc_cnt() - tgt < 0` setelah menulis ALR menangani balapan: target bisa lewat selagi register
  ditulis.
- **Mengapa RTC, bukan DWT.** `CYCCNT` berhenti di Stop. RTC berjalan dari kristal LSE 32,768 kHz yang tetap hidup.
  Hasil uji (17 Nov): selisih lama tidur host vs logger 9-20 us per jendela 28,5 s.
- **Bangun dari Stop = HSI 8 MHz, PLL mati**, sehingga kode wajib menyalakan ulang PLL sebelum melanjutkan.

Bagian keandalan (crash pasca-Stop, NOP sled) adalah temuan 18 Nov, dibahas pada catatan malam ini.
