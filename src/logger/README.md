# src/logger — firmware logger (FINAL, satu folder)

Env: `logger` (`logger_selftest` = flag uji PWM di PA6, bukan data).

| Berkas | Peran |
|---|---|
| `f103/main.cpp` | **ALUR UTAMA**: jam 72 MHz (HSE), mesin status run, perintah lewat nilai baud, jalur USART1 ke host, CSV via USB CDC, input-capture pin status (TIM2 PA0), peta pin |
| `modul/ina226.*` | driver INA226 (I2C), tanpa ketergantungan jenis STM32 |
| `modul/l3_capture.*` | cuplik arus kontinu + stempel waktu |
| `modul/edge_stats.*` | statistik tepi pin status (HIGH, LOW, periode, kesalahan urutan) |
| `modul/ina_report.*` | laporan teks INA226 dan pindai I2C |

Protokol baris ke host: `lib/bench_proto/proto_host_logger.h`.

Mengapa `f103/` lalu `modul/`: objek di `src/` ditautkan menurut urutan path; nama ini ('f' < 'm') mempertahankan urutan
lama sehingga `firmware.bin` tetap identik dengan logger yang mengambil data. Kode bersama yang punya `.cpp` TIDAK boleh
di `lib/` (tata letak Flash bergeser).
