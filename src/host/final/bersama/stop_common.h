// ============================================================================
// HOST deep sleep (Stop mode): alat dasar bersama S2 dan integrasi K1/K2 (langkah 7-9).
// Rencana: docs/rencana_deep_sleep.md. Dasar teruji: S0a (LSE/RTC), S0 (lantai Stop), S1 (alarm RTC).
//
// Isi:
//   - RTC dari LSE (CNT 1024 Hz, alarm = event EXTI17), waktu halus 1/32768 s.
//   - Jam: bangun dari Stop selalu HSI 8 MHz; PLL 48 MHz dipulihkan lagi (pll_start / pll_switch).
//   - Penerima frame attacker (USART2 + DMA1 kanal 6) dengan DUA mode:
//       TERJAGA  : jam PLL, BRR 24 MHz, DMA 36 B + interupsi TC (sama dengan host_rx.h): frame yang tiba
//                  saat CPU bekerja (mis. sedang sign) diterima biasa.
//       TIDUR    : USART mati. PA3 dipantau EXTI3 sebagai EVENT (bangun dari Stop lewat WFE). Tepi turun
//                  bit start byte pertama membangunkan CPU ~10 us kemudian, saat USART belum menyala.
//                  Byte pertama frame SELALU 0xA5 (FRAME_SOF), jadi polanya diketahui:
//                    start 0 | b0..b7 = 1 0 1 0 0 1 0 1 | stop 1   -> garis TINGGI 17,4 us di akhir byte
//                  (bit7 + stop), lalu bit start byte ke-2. Firmware (di HSI 8 MHz, BRR 4 MHz) menunggu
//                  garis tinggi tak putus >= 11 us (hanya terjadi di akhir byte pertama: tinggi lain
//                  di dalam byte <= 8,7 us), lalu menyalakan RE dan DMA 35 B (byte 1..35). Byte 0 diisi
//                  0xA5 oleh firmware. Tanpa mengubah attacker/protokol.
//     Jam tetap HSI selama 35 byte (3,0 ms); PLL dinyalakan di latar belakang dan dialihkan sesudah
//     DMA selesai (BRR tak boleh berubah di tengah frame).
// Pemakaian: lihat src/host/dev/tahap_stop/s2_rx/main.cpp. Makro yang harus ada sebelum include: tidak ada.
// ============================================================================
#pragma once
#include <Arduino.h>
#include "../../bersama/host_util.h"
#include <proto_attacker_host.h>
#include <proto_host_logger.h>

#define PB1_HI() (GPIOB->BSRR = GPIO_BSRR_BS1)
#define PB1_LO() (GPIOB->BSRR = GPIO_BSRR_BR1)
#define PA3_HIGH() ((GPIOA->IDR & GPIO_IDR_IDR3) != 0)

#define RTC_PRL_VAL     31UL                     // 32768 / 32 = 1024 Hz
#define LSE_TIMEOUT_MS  10000UL
#define SYNC_MIN_US     40UL                     // jendela wajar waktu sinkron sejak bangun (us)
#define SYNC_MAX_US     85UL
#define SYNC_TIMEOUT_US 300UL
#define WAKE_RX_TIMEOUT_US 6000UL                // frame 36 B = 3,1 ms; lebih dari ini = gagal

extern "C" { extern __IO uint32_t uwTick; }

// ---------------------------------------------------------------- RTC
static void rtc_wait_write() { while (!(RTC->CRL & RTC_CRL_RTOFF)) {} }
static void rtc_sync() { RTC->CRL &= ~RTC_CRL_RSF; while (!(RTC->CRL & RTC_CRL_RSF)) {} }

static uint32_t rtc_fine() {                     // 1/32768 s, 32 bit
  uint32_t h, l, d, h2, l2;
  do { h = RTC->CNTH; l = RTC->CNTL; d = RTC->DIVL; h2 = RTC->CNTH; l2 = RTC->CNTL; } while (h != h2 || l != l2);
  return (((h << 16) | l) << 5) + (RTC_PRL_VAL - (d & 0x1F));
}
static uint32_t rtc_cnt() {
  uint32_t h, l, h2;
  do { h = RTC->CNTH; l = RTC->CNTL; h2 = RTC->CNTH; } while (h != h2);
  return (h << 16) | l;
}
static inline uint32_t fine_to_us(uint32_t f) { return (uint32_t)((uint64_t)f * 1000000ULL / 32768ULL); }

static void rtc_set_alarm(uint32_t cnt) {
  rtc_wait_write();
  RTC->CRL |= RTC_CRL_CNF;
  RTC->ALRH = cnt >> 16;
  RTC->ALRL = cnt & 0xFFFF;
  RTC->CRL &= ~RTC_CRL_CNF;
  rtc_wait_write();
}
static inline bool alarm_flag() { return (RTC->CRL & RTC_CRL_ALRF) != 0; }
static void alarm_clear() { rtc_wait_write(); RTC->CRL &= ~RTC_CRL_ALRF; rtc_wait_write(); }

// LSE dinyalakan SEKALI saat boot (start dingin 3,5-4,1 s; batas 10 s). Alarm = EVENT EXTI17 (tanpa NVIC).
static bool lse_rtc_start() {
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  (void)RCC->APB1ENR;
  PWR->CR |= PWR_CR_DBP;
  RCC->BDCR |= RCC_BDCR_BDRST;
  RCC->BDCR &= ~RCC_BDCR_BDRST;
  RCC->BDCR |= RCC_BDCR_LSEON;
  const uint32_t c0 = cycles(), lim = (SystemCoreClock / 1000UL) * LSE_TIMEOUT_MS;
  while (!(RCC->BDCR & RCC_BDCR_LSERDY)) if ((uint32_t)(cycles() - c0) > lim) return false;
  RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN;
  rtc_sync();
  rtc_wait_write();
  RTC->CRL |= RTC_CRL_CNF;
  RTC->PRLH = 0; RTC->PRLL = RTC_PRL_VAL;
  RTC->CNTH = 0; RTC->CNTL = 0;
  RTC->CRL &= ~RTC_CRL_CNF;
  rtc_wait_write();
  RTC->CRL &= ~(RTC_CRL_ALRF | RTC_CRL_SECF | RTC_CRL_OWF);
  RTC->CRH = RTC_CRH_ALRIE;
  rtc_wait_write();
  EXTI->IMR &= ~EXTI_IMR_MR17;                   // bukan interupsi
  EXTI->EMR |= EXTI_EMR_MR17;                    // event: membangunkan WFE dari Stop
  EXTI->RTSR |= EXTI_RTSR_TR17;
  EXTI->PR = EXTI_PR_PR17;
  return true;
}

// ---------------------------------------------------------------- jam
// Bangun dari Stop = HSI 8 MHz, PLL mati. CFGR (sumber/pengali PLL, prescaler) tetap tersimpan.
static inline void pll_start() { RCC->CR |= RCC_CR_PLLON; }
static inline void pll_switch() {
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1) | RCC_CFGR_PPRE1_DIV2;     // APB1 /2 sebelum SYSCLK = 48 MHz
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

// ---------------------------------------------------------------- pin, USB
static void usb_off() {
  NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
  NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
  NVIC_DisableIRQ(USBWakeUp_IRQn);
  USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
  USB->ISTR = 0;
  RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
}

// Analog semua pin kecuali PA3 (RX attacker, input mengambang), PA9/PA10 (USART1 logger), PA13/PA14 (SWD),
// PB1 (status). PB1 keluaran.
static void pins_init() {
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  (void)RCC->APB2ENR;
  AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
  GPIOA->CRL = (0x4UL << 12);                   // PA3 input mengambang
  GPIOA->CRH = (0xAUL << 4) | (0x8UL << 8) | (GPIOA->CRH & ((0xFUL << 20) | (0xFUL << 24)));
  GPIOA->ODR = GPIO_ODR_ODR10;                  // PA10 pull-up
  GPIOB->CRL = (0x2UL << 4);
  GPIOB->CRH = 0; GPIOB->ODR = 0;
  GPIOC->CRL = 0; GPIOC->CRH = 0; GPIOC->ODR = 0;
  AFIO->EXTICR[0] &= ~(0xFUL << 12);            // EXTI3 <- PA3
  EXTI->IMR &= ~EXTI_IMR_MR3;
  EXTI->EMR &= ~EXTI_EMR_MR3;
  EXTI->RTSR &= ~EXTI_RTSR_TR3;
  EXTI->FTSR |= EXTI_FTSR_TR3;                  // tepi turun = bit start
}

// ---------------------------------------------------------------- USART1 ke logger (polling)
static void link_init() {
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  (void)RCC->APB2ENR;
  USART1->CR1 = 0; USART1->CR2 = 0; USART1->CR3 = 0;
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + LINK_BAUD / 2) / LINK_BAUD;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}
static void link_tx(const char *s) {
  while (*s) { while (!(USART1->SR & USART_SR_TXE)) {} USART1->DR = (uint8_t)*s++; }
  while (!(USART1->SR & USART_SR_TC)) {}
}

// ---------------------------------------------------------------- penerima frame
static uint32_t g_brr_pll, g_brr_hsi;
static uint8_t  g_rx[FRAME_LEN];
volatile uint32_t g_rx_ok, g_rx_bad, g_rx_lost, g_rx_restart, g_rx_first, g_rx_last;
volatile bool     g_rx_have;
volatile uint32_t g_arrived;
volatile bool     g_pending;

static void rx_hw_init() {                       // setelah jam PLL 48 MHz berjalan
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  const uint32_t p1 = HAL_RCC_GetPCLK1Freq();
  g_brr_pll = (p1 + FRAME_BAUD / 2) / FRAME_BAUD;
  g_brr_hsi = (8000000UL + FRAME_BAUD / 2) / FRAME_BAUD;    // di HSI APB1 dibuat /1 -> PCLK1 8 MHz (BRR 69: galat +0,6 %)
  USART2->CR1 = 0; USART2->CR2 = 0;
  USART2->BRR = g_brr_pll;
  USART2->CR3 = USART_CR3_DMAR;
  USART2->CR1 = USART_CR1_UE;
  DMA1_Channel6->CCR = 0;
  DMA1_Channel6->CPAR = (uint32_t)&USART2->DR;
  NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
  NVIC_EnableIRQ(DMA1_Channel6_IRQn);
}

static inline void dma_arm(uint8_t *dst, uint32_t n, bool tcie) {
  DMA1_Channel6->CCR = 0;
  DMA1->IFCR = DMA_IFCR_CGIF6;
  DMA1_Channel6->CMAR = (uint32_t)dst;
  DMA1_Channel6->CNDTR = n;
  DMA1_Channel6->CCR = DMA_CCR_MINC | (tcie ? DMA_CCR_TCIE : 0) | DMA_CCR_EN;
}

// Mode TERJAGA: PLL, RE menyala, DMA 36 B + TC.
static void rx_awake_start() {
  USART2->CR1 &= ~USART_CR1_RE;
  USART2->BRR = g_brr_pll;
  USART2->CR3 = USART_CR3_DMAR;
  (void)USART2->SR; (void)USART2->DR;
  NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
  dma_arm(g_rx, FRAME_LEN, true);
  USART2->CR1 |= USART_CR1_RE;
}
static void rx_awake_rearm() { dma_arm(g_rx, FRAME_LEN, true); }

static void rx_off() {
  USART2->CR1 &= ~USART_CR1_RE;
  DMA1_Channel6->CCR = 0;
  DMA1->IFCR = DMA_IFCR_CGIF6;
}

// Tak ada frame setengah jalan: DMA belum menerima byte dan garis RX tinggi.
static inline bool rx_idle() { return DMA1_Channel6->CNDTR == FRAME_LEN && PA3_HIGH(); }

// Idle KETAT (18 Nov): rx_idle() hanya melihat satu sampel PA3 + CNDTR, sehingga di 87 us pertama sebuah frame (byte
// 0xA5 belum selesai, PA3 sering tinggi) host salah menganggap garis diam, tidur di TENGAH frame, bangun di tepi acak,
// dan pola sinkron kadang cocok palsu -> frame rusak (diagnosa: bangun palsu lalu tidur lagi 5-21 us kemudian).
// Di sini PA3 harus tinggi terus 120 us (> run tinggi terpanjang di dalam frame ~ 9-10 bit = 87 us).
static bool rx_idle_strict() {
  if (DMA1_Channel6->CNDTR != FRAME_LEN) return false;
  const uint32_t t = cycles(), lim = (SystemCoreClock / 1000000UL) * 120UL;
  while ((uint32_t)(cycles() - t) < lim) {
    if (!PA3_HIGH()) return false;
  }
  return DMA1_Channel6->CNDTR == FRAME_LEN;
}

static bool frame_valid() {
  uint8_t x = 0;
  for (int i = 0; i < FRAME_LEN - 1; i++) x ^= g_rx[i];
  return g_rx[0] == FRAME_SOF && x == g_rx[FRAME_LEN - 1];
}
#ifdef DBG_RX
// Diagnostik (18 Nov): kapan dan lewat jalur mana frame rusak/hilang pertama terjadi (dibaca lewat SWD).
volatile uint32_t g_d_bad_n, g_d_bad_path, g_d_bad_t, g_d_bad_ndtr, g_d_bad_rx[10];
volatile uint32_t g_d_lost_n, g_d_lost_t, g_d_lost_d, g_d_lost_seq, g_d_win0, g_d_win1;
volatile uint32_t g_d_al_mid_n, g_d_al_mid_us;
volatile uint32_t g_d_sp_n, g_d_sp_sync, g_d_sp_flag, g_d_sp_lvl, g_d_sp_d1, g_d_sp_d2, g_d_sp_d3, g_d_sp_gap, g_d_sp_prev, g_d_sp_t, g_d_sp_edges;
volatile uint32_t g_d_entry_c, g_d_prev_exit_c, g_d_prev_cause, g_d_bad_gap_us, g_d_bad_prevcause, g_d_bad_sync, g_d_bad_lvl, g_d_bad_done;
static void dbg_bad(uint32_t path) {
  if (g_d_bad_n++ == 0) {
    g_d_bad_path = path; g_d_bad_t = RTC->CNTL; g_d_bad_ndtr = DMA1_Channel6->CNDTR;
    for (int i = 0; i < 10; i++) g_d_bad_rx[i] = ((volatile uint32_t *)g_rx)[i];
  }
}
#define DBG_BAD(p) dbg_bad(p)
#else
#define DBG_BAD(p) ((void)0)
#endif
static void frame_commit() {
  const uint16_t seq = (uint16_t)g_rx[1] | ((uint16_t)g_rx[2] << 8);
  if (g_rx_have) {
    const uint16_t d = (uint16_t)(seq - (uint16_t)(g_rx_last + 1));
    if (d < 1000) g_rx_lost += d; else g_rx_restart++;       // lompatan besar = attacker dimulai ulang
#ifdef DBG_RX
    if (d >= 1 && d < 1000 && g_d_lost_n++ == 0) { g_d_lost_t = RTC->CNTL; g_d_lost_d = d; g_d_lost_seq = seq; }
#endif
  } else { g_rx_first = seq; g_rx_have = true; }
  g_rx_last = seq;
  g_rx_ok++; g_arrived++; g_pending = true;
}

// Buang byte sampai garis diam (IDLE) atau 10 ms (jam inti sekarang).
static void rx_resync() {
  USART2->CR3 &= ~USART_CR3_DMAR;
  (void)USART2->SR; (void)USART2->DR;
  const uint32_t t = cycles(), lim = SystemCoreClock / 100UL;
  while (!(USART2->SR & USART_SR_IDLE) && (uint32_t)(cycles() - t) < lim) {
    if (USART2->SR & USART_SR_RXNE) (void)USART2->DR;
  }
  (void)USART2->SR; (void)USART2->DR;
  dma_arm(g_rx, FRAME_LEN, true);
  USART2->CR3 |= USART_CR3_DMAR;
}

extern "C" void DMA1_Channel6_IRQHandler(void) {
  DMA1->IFCR = DMA_IFCR_CGIF6;
  if (frame_valid()) { frame_commit(); rx_awake_rearm(); }
  else { g_rx_bad++; DBG_BAD(1); rx_resync(); }
}

// ---------------------------------------------------------------- tidur Stop
#ifdef S2_EDGETRACE
volatile uint32_t g_trace[40], g_trace_n;
#endif
#ifdef S2_TRACE
struct WLog { uint32_t t_fine; uint8_t cause, lvl, ret_imm, pad; uint16_t sync_us; };
volatile WLog g_wlog[48]; volatile uint32_t g_wlog_n;
volatile uint8_t g_badcopy[FRAME_LEN]; volatile uint32_t g_bad_sync, g_bad_ndtr, g_bad_done, g_bad_n;
#endif
struct StopRes {
  int      cause;                                // 0 alarm, 1 frame ok, 2 frame bad, 3 bangun palsu
  uint32_t dt_us;                                // lama Stop terukur RTC (us), sudah dikurangi waktu sejak bangun
  uint32_t sync_us;                              // (frame) waktu sejak bangun sampai garis tinggi >= SYNC_HI_US
};
enum { W_ALARM = 0, W_FRAME_OK = 1, W_FRAME_BAD = 2, W_SPURIOUS = 3 };

// Dipanggil dengan PRIMASK = 1, mode TERJAGA (PLL), tak ada frame setengah jalan, tak ada alarm tertunda.
// Kembali dengan PRIMASK = 1, PLL aktif, mode TERJAGA (DMA terpasang ulang), SysTick belum dinyalakan.
//
// CATATAN KEANDALAN (18 Nov, TUNTAS): tanpa landasan NOP, instruksi pertama sesudah bangun dari Stop kadang
// salah terbaca/tereksekusi (nilai register 'berubah': add #0x1000 menjadi #0x4000, ISB dianggap instruksi tak
// dikenal, dll.), bergantung susunan kode: S2 lulus, K1S/K2S crash deterministik pada frame pertama. Penyebab
// bukan ST-Link (crash juga tanpa ST-Link), bukan LPDS, bukan buffer tulis. Obat: SEV+WFE+WFE dan 24 NOP 16-bit
// dalam SATU blok asm, sehingga tak ada instruksi 32-bit (yang bisa melintasi batas baris fetch) di awal
// pemulihan. Sesudah itu K2S lulus penuh (59/59, S_cpu 98,19 % = logger). Percobaan yang tidak menolong:
// clobber semua register (pembungkus fungsi), fungsi di RAM, LPDS mati, DISDEFWBUF, jeda NOP sesudah beberapa
// instruksi (instruksi 32-bit sebelum jeda yang kena).
static StopRes __attribute__((noinline)) stop_sleep() {
  StopRes r = {W_SPURIOUS, 0, 0};
#ifdef DBG_RX
  g_d_entry_c = cycles();
#endif
  rx_off();
  USART2->CR3 &= ~USART_CR3_DMAR;
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  EXTI->PR = EXTI_PR_PR3;
  EXTI->EMR |= EXTI_EMR_MR3;
  const uint32_t ts = rtc_fine();
  PWR->CR = (PWR->CR & ~PWR_CR_PDDS) | PWR_CR_LPDS | PWR_CR_CWUF;
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  PB1_LO();
  __DSB();
  // SEV (buang event stale) + WFE + WFE (tidur Stop sungguhan) + landasan NOP 16-bit: lihat catatan di bawah.
  asm volatile("sev; wfe; wfe; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;" ::: "memory");
  const uint32_t c_w = cycles();                 // jam inti = HSI 8 MHz
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1) | RCC_CFGR_PPRE1_DIV1;     // PCLK1 = 8 MHz: BRR lebih halus (galat baud < 1 %)
  const uint32_t lvl_w = PA3_HIGH() ? 1 : 0;
  PB1_HI();
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
  EXTI->EMR &= ~EXTI_EMR_MR3;
  EXTI->PR = EXTI_PR_PR3;
  pll_start();

  uint32_t c_sw = c_w;                           // DWT tepat setelah SYSCLK pindah ke PLL
  if (!alarm_flag()) {
    // ------------------------------------------------------- frame: sinkron bit di HSI 8 MHz
    USART2->BRR = g_brr_hsi;
    g_rx[0] = FRAME_SOF;
    dma_arm(g_rx + 1, FRAME_LEN - 1, false);
    USART2->CR3 |= USART_CR3_DMAR;
    (void)USART2->SR; (void)USART2->DR;
    const uint32_t cyc_us = 8UL;                 // siklus per us di HSI
    // Pola 0xA5 (bit 8,68 us): ... rendah 17,4 (b3,b4) | tinggi 8,7 (b5) | rendah 8,7 (b6) | TEPI NAIK (awal b7+stop,
    // tinggi 17,4 us sebelum bit start byte ke-2): RE dinyalakan ~17 us sebelum byte ke-2.
    uint32_t c = cycles(), t_e = c, d1 = 0, d2 = 0, d3 = 0;
    uint32_t lvl = PA3_HIGH() ? 1 : 0;
    bool sync = false;
    for (;;) {
      c = cycles();
      const uint32_t l = PA3_HIGH() ? 1 : 0;
      if (l != lvl) {
        d3 = d2; d2 = d1; d1 = c - t_e; t_e = c; lvl = l;
        if (l == 1 && d1 >= 5 * cyc_us && d1 <= 12 * cyc_us && d2 >= 5 * cyc_us && d2 <= 12 * cyc_us &&
            d3 >= 11 * cyc_us && d3 <= 24 * cyc_us) { sync = true; break; }
      }
      if ((uint32_t)(c - c_w) > SYNC_TIMEOUT_US * cyc_us) break;
    }
    r.sync_us = (uint32_t)(c - c_w) / cyc_us;
    if (sync && r.sync_us >= SYNC_MIN_US && r.sync_us <= SYNC_MAX_US) {
      USART2->CR1 |= USART_CR1_RE;
      const uint32_t cw0 = cycles();
      while (!(DMA1->ISR & DMA_ISR_TCIF6)) {
        if ((uint32_t)(cycles() - cw0) > WAKE_RX_TIMEOUT_US * cyc_us) break;
      }
      const bool done = (DMA1->ISR & DMA_ISR_TCIF6) != 0;
      pll_switch();
      c_sw = cycles();
      USART2->BRR = g_brr_pll;
      if (done && frame_valid()) { frame_commit(); r.cause = W_FRAME_OK; rx_awake_rearm(); }
      else {
        r.cause = W_FRAME_BAD; g_rx_bad++; DBG_BAD(2);
#ifdef DBG_RX
        if (g_d_bad_n == 1) { g_d_bad_gap_us = (g_d_entry_c - g_d_prev_exit_c) / (SystemCoreClock / 1000000UL); g_d_bad_prevcause = g_d_prev_cause; g_d_bad_sync = r.sync_us; g_d_bad_lvl = lvl_w; g_d_bad_done = done; }
#endif
#ifdef S2_TRACE
        if (g_bad_n == 0) { for (int i = 0; i < FRAME_LEN; i++) g_badcopy[i] = g_rx[i]; g_bad_sync = r.sync_us; g_bad_ndtr = DMA1_Channel6->CNDTR; g_bad_done = done; }
        g_bad_n++;
#endif
        rx_resync();
      }
    } else {
      pll_switch();
      c_sw = cycles();
      USART2->BRR = g_brr_pll;
      rx_off();
      rx_awake_start();                          // garis tak sesuai pola: terima biasa (frame ini hilang)
      r.cause = W_SPURIOUS;
#ifdef DBG_RX
      if (g_d_sp_n++ == 0) { g_d_sp_sync = r.sync_us; g_d_sp_flag = sync; g_d_sp_lvl = lvl_w; g_d_sp_d1 = d1 / 8; g_d_sp_d2 = d2 / 8; g_d_sp_d3 = d3 / 8;
        g_d_sp_gap = (g_d_entry_c - g_d_prev_exit_c) / (SystemCoreClock / 1000000UL); g_d_sp_prev = g_d_prev_cause; g_d_sp_t = RTC->CNTL; }
#endif
    }
  } else {
    // ------------------------------------------------------- alarm: pulihkan jam, lalu mode terjaga
    r.cause = W_ALARM;                           // ALRF dibiarkan terangkat: window() yang melihat dan menghapusnya
    if (PA3_HIGH()) {
      // Penerima dinyalakan SEGERA di HSI (garis diam), sebelum PLL siap: frame yang mulai selama pemulihan jam
      // (~0,3-0,5 ms, penyebab hilang 1-4 frame/600 di K2S B, 18 Nov) tetap diterima. Pindah ke PLL baru bila
      // garis diam KETAT (tinggi 120 us), supaya baud tak berubah di tengah byte.
      USART2->BRR = g_brr_hsi;
      NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
      dma_arm(g_rx, FRAME_LEN, true);
      (void)USART2->SR; (void)USART2->DR;
      USART2->CR3 |= USART_CR3_DMAR;
      USART2->CR1 |= USART_CR1_RE;
      const uint32_t t0 = cycles();
      uint32_t t_hi = t0;
      bool diam = false;
      for (;;) {
        const uint32_t c = cycles();
        if (!PA3_HIGH()) t_hi = c;
        else if ((uint32_t)(c - t_hi) > 120UL * 8UL && (RCC->CR & RCC_CR_PLLRDY)) { diam = true; break; }
        if ((uint32_t)(c - t0) > 8000UL * 8UL) break;                       // 8 ms: tak diam, menyerah
      }
      pll_switch();
      c_sw = cycles();
      USART2->BRR = g_brr_pll;
      if (!diam) { rx_off(); rx_awake_start(); }                            // garis tak pernah diam: mulai bersih
    } else {
    pll_switch();
    c_sw = cycles();
    if (!PA3_HIGH()) {                           // frame sedang jalan saat alarm: tunggu garis diam, frame ini hilang
#ifdef DBG_RX
      if (g_d_al_mid_n++ == 0) g_d_al_mid_us = (uint32_t)(cycles() - c_w) / 8;
#endif
      const uint32_t t = cycles(), lim = (SystemCoreClock / 1000UL) * 8UL;
      uint32_t t_low = t;
      while ((uint32_t)(cycles() - t) < lim) {
        if (!PA3_HIGH()) t_low = cycles();
        else if ((uint32_t)(cycles() - t_low) > SystemCoreClock / 5000UL) break;   // tinggi >= 200 us
      }
    }
    rx_awake_start();
    }
  }
  // Waktu sejak bangun: (c_sw - c_w) siklus @8 MHz + sisanya @jam PLL. DWT = HCLK.
  rtc_sync();
  const uint32_t tw = rtc_fine();
  const uint32_t c_after = cycles();
  const uint64_t el_x1000 = (uint64_t)(uint32_t)(c_sw - c_w) * 125ULL +
                            (uint64_t)(uint32_t)(c_after - c_sw) * 1000ULL / (SystemCoreClock / 1000000ULL);
  const int32_t dfine = (int32_t)(tw - ts);      // baca CNT/DIV tak atomik: tidur sangat singkat bisa terbaca -1 tik
  const uint32_t tot = dfine > 0 ? fine_to_us((uint32_t)dfine) : 0;
  const uint32_t el = (uint32_t)(el_x1000 / 1000ULL);
  r.dt_us = tot > el ? tot - el : 0;
#ifdef S2_TRACE
  { const uint32_t i = g_wlog_n % 48; g_wlog[i].t_fine = tw; g_wlog[i].cause = (uint8_t)r.cause; g_wlog[i].lvl = (uint8_t)lvl_w;
    g_wlog[i].ret_imm = (uint8_t)(r.dt_us < 100); g_wlog[i].sync_us = (uint16_t)r.sync_us; g_wlog_n++; }
#endif
#ifdef DBG_RX
  g_d_prev_exit_c = cycles(); g_d_prev_cause = r.cause;
#endif
  return r;
}
