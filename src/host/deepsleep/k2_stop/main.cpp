// ============================================================================
// K2 hardware (DS28E38) + STOP MODE + OVERDRIVE  --  LANGKAH 9 = S4 (integrasi K2, 17 Nov 2026)
// Rencana: docs/rencana_deep_sleep.md. Menggabungkan:
//   - stop_common.h / stop_window.h / stop_link.h : Stop mode, alarm RTC, penerima frame dengan sinkron bit,
//     mesin jendela, kendali run lewat logger (arsitektur "K2S");
//   - ds28e38_od.h : driver DS28E38 overdrive (PA15) + tunggu sign berbasis polling (hasil eksperimen
//     src/host/eksperimen/ds_overdrive: 115,6 ms per sign vs 192,5 ms standar + tunggu 130 ms).
// Alur satu permintaan (serve_one): pastikan OD -> kirim perintah 0xA5 (3,4 ms, aktif) -> TIDUR STOP ~100 ms
// (DQ ditahan kuat tinggi oleh register GPIO yang bertahan di Stop; alarm RTC membangunkan) -> intip per bit
// sampai chip siap (~7 ms, aktif) -> baca hasil (5,6 ms, aktif). Latensi diukur dengan RTC (DWT berhenti di Stop)
// dan dilaporkan dalam 'siklus' HCLK terukur (STOP_LAT_RTC) supaya run_bench menghitung ms yang benar.
// Build: pio run -e host_k2_stop   Flash: python tools/flash_stop.py .pio/build/host_k2_stop/firmware.elf
// Uji:   python tools/run_bench.py run A 60 (bila jalur logger->host tersambung) atau env host_k2_stop_auto + tools/k1s_run.py
// ============================================================================
#include "../stop_common.h"
#include "../ds28e38_od.h"

volatile uint32_t g_lse_ok = 0;
static bool g_chip_ok = false;

#define HL_ARCH  "K2S"
#define HL_READY (g_chip_ok && g_lse_ok)
#include "../stop_link.h"

#define STOP_LAT_RTC 1

static void wait_chip_ms(uint32_t ms);          // didefinisikan di stop_window.h
static void stop_wait_hook(uint32_t ms) { wait_chip_ms(ms); }

// Melayani satu permintaan: sign DS28E38 dengan tunggu chip di Stop. lat = tik halus RTC (1/32768 s) seluruh transaksi.
static int serve_one(uint32_t *lat) {
  const uint32_t f0 = rtc_fine();
  uint8_t sig[64];
  Tm tm;
  int e = 1;
  if (ds_od_ensure()) e = ds_sign(DS_DIGEST, sig, &tm);
  *lat = rtc_fine() - f0;
  return e;
}
#include "../stop_window.h"

#define SAFE_MS 5000UL

void setup() {
  usb_off();
  cyccnt_start();
  pins_init();
  ds_init();                                      // PA15: setelah pins_init (yang menganalogkan pin tak terpakai)
  hl_init();
  PB1_HI();
  const uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < SAFE_MS) {}  // jendela aman SWD (Stop memutus SWD)
  DBGMCU->CR = 0;
  uint8_t rom[8];
  g_chip_ok = ds_read_rom(rom);
  if (g_chip_ok) g_chip_ok = ds_od_enter();
  g_ds_wait = stop_wait_hook;
  g_lse_ok = lse_rtc_start() ? 1 : 0;
  rx_hw_init();
  PB1_LO();
  if (g_lse_ok) hl_clk_hz = measure_hclk();
}

#ifdef K2S_AUTO
#ifndef K2S_AUTO_S
#define K2S_AUTO_S      60
#endif
#ifndef K2S_AUTO_GAP_MS
#define K2S_AUTO_GAP_MS 8000
#endif
#endif

void loop() {
#ifdef K2S_AUTO
  // Mode OTOMATIS (jalur logger -> host tak tersambung): lihat K1S_AUTO di src/host/deepsleep/k1_stop/main.cpp.
  if (g_chip_ok && g_lse_ok) {
    const uint32_t t = millis();
    while ((uint32_t)(millis() - t) < K2S_AUTO_GAP_MS) {}
    char cmd[12];
    snprintf(cmd, sizeof cmd, "S%lu", (unsigned long)K2S_AUTO_S);
    hl_txf(LINK_R_START ",%s\n", cmd);
    hl_tx_flush();
    cmd_window(K2S_AUTO_S, 1000, true);
    g_hl_runs++;
    hl_send_result();
    return;
  }
#endif
  hl_service();
}
