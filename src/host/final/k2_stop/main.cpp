// ============================================================================
// K2 hardware (DS28E38) + STOP MODE + OVERDRIVE  --  LANGKAH 9 = S4 (integrasi K2, 17 Nov 2026)
// Rencana: docs/referensi/rencana_deep_sleep.md. Menggabungkan:
//   - stop_common.h / stop_window.h / stop_link.h : Stop mode, alarm RTC, penerima frame dengan sinkron bit,
//     mesin jendela, kendali run lewat logger (arsitektur "K2S");
//   - ds28e38_od.h : driver DS28E38 overdrive (PA15) + tunggu sign berbasis polling (hasil eksperimen
//     src/host/dev/eksperimen/ds_overdrive: 115,6 ms per sign vs 192,5 ms standar + tunggu 130 ms).
// Alur satu permintaan (serve_one): pastikan OD -> kirim perintah 0xA5 (3,4 ms, aktif) -> TIDUR STOP ~100 ms
// (DQ ditahan kuat tinggi oleh register GPIO yang bertahan di Stop; alarm RTC membangunkan) -> intip per bit
// sampai chip siap (~7 ms, aktif) -> baca hasil (5,6 ms, aktif). Latensi diukur dengan RTC (DWT berhenti di Stop)
// dan dilaporkan dalam 'siklus' HCLK terukur (STOP_LAT_RTC) supaya run_bench menghitung ms yang benar.
// Build: pio run -e host_k2_stop   Flash: python tools/1_akuisisi/flash_stop.py .pio/build/host_k2_stop/firmware.elf
// Uji:   python tools/1_akuisisi/run_bench.py run A 60 (bila jalur logger->host tersambung) atau env host_k2_stop_auto + tools/dev/tahap_stop/k1s_run.py
// ============================================================================
#include "../bersama/stop_common.h"
#include "../bersama/ds28e38_od.h"

volatile uint32_t g_lse_ok = 0;
static bool g_chip_ok = false;

#ifdef DIAG_SIG                              // B1 (18 Nov): simpan keluaran mentah chip (64 B) 30 tanda tangan pertama + ROM
#define DIAG_N 30
static uint8_t  g_diag_sig[DIAG_N][64];
static uint32_t g_diag_n = 0;
static uint8_t  g_diag_rom[8];
#endif
#ifdef DIAG_LONG                             // 18 Nov: rincian tanda tangan lambat (> 150 ms) untuk diagnosis frame hilang K2S
struct LngRec { uint32_t lat_us, ens_us, send, busy, read, dstop, err, wn, wc[8], wdt[8], wt[8]; };
static uint32_t g_wk_n, g_wk_c[8], g_wk_dt[8], g_wk_f[8];   // bangun dari Stop selama tanda tangan berjalan
static void diag_wake(uint32_t c, uint32_t dt) {
  if (g_wk_n < 8) { g_wk_f[g_wk_n] = rtc_fine(); g_wk_c[g_wk_n] = c; g_wk_dt[g_wk_n] = dt; g_wk_n++; }
}
#define LNG_N 16
static LngRec   g_lng[LNG_N];
static uint32_t g_lng_n = 0;
static uint32_t diag_nstop();                // didefinisikan setelah stop_window.h
#endif
#define HL_ARCH  "K2S"
#define HL_READY (g_chip_ok && g_lse_ok)
#include "../bersama/stop_link.h"
#ifdef DIAG_SIG
static void hl_diag_emit() {
  char hx[129];
  for (int j = 0; j < 8; j++) snprintf(hx + 2 * j, 3, "%02x", g_diag_rom[j]);
  hl_txf(">ROM,%s\n", hx);
  for (uint32_t i = 0; i < g_diag_n; i++) {
    for (int j = 0; j < 64; j++) snprintf(hx + 2 * j, 3, "%02x", g_diag_sig[i][j]);
    hl_txf(">SIG,%lu,%s\n", (unsigned long)i, hx);
  }
#ifdef DIAG_LONG
  for (uint32_t i = 0; i < g_lng_n; i++)
    hl_txf(">LNG,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)i, (unsigned long)g_lng[i].lat_us,
           (unsigned long)g_lng[i].ens_us, (unsigned long)g_lng[i].send, (unsigned long)g_lng[i].busy,
           (unsigned long)g_lng[i].read, (unsigned long)g_lng[i].dstop, (unsigned long)g_lng[i].err);
  for (uint32_t i = 0; i < g_lng_n; i++)
    for (uint32_t k = 0; k < g_lng[i].wn; k++)
      hl_txf(">WK,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)i, (unsigned long)k, (unsigned long)g_lng[i].wc[k],
             (unsigned long)g_lng[i].wdt[k], (unsigned long)g_lng[i].wt[k]);
  g_lng_n = 0;                               // per jendela
#endif
}
#endif

#define STOP_LAT_RTC 1

static void wait_chip_ms(uint32_t ms);          // didefinisikan di stop_window.h
static void stop_wait_hook(uint32_t ms) { wait_chip_ms(ms); }

// Melayani satu permintaan: sign DS28E38 dengan tunggu chip di Stop. lat = tik halus RTC (1/32768 s) seluruh transaksi.
static int serve_one(uint32_t *lat) {
  const uint32_t f0 = rtc_fine();
  uint8_t sig[64];
  Tm tm;
  int e = 1;
#ifdef DIAG_LONG
  const uint32_t ns0 = diag_nstop();
  g_wk_n = 0;
  tm.send_us = tm.busy_us = tm.read_us = 0;
  const bool ens = ds_od_ensure();
  const uint32_t f_ens = rtc_fine();
  if (ens) e = ds_sign(DS_DIGEST, sig, &tm);
#else
  if (ds_od_ensure()) e = ds_sign(DS_DIGEST, sig, &tm);
#endif
  *lat = rtc_fine() - f0;
#ifdef DIAG_LONG
  if (*lat > 4915UL /* 150 ms dalam tik 1/32768 s */ && g_lng_n < LNG_N) {
    LngRec &r = g_lng[g_lng_n++];
    r.lat_us = (uint32_t)((uint64_t)*lat * 1000000ULL / 32768ULL);
    r.ens_us = (uint32_t)((uint64_t)(f_ens - f0) * 1000000ULL / 32768ULL);
    r.send = tm.send_us; r.busy = tm.busy_us; r.read = tm.read_us;
    r.dstop = diag_nstop() - ns0; r.err = (uint32_t)e | (ens ? 0u : 0x100u);
    r.wn = g_wk_n;
    for (uint32_t k = 0; k < g_wk_n; k++) {
      r.wc[k] = g_wk_c[k]; r.wdt[k] = g_wk_dt[k];
      r.wt[k] = (uint32_t)((uint64_t)(g_wk_f[k] - f0) * 1000000ULL / 32768ULL);
    }
  }
#endif
#ifdef DIAG_SIG
  if (e == 0 && g_diag_n < DIAG_N) memcpy(g_diag_sig[g_diag_n++], sig, 64);
#endif
  return e;
}
#include "../bersama/stop_window.h"
#ifdef DIAG_LONG
static uint32_t diag_nstop() { return g_n_stop; }
#endif

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
#ifdef DIAG_SIG
  memcpy(g_diag_rom, rom, 8);
#endif
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
  // Mode OTOMATIS (jalur logger -> host tak tersambung): lihat K1S_AUTO di src/host/final/k1_stop/main.cpp.
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
