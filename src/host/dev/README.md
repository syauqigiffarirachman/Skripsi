# src/host/dev — kode PENGEMBANGAN host (tidak ada di data akhir)

| Folder | Isi | Env |
|---|---|---|
| `tahap_stop/` | tangga menuju Stop mode: S0a (LSE/RTC), S0 (lantai arus Stop), S1 (alarm RTC), S2 (bangun oleh frame) | `host_s0a_lse*`, `host_s0_stop`, `host_s1_rtc`, `host_s2_rx`, `host_s2_trace` |
| `tahap_d/` | firmware tahap D tanpa Stop (tidur WFI): K1 software, K2 hardware + `bersama/` (host_link, host_ram, host_rx, host_time) | `k1_sw`, `k1_sw_auto`, `k2_hw` |
| `eksperimen/` | percobaan awal: overdrive DS28E38, penerima frame, monolit, sederhana, softwaretes (micro-ecc dan Mbed TLS heavy) | `host_ds_od*`, `k1_rx`, `sederhana_stlink`, `genericSTM32F103C8`, `softwaretes*` |

`tahap_stop/s2_rx` memakai `../../../final/bersama/stop_common.h` (alat Stop yang kemudian menjadi dasar firmware final).
Firmware final tidak pernah meng-`#include` apa pun dari folder ini.
