// ============================================================================
// TAHAP 5 (host): jalur kendali run lewat USART1 ke logger - DIPAKAI BERSAMA K1 dan K2
// (di-include oleh src/host/k1_software/main.cpp dan src/host/k2_hardware/main.cpp; blok ini IDENTIK
// di keduanya karena satu berkas).
//
// Alasan: di bangku ukur USB-C host hanya daya (tanpa D+/D-), jadi jendela pengamatan tidak bisa
// dipicu lewat USB. Sesuai paper v3 (Tabel 3.3): host PA9 (TX) -> logger PA10 (RX), host PA10 (RX)
// <- logger PA9 (TX), 115200 8N1, GND bersama.
//
// Rancangan:
//   - Register USART1 langsung, TANPA interupsi dan TANPA DMA: tidak ada sumber bangun baru bagi
//     WFI di dalam jendela. Penerimaan di-polling dari loop() (hanya di LUAR jendela); pengiriman
//     blocking. Selama jendela host tidak mengirim dan tidak membaca apa pun di jalur ini.
//   - Protokol baris ASCII (diakhiri '\n'); definisi resmi di lib/bench_proto/proto_host_logger.h:
//       logger -> host   ?            minta identitas
//                        S<detik>     RUN_START: jendela dengan permintaan NYATA attacker (1..80 s)
//                        W<detik>,<p> RUN_START: jendela dengan permintaan sintetis TIM4 tiap p ms
//       host -> logger   >ID,<arsitektur>,<tahap>,<SystemCoreClock>,<siap 0/1>
//                        >START,<perintah>        dikirim TUNTAS (TC) sebelum t0 jendela
//                        >END,<kolom...>          RUN_END + hasil jendela (lihat hl_send_result)
//                        >L,<siklus>,...          latensi sign per permintaan (siklus CYCCNT), 10 per baris
//                        >DONE                    akhir kiriman
//                        >ERR,<alasan>
//   - Latensi per permintaan disimpan di RAM selama jendela (paper: hasil per permintaan disimpan
//     lalu dikirim SETELAH run), maks HL_LAT_MAX; kelebihannya hanya dihitung (lat_drop).
//
// Pemakaian di main.cpp: definisikan HL_ARCH ("K1"/"K2") dan HL_READY (ekspresi bool) sebelum
// include; panggil hl_init() di setup(), hl_service() di loop(); isi hl_res di akhir cmd_window();
// panggil hl_lat_reset() di awal jendela dan hl_lat_add(siklus) tiap sign berhasil.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <stdarg.h>
#include "../../../bersama/host_util.h"
#include <proto_host_logger.h>   // perintah/balasan/batas (lib/bench_proto, satu definisi dengan logger)

#define HL_LINE_MAX 24
#define HL_LAT_MAX  400                          // K2 barrage 60 s: ~298 sign; K1: <= 92
#define HL_STAGE    5

struct HlResult {
  uint32_t secs, p_ms, uart;
  uint32_t t_obs, t_sleep, act_us;
  int32_t  resid;
  uint32_t arrived, served, gagal, wfi, spurious, pin_salah;
  uint32_t rx_ok, rx_bad, rx_lost;
};
static HlResult hl_res;

static uint32_t hl_lat[HL_LAT_MAX];
static uint32_t hl_lat_n = 0, hl_lat_drop = 0;

static inline void hl_lat_reset() { hl_lat_n = 0; hl_lat_drop = 0; }
static inline void hl_lat_add(uint32_t cyc) {
  if (hl_lat_n < HL_LAT_MAX) hl_lat[hl_lat_n++] = cyc; else hl_lat_drop++;
}

// Dibaca lewat SWD untuk verifikasi tanpa logger.
volatile uint32_t g_hl_rx_lines = 0, g_hl_runs = 0, g_hl_err = 0, g_hl_ore = 0;

static void hl_init() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  // PA9 = TX: AF push-pull 2 MHz (CNF=10, MODE=10 -> 0xA); PA10 = RX: input pull-up (0x8, ODR=1)
  // supaya jalur diam (tinggi) walau logger belum tersambung.
  GPIOA->CRH = (GPIOA->CRH & ~((0xFUL << 4) | (0xFUL << 8))) | (0xAUL << 4) | (0x8UL << 8);
  GPIOA->BSRR = GPIO_BSRR_BS10;
  USART1->CR1 = 0; USART1->CR2 = 0; USART1->CR3 = 0;
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + LINK_BAUD / 2) / LINK_BAUD;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;      // tanpa RXNEIE: polling saja
}

static void hl_tx(const char *s) {
  while (*s) {
    while (!(USART1->SR & USART_SR_TXE)) {}
    USART1->DR = (uint8_t)*s++;
  }
}

// Tunggu byte terakhir benar-benar keluar dari pin (dipakai sebelum t0 jendela).
static void hl_tx_flush() { while (!(USART1->SR & USART_SR_TC)) {} }

static void hl_txf(const char *fmt, ...) {
  char b[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof b, fmt, ap);
  va_end(ap);
  hl_tx(b);
}

static void hl_rx_flush() {
  (void)USART1->SR; (void)USART1->DR;
  (void)USART1->SR; (void)USART1->DR;
}

// Non-blocking: kumpulkan byte; true bila satu baris utuh sudah ada di out (tanpa '\n').
static bool hl_poll_line(char *out) {
  static char line[HL_LINE_MAX];
  static uint8_t n = 0;
  const uint32_t sr = USART1->SR;
  if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {         // galat: buang baris yang sedang dirakit
    (void)USART1->DR;
    if (sr & USART_SR_ORE) g_hl_ore++;
    n = 0;
    return false;
  }
  if (!(sr & USART_SR_RXNE)) return false;
  const char c = (char)USART1->DR;
  if (c == '\r') return false;
  if (c != '\n') { if (n < HL_LINE_MAX - 1) line[n++] = c; else n = 0; return false; }
  line[n] = '\0';
  memcpy(out, line, (size_t)n + 1);
  n = 0;
  return out[0] != '\0';
}

static void hl_send_result() {
  uint32_t mn = 0xFFFFFFFFUL, mx = 0;
  uint64_t sum = 0;
  for (uint32_t i = 0; i < hl_lat_n; i++) {
    const uint32_t v = hl_lat[i];
    sum += v;
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }
  if (hl_lat_n == 0) mn = 0;
  // Kolom: arsitektur, detik, p_ms, uart, T_obs(us), T_sleep(us), aktif_CYCCNT(us), sisa(us), datang,
  // dilayani, gagal_sign, WFI, bangun_lain, pin_salah, frame_ok, frame_bad, frame_hilang, n_lat,
  // lat_min, lat_rata2, lat_maks (siklus), lat_tak_tersimpan, SystemCoreClock nominal (Hz)
  hl_txf(LINK_R_END "," HL_ARCH ",%lu,%lu,%lu,%lu,%lu,%lu,%ld,",
         (unsigned long)hl_res.secs, (unsigned long)hl_res.p_ms, (unsigned long)hl_res.uart,
         (unsigned long)hl_res.t_obs, (unsigned long)hl_res.t_sleep, (unsigned long)hl_res.act_us,
         (long)hl_res.resid);
  hl_txf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,",
         (unsigned long)hl_res.arrived, (unsigned long)hl_res.served, (unsigned long)hl_res.gagal,
         (unsigned long)hl_res.wfi, (unsigned long)hl_res.spurious, (unsigned long)hl_res.pin_salah,
         (unsigned long)hl_res.rx_ok, (unsigned long)hl_res.rx_bad, (unsigned long)hl_res.rx_lost);
  hl_txf("%lu,%lu,%lu,%lu,%lu,%lu\n",
         (unsigned long)hl_lat_n, (unsigned long)mn,
         (unsigned long)(hl_lat_n ? sum / hl_lat_n : 0), (unsigned long)mx,
         (unsigned long)hl_lat_drop, (unsigned long)SystemCoreClock);
  for (uint32_t i = 0; i < hl_lat_n; i++) {
    if (i % 10 == 0) hl_tx(LINK_R_LAT);
    hl_txf(",%lu", (unsigned long)hl_lat[i]);
    if (i % 10 == 9 || i + 1 == hl_lat_n) hl_tx("\n");
  }
  hl_tx(LINK_R_DONE "\n");
}

static void cmd_window(uint32_t secs, uint32_t p_ms, bool uart);   // didefinisikan di main.cpp

// Panggil dari loop(). Hanya berjalan di luar jendela.
static void hl_service() {
  char line[HL_LINE_MAX];
  if (!hl_poll_line(line)) return;
  g_hl_rx_lines++;
  if (line[0] == LINK_CMD_ID[0]) {
    hl_txf(LINK_R_ID "," HL_ARCH ",%d,%lu,%d\n", HL_STAGE, (unsigned long)SystemCoreClock, (HL_READY) ? 1 : 0);
    return;
  }
  uint32_t secs = 0, p = 1000;
  bool uart = false;
  if (line[0] == LINK_CMD_UART[0]) {
    secs = parse_u32(line + 1, 0);
    uart = true;
    if (secs < 1 || secs > LINK_S_SECS_MAX) secs = 0;       // batas TIM4 16-bit + prescaler (89 s pada 48 MHz)
  } else if (line[0] == LINK_CMD_SYN[0]) {
    secs = parse_u32(line + 1, 0);
    const char *comma = strchr(line, ',');
    if (comma) p = parse_u32(comma + 1, 0);
    if (secs < 1 || secs > LINK_W_SECS_MAX || p < LINK_W_P_MIN_MS || p > LINK_W_P_MAX_MS) secs = 0;
  } else {
    g_hl_err++;
    hl_txf(LINK_R_ERR ",perintah,%s\n", line);
    return;
  }
  if (secs == 0)   { g_hl_err++; hl_txf(LINK_R_ERR ",parameter,%s\n", line); return; }
  if (!(HL_READY)) { g_hl_err++; hl_tx(LINK_R_ERR ",belum_siap\n"); return; }
  hl_txf(LINK_R_START ",%s\n", line);
  hl_tx_flush();                                // t0 jendela baru dimulai setelah baris ini tuntas
  cmd_window(secs, p, uart);
  hl_rx_flush();                                // buang derau yang mungkin masuk selama jendela
  g_hl_runs++;
  hl_send_result();
}
