# Software-Based ECDSA P-256 terberat (kelompok K1 & K3)

Firmware STM32F103 untuk sisi **software** pada perbandingan daya software vs *hardware offloading* DS28E38
(Bab 3 usulan penelitian). Library: **Mbed TLS 3.6.7**, profil *portable C* dengan semua optimasi opsional dimatikan
(`include/mbedtls_heavy_config.h`). Sumber upstream di `lib/mbedtls/` tidak diubah (hanya 10 berkas yang dibutuhkan).

## Seberapa "berat"?

**Terukur di board (B30, 48 MHz, 2026-11-11)**: heavy = **237,4 M siklus = 4.946,4 ms** (sd 7,8 ms, min/max 4.932,8/4.958,2),
30/30 tanda tangan valid di board dan di PC, stack 856-892 B, heap puncak 1.168 B. Data: `data/k1_sw_mbedtls_heavy_msg_ayah_48mhz.csv`.
Itu **≈ 19,1×** micro-ecc terukur (259,5 ms).

Baris lain di tabel di bawah adalah **estimasi emulator** Cortex-M3: kode ARM asli (flag sama dengan PlatformIO)
dijalankan di emulator, siklus dikalibrasi ke micro-ecc terukur (12,46 M siklus). Pada micro-ecc model mentah selisih 3 %;
pada heavy estimasi 248 M vs terukur 237 M, selisih 4,6 % (campuran instruksi berbeda, banyak pembagian bignum).
Semua tanda tangan emulator diverifikasi di PC (`tools/p256.py`).

| Varian (sign P-256, 48 MHz) | siklus (est.) | ms | × micro-ecc | RAM puncak (heap+stack) |
|---|---|---|---|---|
| micro-ecc, **terukur di board** | 12,46 M | 259,5 | 1,0 | 0 + 804 B |
| mbedTLS **profil default** | 12,98 M | 270 | 1,04 | 860 + 864 B |
| TinyCrypt v0.2.8 | 19,3 M | 402 | 1,55 | 0 + 884 B |
| mbedTLS, hanya `NIST_OPTIM` mati | 75,7 M | 1.578 | 6,1 | 864 + 944 B |
| mbedTLS, tanpa tabel comb G + window 2 | 35,0 M | 729 | 2,8 | 888 + 840 B |
| **mbedTLS heavy (env ini)** | 248 M est. / **237,4 M terukur** | 5.172 est. / **4.946 terukur** | **≈ 19** | **1.168 + 920 B** (board: 1.168 + 892 B) |

Firmware ini: Flash 52,7 KB / 64 KB (80,3 %), RAM statik 6,0 KB / 20 KB. Hanya baris micro-ecc dan heavy yang pernah
diukur di board; baris lain masih estimasi. Reproduksi emulator: `python -I tools/emu_cycles/build.py` lalu `python tools/emu_cycles/run.py` (butuh `pip install unicorn capstone`).

## Pakai

```
pio run -e softwaretes_mbedtls -t upload      # lewat ST-Link, lalu CABUT ST-Link
python monitor.py                              # atau tools/bench_message.py / tools/window_test.py
```
Pembanding profil default Mbed TLS: tambahkan `-DMBEDTLS_PROFILE_DEFAULT` di `build_flags` (Flash 55,0 KB).
Matikan verifikasi di board (hemat Flash dan waktu): `-DON_DEVICE_VERIFY=0`.

| Perintah (115200, newline) | Fungsi |
|---|---|
| `P` | info: clock, saklar library, RAM, kunci publik, uji "DWT tetap jalan saat WFI" |
| `B[n][,gap][\|pesan]` | latensi + RAM n kali (default 30), jeda `gap` ms (tidur WFI). CSV 10 kolom, kompatibel `tools/bench_message.py sw` |
| `M<pesan>` / `S<hex64>` | satu tanda tangan dari pesan / digest; cetak `r\|\|s` untuk verifikasi di PC |
| `N[detik][,periode]` | **Skenario A / K1**: 1 permintaan sah tiap `periode` ms (default 60 s, 10000 ms); CPU `WFI` di antaranya |
| `A[detik]` | **Skenario B / K3, penyerang eksternal (USB)**: tiap baris `S<hex64>` = satu tanda tangan penuh. Ber-ack: firmware mengirim `.` saat mulai memproses, penyerang mengirim 1 baris per `.` (`tools/window_test.py attack`) |
| `F[detik]` | **Skenario B / K3, penyerang internal**: permintaan selalu tersedia tanpa jeda (0 ms), tanpa USB sama sekali (`tools/window_test.py internal`). Dianjurkan untuk pengukuran arus yang bersih |

Selama jendela `N`/`F` firmware tidak mengirim apa pun lewat USB (`A` hanya mengirim ack `.`); hasil dicetak setelah selesai
(`S_cpu`, `T_obs`, jumlah permintaan, latensi tiap tanda tangan).

> **Batasan USB (bug core STM32duino 4.21 di STM32F103).** Bila antrean RX USB (3 paket = 192 B) penuh, core menyiapkan
> endpoint OUT dengan buffer NULL (`USBD_CDC_ClearBuffer`), paket berikutnya ditulis ke alamat 0 oleh `USB_ReadPMA`, dan
> board **HardFault** (CFSR=0x400, HFSR=FORCED, USB mati sampai di-reset). Terjadi pada uji pertama mode `A` dengan flood
> tanpa ack. Akibatnya: jangan mengirim > ~190 byte ke board saat firmware sibuk menandatangani (~5 s), dan mode `A`
> harus lewat `tools/window_test.py` (ber-ack). Pemulihan: flash ulang atau reset.

## Prosedur ukur (Bab 3.5)

1. **Latensi (DWT)**: `python tools/bench_message.py sw COM12 "ayah mau mandi" 30 data/k1_sw_mbedtls.csv`
   (atau `B30` di monitor). Verifikasi tiap sampel dilakukan di board, di luar jendela latensi.
2. **Arus dinamis**: catu daya presisi seri dengan jumper VCC STM32. Hubungkan **PB0** (TRIG, HIGH selama satu
   tanda tangan), **PB1** (SLEEP, HIGH selama WFI), **PB10** (WINDOW, HIGH selama jendela) ke osiloskop/logic analyzer.
   Nyalakan pencatat arus lebih dulu, lalu: `python tools/window_test.py normal COM12 60 10000 data/k1_window.csv`.
3. **%S_cpu**: dicetak firmware (`# Tidur WFI ... S_cpu=`) = waktu di dalam `WFI` / panjang jendela, dihitung dari
   DWT. Pembanding independen: duty PB1 pada logic analyzer.
4. **K3**: `python tools/window_test.py internal COM12 60 data/k3_window.csv` (pengukuran arus) atau
   `... attack COM12 60 ...` (model penyerang eksternal). Interupsi SysTick (1 kHz) ikut membangunkan CPU saat tidur
   (± 0,6 % waktu aktif pada uji skenario A); sama di kedua arsitektur, jadi tidak membiaskan perbandingan.
5. **Power-cycle setelah upload ST-Link.** OpenOCD menyetel `DBGMCU_CR.DBG_SLEEP` (debug tetap hidup saat tidur), yang
   bertahan sampai power-on reset dan membuat `WFI` hampir tidak menghemat arus. Cabut ST-Link dan putus-sambung catu
   daya sebelum mengukur arus. (Pengukuran waktu/%S_cpu tidak terpengaruh.)
6. **Pakai satu biner yang sama untuk semua run K1/K3.** Dua build berbeda (kode bergeser di flash) memberi siklus sign
   berbeda ± 1 % (4.995 vs 4.944 ms pada uji ini), padahal sumber kripto identik.

Fungsi tidur, pin ukur, dan penghitungan %S ada di `include/powertest.h` agar firmware DS28E38 bisa memakai
**kode yang persis sama** (variabel kontrol Bab 3.3.1 c.5).

## Hal yang perlu diputuskan / diketahui

- **Baseline terberat adalah pilihan konfigurasi, bukan perilaku default library.** Dengan Mbed TLS default,
  software ≈ micro-ecc ≈ 0,26–0,27 s, hampir sama dengan DS28E38 terukur (224 ms total). Angka ≈ 5 s di sini muncul karena
  empat optimasi resmi dimatikan. Sebut ini sebagai *batas atas (worst case)* dan laporkan micro-ecc / mbedTLS default
  sebagai *batas bawah*, supaya klaim persentase reduksi tidak dibantah penguji.
- **Clock**: build USB bawaan = **HSI/2 × 12 = 48 MHz** (`generic_clock.c`), bukan 72 MHz seperti Bab 3.3.1 c.2 dan
  3.4.1 a. Pada 72 MHz sign heavy ≈ 3,4 s. Pilih satu dan samakan di K1–K4.
- **Sudah dijalankan di board (2026-11-11)**: `P` (clock 48 MHz, "DWT di WFI: ya"), `B3`, **B30**, `N30,10000`, `F30`, `A30`.
  - Skenario A (30 s, periode 10 s): 3 permintaan, **S_cpu = 49,386 %**, T_obs DWT 29.999,997 ms vs millis 30.000 ms.
  - Skenario B (`F30` dan `A30`): 7 tanda tangan, **S_cpu = 0,000 %**, 99,99 % waktu di dalam sign, 0 kali WFI
    (sleep deprivation seperti yang dimodelkan penelitian). Jendela berakhir setelah sign yang sedang berjalan selesai
    (T_obs ≈ 34,6 s untuk T_obs diset 30 s); %S_cpu memakai T_obs aktual.
  - **Belum**: pengukuran arus (butuh instrumen dan wiring PB0/PB1/PB10), dan verifikasi pin SLEEP/TRIG di logic analyzer.
- Satu tanda tangan ≈ 5 s, jadi `periode` skenario A harus > T_sign (kolom `overrun` > 0 bila tidak). B30 dengan
  verifikasi di board memakan ± 6 menit; `-DON_DEVICE_VERIFY=0` memangkasnya jadi ± 2,5 menit.
- RNG nonce = xorshift dari derau ADC: **tidak aman**, hanya untuk benchmark (biayanya diabaikan terhadap sign).
  Kunci privat uji statis di Flash, sesuai model ancaman software-based.
- Kunci publik konstan dihitung offline (`tools/p256.py`); jika `PRIVATE_KEY` diganti, hitung ulang `PUBLIC_KEY`
  (salah nilai terlihat sebagai `verify_ok = 0`).
