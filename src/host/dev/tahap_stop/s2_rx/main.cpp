// ============================================================================
// HOST deep sleep, LANGKAH 7 = S2: bangun dari Stop oleh FRAME attacker (EXTI3 PA3) + terima utuh.
// Rencana: docs/rencana_deep_sleep.md. Dasar: S1 (Stop + alarm RTC, LULUS), stop_common.h (alat bersama).
//
// Fungsi tahap ini satu: Stop di antara frame attacker; tiap frame (36 B, 115200) membangunkan host dari
// Stop, diterima UTUH tanpa mengubah attacker (sinkron bit di byte pertama 0xA5, lihat stop_common.h), lalu
// "kerja" sibuk S2_WORK_MS (pengganti sign) dan tidur lagi. Jendela S2_WIN_S detik diakhiri alarm RTC (event
// EXTI17); hasil tiap jendela dikirim lewat USART1 (logger merekam sebagai baris h):
//   S2,<jendela>,<t_obs_us>,<t_sleep_us>,<n_stop>,<palsu>,<ok>,<bad>,<hilang>,<restart>,<dilayani>,
//      <seq_pertama>,<seq_terakhir>,<sync_min_us>,<sync_maks_us>,<frame_wake>,<alarm_wake>
// Kebijakan: PRIMASK = 1 saat memutuskan tidur; frame yang tiba saat CPU terjaga (kerja/lapor) diterima
// DMA + interupsi TC (mode terjaga), bukan lewat jalur bangun.
// Build: pio run -e host_s2_rx    Upload: ST-Link '$' (SWD tak tersambung selama Stop: jendela aman 5 s)
// Ukur:  python tools/dev/tahap_stop/s2_rx.py --logger COM18 --attacker COM22 A|B
// ============================================================================
#include "../../../final/bersama/stop_common.h"

#ifndef S2_WIN_S
#define S2_WIN_S    10UL                        // jendela laporan (s)
#endif
#ifndef S2_WORK_MS
#define S2_WORK_MS  20UL                        // kerja per frame (pengganti sign)
#endif
#define SAFE_MS     5000UL

volatile uint32_t g_lse_ok = 0, g_win = 0;

static void work(uint32_t ms) {
  const uint32_t c0 = cycles(), lim = (SystemCoreClock / 1000UL) * ms;
  while ((uint32_t)(cycles() - c0) < lim) {}
}

static void window() {
  static uint32_t end_cnt = 0;                  // jadwal mutlak (tik RTC 1024 Hz)
  uint32_t n_stop = 0, palsu = 0, served = 0, fwake = 0, awake_n = 0;
  uint32_t sync_min = 0xFFFFFFFFUL, sync_max = 0;
  uint64_t t_sleep = 0, uw_rem = 0;
  // Baseline = snapshot saat laporan sebelumnya dibuat (bukan saat jendela ini mulai): frame yang tiba selama
  // pengiriman laporan (mode terjaga, ISR TC) tetap terhitung di jendela berikutnya, tidak jatuh di celah.
  static uint32_t ok0 = 0, bad0 = 0, lost0 = 0, rs0 = 0, last0 = 0;
  const uint32_t f0 = rtc_fine();
  if (end_cnt == 0) end_cnt = rtc_cnt();
  end_cnt += S2_WIN_S * 1024UL;
  if ((int32_t)(end_cnt - rtc_cnt()) < 4) end_cnt = rtc_cnt() + 4;
  alarm_clear();
  rtc_set_alarm(end_cnt - 1);                   // ALRF naik saat CNT berganti dari ALR ke ALR+1
  PB1_HI();
  for (;;) {
    __disable_irq();
    if (alarm_flag()) { __enable_irq(); break; }
    if (g_pending) {
      g_pending = false; __enable_irq();
      work(S2_WORK_MS); served++;
      continue;
    }
    if (!rx_idle()) {                           // frame sedang masuk (mode terjaga): tunggu selesai (~3 ms)
      __enable_irq(); __ISB();
      const uint32_t c = cycles(), lim = (SystemCoreClock / 1000UL) * 8UL;
      while (!rx_idle() && (uint32_t)(cycles() - c) < lim) {}
      if (!rx_idle()) { __disable_irq(); rx_resync(); __enable_irq(); }   // byte nyasar: pulihkan perataan
      continue;
    }
    const StopRes r = stop_sleep();
    n_stop++; t_sleep += r.dt_us;
    uw_rem += r.dt_us; uwTick += (uint32_t)(uw_rem / 1000); uw_rem %= 1000;
    if (r.cause == W_SPURIOUS) palsu++;
    if (r.cause == W_FRAME_OK || r.cause == W_FRAME_BAD) {
      fwake++;
      if (r.sync_us < sync_min) sync_min = r.sync_us;
      if (r.sync_us > sync_max) sync_max = r.sync_us;
    }
    if (r.cause == W_ALARM) awake_n++;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    __enable_irq();
    __ISB();
  }
  const uint32_t t_obs = fine_to_us(rtc_fine() - f0);
  __disable_irq();
  const uint32_t ok_n = g_rx_ok, bad_n = g_rx_bad, lost_n = g_rx_lost, rs_n = g_rx_restart, last_n = g_rx_last;
  __enable_irq();
  g_win++;
  char b[220];
  snprintf(b, sizeof b, "S2,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
           (unsigned long)g_win, (unsigned long)t_obs, (unsigned long)t_sleep, (unsigned long)n_stop,
           (unsigned long)palsu, (unsigned long)(ok_n - ok0), (unsigned long)(bad_n - bad0),
           (unsigned long)(lost_n - lost0), (unsigned long)(rs_n - rs0), (unsigned long)served,
           (unsigned long)last0, (unsigned long)last_n,
           (unsigned long)(sync_min == 0xFFFFFFFFUL ? 0 : sync_min), (unsigned long)sync_max,
           (unsigned long)fwake, (unsigned long)awake_n);
  ok0 = ok_n; bad0 = bad_n; lost0 = lost_n; rs0 = rs_n; last0 = last_n;
  link_tx(b);
}

void setup() {
  usb_off();
  cyccnt_start();
  pins_init();
  link_init();
  PB1_HI();
  const uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < SAFE_MS) {}   // jendela aman SWD
  DBGMCU->CR = 0;
  g_lse_ok = lse_rtc_start() ? 1 : 0;
  rx_hw_init();
  rx_awake_start();
  char b[96];
  snprintf(b, sizeof b, "S2,MULAI,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)g_lse_ok, (unsigned long)SystemCoreClock,
           (unsigned long)S2_WIN_S, (unsigned long)S2_WORK_MS, (unsigned long)g_brr_hsi);
  link_tx(b);
}

void loop() {
  if (!g_lse_ok) { PB1_LO(); for (;;) {} }
  window();
}
