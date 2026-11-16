# Pinout Arsitektur Pengujian (bangku uji Tahap D)

Disusun 16 Nov 2026 dari kode firmware yang dipakai pada 120 run final (commit `448befe`), `platformio.ini`, dan logbook 12–15 Nov.
Tujuannya jadi acuan saat merancang PCB/board yang lebih rapi. Butir bertanda **[cek]** belum tercatat di kode/logbook dan perlu diukur langsung di bangku.

---

## 1. Ringkasan board

| Peran | Board / MCU | Jam | Firmware (env) | Kode | Daya saat run | Pemrograman |
|---|---|---|---|---|---|---|
| **Host K1** (software ECDSA, Mbed TLS) | Blue Pill STM32F103C8T6 (Cortex-M3) | HSI → 48 MHz (tanpa kristal) | `k1_sw` | `src/host/k1_software/` + `src/host_common/` | USB-C **hanya daya** (VBUS, GND, CC) lewat shunt INA226 | ST-Link serial `$` |
| **Host K2** (hardware DS28E38) | Board yang SAMA dengan K1, diflash ulang | HSI → 48 MHz | `k2_hw` | `src/host/k2_hardware/` + `src/host_common/` | idem | ST-Link serial `$` |
| **Logger** | Blue Pill STM32F103C8T6 klon (UID nol) | HSE 8 MHz ×9 = 72 MHz (kristal) | `logger` (build L6B1) | `src/logger/f103/` + `src/logger_common/` | micro-USB sendiri (juga jalur CSV ke PC) | ST-Link serial `X` |
| **Attacker** (pembangkit permintaan) | STM32F030C8T6 (Cortex-M0) | HSI 48 MHz | `attacker` | `src/attacker/` | USB bawaan board / CH340 | ST-Link (13 Nov), bootloader CH340 dinonaktifkan |
| **Sensor arus** | Modul INA226, shunt berkode R100 | – | – | `src/logger_common/ina226.*` | VS dari 3V3 logger | – |
| **Secure element** | DS28E38 (K2 saja) | – | – | – | dari host (1-Wire) | – |


---

## 2. Host STM32F103C8T6 (K1 dan K2)

Satu board fisik, firmware diganti per blok arsitektur. Peta pin identik di kedua firmware kecuali 1-Wire (K2) dan ADC entropi (K1).

| Pin | Fungsi | Periferal / mode | Arah | Terhubung ke | Dipakai | Catatan |
|---|---|---|---|---|---|---|
| PA0 | Sumber entropi (derau ADC) | ADC1, analog | in | **tidak tersambung** (mengambang) | K1 | Dicampur CYCCNT + UID + micros, di-SHA-256 (H2). Jangan disambung apa pun di board baru tanpa mengganti sumber entropi. |
| PA2 | USART2 TX | AF | out | – (tak dipakai) | K1, K2 | Bebas. |
| PA3 | USART2 RX: frame permintaan attacker | USART2 + DMA1 Ch6, input floating, 115200 8N1 | in | Attacker PA9 via R seri ~1 kΩ | K1, K2 | Frame 36 B, satu interupsi TC per frame. |
| PA9 | USART1 TX: jalur kendali run | AF push-pull 2 MHz, register langsung, tanpa IRQ/DMA | out | Logger PA10 | K1, K2 | Protokol `>ID/>START/>END/>L/>DONE` (`host_link.h`). |
| PA10 | USART1 RX: jalur kendali run | input pull-up, polling di luar jendela | in | Logger PA9 | K1, K2 | |
| PA11 / PA12 | USB D− / D+ (CDC) | USB FS | – | **tidak tersambung saat run** (USB-C hanya daya) | K1, K2 (debug) | Saat debug lewat USB, SOF di-mask selama jendela. |
| PA13 / PA14 | SWDIO / SWCLK | SWD | – | ST-Link `$` | flash | ST-Link **dicabut** saat run final; pin 3V3 ST-Link wajib ke 3V3 target (klon). |
| PA15 | 1-Wire data DS28E38 | GPIO open-drain bit-bang (lib OneWire) | i/o | DS28E38 IO + pull-up ke 3V3 | K2 | Nilai pull-up **[cek]**. PA15 = JTDI: JTAG harus dilepas (core sudah memakai SWD saja). |
| PB1 | Pin status CPU (1 = aktif, 0 = akan WFI) | GPIO push-pull, BSRR | out | Logger PA0 | K1, K2 | Pembanding independen %S_cpu dan latensi. |
| PC13 | LED onboard | – | – | – | tidak dipakai | Dibiarkan mati di semua run (variabel kontrol). |
| 5V / VBUS | Catu | – | in | USB-C → shunt INA226 (IN+ → IN−) → host | – | Titik ukur energi = sisi beban (IN−). |
| 3V3 | Rail logika | regulator onboard | – | DS28E38 VCC (K2), ST-Link 3V3 (saat flash) | – | |
| GND | Ground bersama | – | – | Logger, attacker, INA226, DS28E38 | – | Wajib satu titik ground. |

Periferal internal tanpa pin (penting bila memindah ke MCU lain): TIM2→TIM3 berantai (waktu 32-bit 1 µs, tetap jalan saat WFI), TIM4 (permintaan sintetis/akhir jendela), TIM1 (bangun K2 saat menunggu DS28E38), DMA1 Ch6 (USART2 RX), DWT CYCCNT (latensi).

Cadangan di `include/powertest.h` (tidak dipakai di Tahap D): PB0 = TRIG per sign, PB1 = SLEEP, PB10 = WINDOW.

---

## 3. DS28E38 (K2)

| Pin DS28E38 | Ke | Catatan |
|---|---|---|
| IO | Host PA15 + resistor pull-up ke 3V3 | 1-Wire kecepatan standar, Skip ROM. Pull-up tetap di semua run. Nilai **[cek]**. |
| VCC (bila varian 3 pin) | Host 3V3 | **[cek]** kemasan/modul yang dipakai (parasite vs catu langsung). |
| GND | GND bersama | |

Pada K1 modul DS28E38 **[cek]** tetap terpasang atau dicabut; untuk board baru sebaiknya ada jumper/saklar supaya beban statis identik di kedua arsitektur.

---

## 4. Logger STM32F103C8T6 (Blue Pill klon)

| Pin | Fungsi | Periferal / mode | Arah | Terhubung ke | Catatan |
|---|---|---|---|---|---|
| PA0 | Tangkap tepi pin status host | TIM2_CH1 (naik) + CH2→TI1 (turun), input pull-down | in | Host PB1 | TIM2 16-bit + penghitung overflow → tik 72 MHz 64-bit. |
| PA6 | Generator uji PWM 100 Hz / 30 % | TIM3_CH1 | out | jumper ke PA0 (uji mandiri saja) | Hanya env `logger_selftest`; env `logger` membiarkan PA6 input. **Jangan** sambung PB1 host dan jumper PA6 bersamaan. |
| PA9 | USART1 TX | Serial1, 115200 8N1 | out | Host PA10 | |
| PA10 | USART1 RX | Serial1, buffer RX 512 B | in | Host PA9 | |
| PA11 / PA12 | USB CDC ke PC (CSV) | USB FS | – | PC (micro-USB) | Klon: patch nomor seri wajib; jalur OUT PC→logger rusak, perintah dikirim lewat nilai baud (SET_LINE_CODING). |
| PA13 / PA14 | SWDIO / SWCLK | SWD | – | ST-Link `X` | |
| PB6 | I2C1 SCL | 400 kHz | out | INA226 SCL | Pemulihan bus (`INA226::recover`). |
| PB7 | I2C1 SDA | 400 kHz | i/o | INA226 SDA | |
| PC13 | LED detak 1 Hz | GPIO, aktif rendah | out | onboard | |
| OSC_IN/OUT (PD0/PD1) | Kristal 8 MHz | HSE | – | onboard | Acuan waktu independen (host memakai HSI, galat ~−0,5 %). |
| 3V3 | Catu INA226 VS | – | out | INA226 VS | |
| GND | Ground bersama | – | – | Host, INA226 | |

---

## 5. INA226 (modul)

| Pin modul | Ke | Catatan |
|---|---|---|
| VS | Logger 3V3 | |
| GND | GND bersama | |
| SCL / SDA | Logger PB6 / PB7 | Pull-up I2C dari modul **[cek]** nilai. |
| A0 | GND | Alamat terukur **0x44** (A1 = VS, A0 = GND). Paper v3 menulis 0x40. |
| A1 | VS | |
| IN+ | VBUS dari USB-C (sisi sumber, port PC) | Shunt di sisi positif (high-side). |
| IN− | VBUS ke host (sisi beban) | |
| VBS (bus) | IN− | Disambung 15 Nov (A2a); Vbus terukur ~5,02–5,06 V. |
| ALERT | tidak tersambung | Opsi perbaikan cap waktu (mode conversion-ready + interupsi). |

Konfigurasi: cfg 0x4127 (AVG 1, 1,1 ms shunt + bus, kontinu), CAL 0x0800; laju cuplik nyata ~495,7/s. Shunt efektif terukur ~0,080–0,083 Ω walau berkode R100 → kalibrasi di PC (`--rshunt`), offset −2 LSB (−50 µA).

---

## 6. Attacker STM32F030C8T6

| Pin | Fungsi | Periferal / mode | Arah | Terhubung ke | Catatan |
|---|---|---|---|---|---|
| PA9 | USART1 TX: frame permintaan | Serial1, 115200 8N1 | out | R seri ~1 kΩ → Host PA3 | Frame 36 B: `A5`, seq u16 LE, 32 B isi, XOR. Laju TIM3: A = 1/s, B = 10/s. |
| PA10 | USART1 RX | Serial1 | in | – (tak dipakai) | |
| PA2 | USART2 TX: konsol | Serial2 | out | CH340 RX | Perintah `a/b/x/s/f`. Auto-start 1 Hz (`ATTACKER_AUTOSTART_HZ`). |
| PA3 | USART2 RX: konsol | Serial2 | in | CH340 TX | |
| PA13 / PA14 | SWD | – | – | ST-Link (hanya saat flash) | |
| BOOT0 / NRST | Auto-reset bootloader | DTR/RTS CH340 | – | onboard | Jalur lama (`extra_script_attacker.py`), tidak dipakai lagi. |
| GND | Ground bersama | – | – | Host | |

---

## 7. Daftar sambungan antar-board (netlist)

| # | Dari | Ke | Sinyal | Komponen seri | Catatan |
|---|---|---|---|---|---|
| 1 | Attacker PA9 | Host PA3 | Frame permintaan UART 115200 | R ~1 kΩ | Satu arah. |
| 2 | Host PA9 | Logger PA10 | Kendali run host→logger | – | |
| 3 | Logger PA9 | Host PA10 | Kendali run logger→host | – | |
| 4 | Host PB1 | Logger PA0 | Pin status CPU | – | Lepas jumper PA6–PA0 saat run. |
| 5 | Logger PB6 / PB7 | INA226 SCL / SDA | I2C 400 kHz | – | |
| 6 | Logger 3V3 | INA226 VS | Catu sensor | – | |
| 7 | USB-C port PC (VBUS) | INA226 IN+ | Catu host | shunt R100 | |
| 8 | INA226 IN− (+VBS) | Host 5V | Catu host terukur | – | |
| 9 | Host PA15 | DS28E38 IO | 1-Wire | pull-up ke 3V3 **[cek]** | K2. |
| 10 | Logger PA11/PA12 | PC | CSV (USB CDC) | – | |
| 11 | Attacker PA2/PA3 | CH340 → PC | Konsol | – | |
| 12 | Semua GND | – | Ground bersama | – | |

Jalur PC: logger USB CDC (COM18 terakhir), attacker CH340 (COM16/17), host USB hanya daya. Nomor COM berubah tiap cabut-pasang.

---

## 8. Diagram ringkas

```
             PC (run_bench.py)
     USB CDC │                │ CH340
             │                │
   ┌─────────┴──────┐   ┌─────┴──────────┐
   │ LOGGER F103    │   │ ATTACKER F030  │
   │ 72 MHz HSE     │   │                │
   │ PA0  ◄─────────┼─┐ │ PA9 ─[1k]──┐   │
   │ PA9/PA10 ◄──►──┼┐│ └────────────┼───┘
   │ PB6/PB7 ─┐     │││              │
   └──────────┼─────┘││              │
         I2C  │      ││ USART1       │ USART2
   ┌──────────┴──┐   ││              │
   │ INA226 0x44 │   ││  ┌───────────┴──────────┐
   │ IN+ ◄ USB-C │   │└─►│ PA10/PA9   HOST F103 │
   │ IN− ────────┼───┼──►│ 5V         48 MHz HSI│
   └─────────────┘   └───┤ PB1                  │
                         │ PA3  ◄───────────────┘
                         │ PA15 ── DS28E38 (K2)  │
                         └───────────────────────┘
               GND bersama ke semua node
```

---

## 9. Catatan untuk board berikutnya

1. **Kristal di host.** Host memakai HSI (galat ~−0,5 %, berubah antar run); sekarang dikoreksi dengan waktu logger. Board baru: pasang kristal 8 MHz dan konfigurasi HSE.
2. **Pull-up 1-Wire dan beban DS28E38** dibuat terdokumentasi (nilai tetap, footprint jelas) dan ada jumper supaya kondisi K1/K2 setara.
3. **Shunt presisi.** Ganti shunt modul (efektif ~0,08 Ω) dengan resistor shunt 1 % yang diketahui; pertimbangkan pin ALERT INA226 ke input-capture logger untuk cap waktu cuplikan.
4. **SWD terpisah dari catu.** ST-Link terpasang mengubah arus terukur (0,03–0,11 mA) dan OpenOCD menyalakan DBGMCU_CR. Sediakan header SWD yang mudah dicabut atau isolasi.
5. **USB host hanya daya** di bangku ukur. Bila board baru punya USB data, beri jumper untuk memutus D+/D− saat ukur.
6. **Logger MCU asli** (bukan klon UID nol) agar USB OUT bekerja; atau beri UART-USB terpisah untuk perintah.
7. **Pin uji PA6→PA0** dijadikan jumper berlabel, jangan tersambung bersamaan dengan pin status host.
8. Pertahankan R seri ~1 kΩ di jalur attacker→host (proteksi bila beda catu/ground saat bongkar-pasang).
9. Sinyal opsional yang sudah disiapkan di kode: PB0 TRIG, PB10 WINDOW (`include/powertest.h`) dapat diberi test point.
