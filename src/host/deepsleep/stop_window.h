// ============================================================================
// Mesin JENDELA PENGAMATAN varian Stop mode (17 Nov 2026), dipakai bersama K1S dan K2S.
// Dasar: stop_common.h (RTC, Stop, penerima frame) dan stop_link.h (kendali run lewat logger).
//
// Satu alarm RTC dimultipleks untuk dua kejadian: AKHIR JENDELA dan (K2S) SELESAI TUNGGU CHIP (sign DS28E38
// ~107 ms, ditunggu di Stop). alarm_rearm() memilih target terdekat; alarm_flag() lalu menandakan 'ada yang jatuh
// tempo' dan alarm_rearm() menilai ulang berdasarkan CNT RTC. Sign yang sedang berjalan saat jendela berakhir
// diselesaikan dulu (sama dengan K1/K2 WFI); PB1 turun dan T_obs dibaca setelah itu.
//
// Pemakai mendefinisikan SEBELUM include ini:
//   static int serve_one(uint32_t *lat);   // melayani satu permintaan; 0 = sukses; lat = latensi (satuan di bawah)
//   #define STOP_LAT_RTC 1                 // (opsional) lat = tik halus RTC (1/32768 s), diubah ke 'siklus' HCLK
//                                          // terukur saat laporan; tanpa ini lat = siklus DWT (hanya bila tak ada Stop di dalam)
// Memakai: hl_res, hl_lat_*, hl_clk_hz (stop_link.h), g_rx_*, g_arrived, g_pending (stop_common.h).
// ============================================================================
#pragma once

static volatile bool g_win_over;
static uint32_t g_win_end_cnt;
static volatile bool g_sign_wait;                 // sedang menunggu chip (tidur) sampai g_sign_ready
static uint32_t g_sign_ready;
static uint32_t g_n_stop, g_spurious;
static uint64_t g_t_sleep, g_uw_rem;

// Pilih target alarm terdekat yang belum lewat; tandai yang sudah lewat. Dipanggil bila ada target baru atau
// alarm_flag() terangkat.
static void alarm_rearm() {
  for (;;) {
    const uint32_t now = rtc_cnt();
    if (!g_win_over && (int32_t)(now - g_win_end_cnt) >= 0) g_win_over = true;
    if (g_sign_wait && (int32_t)(now - g_sign_ready) >= 0) g_sign_wait = false;
    bool have = false;
    uint32_t tgt = 0;
    if (!g_win_over) { tgt = g_win_end_cnt; have = true; }
    if (g_sign_wait && (!have || (int32_t)(g_sign_ready - tgt) < 0)) { tgt = g_sign_ready; have = true; }
    alarm_clear();
    if (!have) return;
    rtc_set_alarm(tgt - 1);                       // ALRF terangkat saat CNT berganti dari tgt-1 ke tgt
    if ((int32_t)(rtc_cnt() - tgt) < 0) return;   // target masih di depan: alarm akan menyala
  }                                               // sudah lewat selagi menulis ALR: nilai ulang
}

// Blok sampai ada alarm (diproses: alarm_rearm) atau, bila exit_on_pending, ada permintaan menunggu.
// Tidur Stop di antara kejadian; frame yang masuk saat terjaga diterima DMA, saat tidur lewat sinkron bit.
static void wait_event(bool exit_on_pending) {
  for (;;) {
    __disable_irq();
    if (alarm_flag()) { __enable_irq(); alarm_rearm(); return; }
    if (exit_on_pending && g_pending) { __enable_irq(); return; }
    if (!rx_idle_strict()) {                      // frame sedang masuk (mode terjaga): tunggu selesai (~3 ms)
      __enable_irq(); __ISB();
      const uint32_t c = cycles(), lim = (SystemCoreClock / 1000UL) * 8UL;
      while (!rx_idle() && (uint32_t)(cycles() - c) < lim) {}
      if (!rx_idle()) { __disable_irq(); rx_resync(); __enable_irq(); }
      continue;
    }
    const StopRes r = stop_sleep();
    g_n_stop++; g_t_sleep += r.dt_us;
    g_uw_rem += r.dt_us; uwTick += (uint32_t)(g_uw_rem / 1000); g_uw_rem %= 1000;
    if (r.cause == W_SPURIOUS) g_spurious++;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    __enable_irq();
    __ISB();
  }
}

// Tunggu chip (K2S): tidur Stop sampai ~ms lagi (kuantisasi tik 0,98 ms), jendela boleh berakhir di tengahnya.
static void wait_chip_ms(uint32_t ms) {
  g_sign_ready = rtc_cnt() + (ms * 1024UL + 999UL) / 1000UL + 1;
  g_sign_wait = true;
  alarm_rearm();
  while (g_sign_wait) wait_event(false);
}

// HCLK terukur terhadap RTC (Hz, dalam detik-RTC): siklus DWT selama 2 s RTC. Dipanggil setelah jendela.
static uint32_t measure_hclk() {
  uint32_t f = rtc_fine(), f2;
  do { f2 = rtc_fine(); } while (f2 == f);
  const uint32_t c0 = cycles(), fa = f2, tgt = fa + 65536UL;
  uint32_t fb, c1;
  do { fb = rtc_fine(); c1 = cycles(); } while ((int32_t)(fb - tgt) < 0);
  return (uint32_t)((uint64_t)(uint32_t)(c1 - c0) * 32768ULL / (uint32_t)(fb - fa));
}

volatile uint32_t g_win_n = 0, g_win_t_obs, g_win_t_sleep, g_win_arrived, g_win_served;   // dibaca lewat SWD

static void cmd_window(uint32_t secs, uint32_t p_ms, bool uart) {
  (void)p_ms; (void)uart;                         // hanya jendela dengan permintaan nyata (S)
  hl_lat_reset();
  g_rx_ok = g_rx_bad = g_rx_lost = g_rx_restart = 0;
  g_rx_have = false;
  g_arrived = 0; g_pending = false;
  g_n_stop = 0; g_spurious = 0; g_t_sleep = 0; g_uw_rem = 0;
  uint32_t served = 0, gagal = 0;

  g_win_over = false; g_sign_wait = false;
  g_win_end_cnt = rtc_cnt() + secs * 1024UL;
  alarm_rearm();
  const uint32_t f0 = rtc_fine();
#ifdef DBG_RX
  g_d_win0 = RTC->CNTL; g_d_bad_n = 0; g_d_lost_n = 0; g_d_sp_n = 0; g_d_al_mid_n = 0;
#endif
  PB1_HI();
  rx_awake_start();
  while (!g_win_over) {
    if (alarm_flag()) alarm_rearm();              // beban penuh: tanpa ini akhir jendela tak terlihat selama antrean ada
    if (g_win_over) break;
    __disable_irq();
    const bool have = g_pending;
    if (have) g_pending = false;
    __enable_irq();
    if (have) {
      uint32_t lat = 0;
      if (serve_one(&lat) == 0) { served++; hl_lat_add(lat); } else gagal++;
      continue;
    }
    wait_event(true);
  }
  const uint32_t f1 = rtc_fine();
#ifdef DBG_RX
  g_d_win1 = RTC->CNTL;
#endif
  PB1_LO();                                       // tepi turun = akhir jendela (logger)
  rx_off();
  const uint32_t t_obs = fine_to_us(f1 - f0);
  __disable_irq();
  const uint32_t ok = g_rx_ok, bad = g_rx_bad, lost = g_rx_lost, arr = g_arrived;
  __enable_irq();
  g_win_t_obs = t_obs; g_win_t_sleep = (uint32_t)g_t_sleep; g_win_arrived = arr; g_win_served = served;
  g_win_n++;
  hl_res.secs = secs; hl_res.p_ms = 1000; hl_res.uart = 1;
  hl_res.t_obs = t_obs; hl_res.t_sleep = (uint32_t)g_t_sleep;
  hl_res.act_us = t_obs > (uint32_t)g_t_sleep ? t_obs - (uint32_t)g_t_sleep : 0;
  hl_res.resid = 0;
  hl_res.arrived = arr; hl_res.served = served; hl_res.gagal = gagal;
  hl_res.wfi = g_n_stop; hl_res.spurious = g_spurious; hl_res.pin_salah = 0;
  hl_res.rx_ok = ok; hl_res.rx_bad = bad; hl_res.rx_lost = lost;
  hl_clk_hz = measure_hclk();                     // PB1 rendah: di luar jendela logger
#ifdef STOP_LAT_RTC
  // lat disimpan sebagai tik halus RTC; ubah ke 'siklus' HCLK terukur: ms = siklus / clk_hz = tik / 32768 (detik-RTC)
  for (uint32_t i = 0; i < hl_lat_n; i++) hl_lat[i] = (uint32_t)((uint64_t)hl_lat[i] * hl_clk_hz / 32768ULL);
#endif
}
