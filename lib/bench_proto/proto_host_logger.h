// ============================================================================
// Protokol HOST <-> LOGGER: jalur kendali run (pustaka bersama bench_proto, header-only).
//
// SATU-SATUNYA definisi perintah, awalan balasan, dan batas jendela. Dipakai host
// (src/host_common/host_link.h dan CLI USB di src/host/k1_software, src/host/k2_hardware) dan logger
// (src/logger/f103/main.cpp). PC (tools/1_akuisisi/run_bench.py) mengurai baris yang sama: sesuaikan juga bila
// format diubah.
//
// Jalur fisik: host USART1 PA9 (TX) -> logger PA10 (RX), logger PA9 (TX) -> host PA10 (RX),
// 115200 8N1, ground bersama. Baris ASCII diakhiri '\n'.
//   logger -> host   ?              minta identitas
//                    S<detik>       jendela dengan permintaan NYATA attacker (1..LINK_S_SECS_MAX)
//                    W<detik>,<p>   jendela dengan permintaan sintetis TIM4 tiap p ms
//   host -> logger   >ID,<arsitektur>,<tahap>,<SystemCoreClock>,<siap 0/1>
//                    >START,<perintah>   dikirim tuntas sebelum t0 jendela
//                    >END,<23 kolom>     hasil jendela (urutan kolom: hl_send_result di host_link.h)
//                    >L,<siklus>,...     latensi sign per permintaan (siklus CYCCNT), 10 per baris
//                    >DONE               akhir kiriman
//                    >ERR,<alasan>
// ============================================================================
#pragma once
#include <string.h>

#define LINK_BAUD       115200UL

// Perintah logger -> host: string satu huruf (pengirim menyambung literal, penerima memakai [0]).
#define LINK_CMD_ID     "?"
#define LINK_CMD_UART   "S"            // S<detik>
#define LINK_CMD_SYN    "W"            // W<detik>,<p_ms>

// Awalan balasan host -> logger.
#define LINK_R_ID       ">ID"
#define LINK_R_START    ">START"
#define LINK_R_END      ">END"
#define LINK_R_LAT      ">L"
#define LINK_R_DONE     ">DONE"
#define LINK_R_ERR      ">ERR"

// Batas jendela host. S: TIM4 16-bit + prescaler (maks 89 s pada 48 MHz). Host menolak perintah di
// luar batas; logger memakai batas yang sama supaya tidak mengirim perintah yang pasti ditolak.
#define LINK_S_SECS_MAX 80
#define LINK_W_SECS_MAX 120
#define LINK_W_P_MIN_MS 100
#define LINK_W_P_MAX_MS 6000

// true bila baris s diawali awalan p (p harus literal string, mis. LINK_R_START).
#define LINK_HAS_PREFIX(s, p) (strncmp((s), (p), sizeof(p) - 1) == 0)
