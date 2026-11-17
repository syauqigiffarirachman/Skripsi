// ============================================================================
// HOST deep sleep, LANGKAH 6 = S1: Stop + bangun dari ALARM RTC + pulih 48 MHz + T_sleep dari RTC.
// Rencana: docs/rencana_deep_sleep.md. Dasar: S0a (LSE/RTC, LULUS) dan S0 (Stop, lantai 1,99 mA, LULUS).
//
// Fungsi tahap ini satu: siklus periodik bangun-kerja-tidur dengan Stop, dan waktu tidur yang diukur host
// (RTC) cocok dengan pengukuran independen logger (PB1, kristal 72 MHz).
//   - RTC dari LSE, PRL = 31 -> CNT 1024 Hz; waktu halus = CNT x 32 + (31 - DIV) dalam 1/32768 s (30,5 us).
//     Setelah bangun dari Stop register RTC disinkronkan dulu (RSF) sebelum dibaca.
//   - Siklus: alarm tiap S1_P_TICK tik (1024 = 1 s, jadwal mutlak, tidak menumpuk galat) -> bangun ->
//     pulih PLL 48 MHz -> PB1 = 1 -> kerja sibuk S1_WORK_MS (pengganti sign) -> PB1 = 0 -> Stop.
//   - PB1: 1 = aktif, 0 = tidur. PB1 turun TEPAT sebelum WFI dan naik SETELAH waktu bangun dibaca, jadi
//     lama PB1 = 0 di logger = T_sleep host (termasuk sinkron RSF ~61 us dan pulih PLL).
//   - Kebijakan tidur sama dengan K1: PRIMASK = 1 -> cek flag -> SysTick mati -> WFI (Stop) -> bangun
//     -> SysTick nyala -> uwTick dikoreksi -> PRIMASK = 0 (ISR alarm berjalan).
//   - Tiap jendela S1_K siklus host mengirim lewat USART1 (logger merekam sebagai baris h):
//       S1,<jendela>,<K>,<t_obs_us>,<t_sleep_us>,<n_stop>,<bangun_palsu>,<pulih_maks_us>,<tidur_min_us>,<tidur_maks_us>
//   - USB CDC dimatikan di awal (build tetap pakai flag CDC agar jam = K1/K2: HSI/2 x 12 = 48 MHz).
//   - Pin yang tak dipakai = analog (seperti S0). Jendela aman 5 s setelah boot untuk flash ulang.
//
// Build: pio run -e host_s1_rtc    Upload: pio run -e host_s1_rtc -t upload   Ukur: python tools/s1_rtc.py
// ============================================================================
#include <Arduino.h>
#include "../../../host_common/host_util.h"
#include <proto_host_logger.h>

#ifndef S1_P_TICK
#define S1_P_TICK   1024UL                      // periode siklus (tik RTC 1024 Hz): 1024 = 1 s
#endif
#ifndef S1_WORK_MS
#define S1_WORK_MS  50UL                        // kerja sibuk per siklus (ms)
#endif
#ifndef S1_K
#define S1_K        30UL                        // siklus per jendela laporan
#endif
#define SAFE_MS        5000UL
#define LSE_TIMEOUT_MS 10000UL
#define RTC_PRL        31UL                     // 32768 / 32 = 1024 Hz

#define PB1_HI() (GPIOB->BSRR = GPIO_BSRR_BS1)
#define PB1_LO() (GPIOB->BSRR = GPIO_BSRR_BR1)

extern "C" { extern __IO uint32_t uwTick; }

volatile uint32_t g_lse_ok = 0, g_alarm = 0, g_win = 0, g_err = 0;

extern "C" void RTC_Alarm_IRQHandler(void) {
  RTC->CRL &= ~RTC_CRL_ALRF;
  EXTI->PR = EXTI_PR_PR17;
  g_alarm = 1;
}

// ---------------------------------------------------------------- RTC
static void rtc_wait_write() { while (!(RTC->CRL & RTC_CRL_RTOFF)) {} }
static void rtc_sync() { RTC->CRL &= ~RTC_CRL_RSF; while (!(RTC->CRL & RTC_CRL_RSF)) {} }

// Waktu halus 32-bit dalam 1/32768 s (meluap tiap ~36 jam; selisih tak bertanda tetap benar).
static uint32_t rtc_fine() {
  uint32_t h, l, d, h2, l2;
  do {
    h = RTC->CNTH; l = RTC->CNTL; d = RTC->DIVL;
    h2 = RTC->CNTH; l2 = RTC->CNTL;
  } while (h != h2 || l != l2);
  return (((h << 16) | l) << 5) + (RTC_PRL - (d & 0x1F));
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

static bool lse_rtc_start() {
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  (void)RCC->APB1ENR;
  PWR->CR |= PWR_CR_DBP;
  RCC->BDCR |= RCC_BDCR_BDRST;                  // keadaan domain backup selalu sama tiap boot
  RCC->BDCR &= ~RCC_BDCR_BDRST;
  RCC->BDCR |= RCC_BDCR_LSEON;
  const uint32_t c0 = cycles(), lim = (SystemCoreClock / 1000UL) * LSE_TIMEOUT_MS;
  while (!(RCC->BDCR & RCC_BDCR_LSERDY)) if ((uint32_t)(cycles() - c0) > lim) return false;
  RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN;
  rtc_sync();
  rtc_wait_write();
  RTC->CRL |= RTC_CRL_CNF;
  RTC->PRLH = 0; RTC->PRLL = RTC_PRL;
  RTC->CNTH = 0; RTC->CNTL = 0;
  RTC->CRL &= ~RTC_CRL_CNF;
  rtc_wait_write();
  RTC->CRL &= ~(RTC_CRL_ALRF | RTC_CRL_SECF | RTC_CRL_OWF);
  RTC->CRH = RTC_CRH_ALRIE;
  rtc_wait_write();
  EXTI->IMR |= EXTI_IMR_MR17;                   // alarm RTC = EXTI 17 (satu-satunya jalan bangun dari Stop)
  EXTI->RTSR |= EXTI_RTSR_TR17;
  EXTI->PR = EXTI_PR_PR17;
  NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);
  NVIC_SetPriority(RTC_Alarm_IRQn, 0);
  NVIC_EnableIRQ(RTC_Alarm_IRQn);
  return true;
}

// ---------------------------------------------------------------- jam, pin, USB
// Bangun dari Stop selalu memakai HSI 8 MHz; CFGR (sumber/pengali PLL, prescaler) tetap tersimpan.
static void clock_restore() {
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

static void usb_off() {
  NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
  NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
  NVIC_DisableIRQ(USBWakeUp_IRQn);
  USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;    // transceiver dimatikan
  USB->ISTR = 0;
  RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
}

// Semua pin analog kecuali PA9/PA10 (USART1 logger), PA13/PA14 (SWD), PB1 (status).
static void pins_init() {
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  (void)RCC->APB2ENR;
  AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
  GPIOA->CRL = 0;
  GPIOA->CRH = (0xAUL << 4) | (0x8UL << 8) | (GPIOA->CRH & ((0xFUL << 20) | (0xFUL << 24)));
  GPIOA->ODR = GPIO_ODR_ODR10;                  // PA10 pull-up
  GPIOB->CRL = (0x2UL << 4);                    // PB1 output push-pull 2 MHz
  GPIOB->CRH = 0; GPIOB->ODR = 0;
  GPIOC->CRL = 0; GPIOC->CRH = 0; GPIOC->ODR = 0;
}

// ---------------------------------------------------------------- USART1 ke logger
static void link_init() {
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  (void)RCC->APB2ENR;
  USART1->CR1 = 0; USART1->CR2 = 0; USART1->CR3 = 0;
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + LINK_BAUD / 2) / LINK_BAUD;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}
static void link_tx(const char *s) {
  while (*s) { while (!(USART1->SR & USART_SR_TXE)) {} USART1->DR = (uint8_t)*s++; }
  while (!(USART1->SR & USART_SR_TC)) {}        // tuntas sebelum tidur (jam berhenti di Stop)
}

// ---------------------------------------------------------------- satu jendela
static void window() {
  uint32_t n_stop = 0, spurious = 0, rest_max = 0, sl_min = 0xFFFFFFFFUL, sl_max = 0;
  uint64_t t_sleep = 0, uw_rem = 0;
  const uint32_t f0 = rtc_fine();
  uint32_t next = rtc_cnt() + 2;                // jadwal mutlak mulai sedikit di depan
  PB1_HI();
  for (uint32_t k = 0; k < S1_K; k++) {
    next += S1_P_TICK;
    if ((int32_t)(next - rtc_cnt()) < 2) { g_err++; next = rtc_cnt() + 2; }   // kerja lewat jadwal
    rtc_set_alarm(next - 1);                    // ALRF terangkat saat CNT berganti dari ALR ke ALR+1
    g_alarm = 0;
    for (;;) {
      __disable_irq();
      if (g_alarm) { __enable_irq(); break; }
      SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
      const uint32_t ts = rtc_fine();
      PWR->CR = (PWR->CR & ~PWR_CR_PDDS) | PWR_CR_LPDS | PWR_CR_CWUF;
      SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
      PB1_LO();
      __DSB();
      __WFI();
      SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
      const uint32_t c_w = cycles();            // DWT berhenti di Stop; pulih diukur dari sini
      clock_restore();
      rtc_sync();
      const uint32_t tw = rtc_fine();
      PB1_HI();
      const uint32_t rest_us = (uint32_t)(cycles() - c_w) / 8U;   // kasar: sebagian di 8 MHz, sebagian 48 MHz
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
      const uint32_t dt = fine_to_us(tw - ts);
      t_sleep += dt; n_stop++;
      if (dt < sl_min) sl_min = dt;
      if (dt > sl_max) sl_max = dt;
      if (rest_us > rest_max) rest_max = rest_us;
      uw_rem += dt; uwTick += (uint32_t)(uw_rem / 1000); uw_rem %= 1000;
      __enable_irq();
      __ISB();
      if (!g_alarm) spurious++;
    }
    const uint32_t c0 = cycles(), lim = (SystemCoreClock / 1000UL) * S1_WORK_MS;
    while ((uint32_t)(cycles() - c0) < lim) {}  // "kerja" (pengganti sign)
  }
  const uint32_t t_obs = fine_to_us(rtc_fine() - f0);
  g_win++;
  char b[128];
  snprintf(b, sizeof b, "S1,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)g_win, (unsigned long)S1_K,
           (unsigned long)t_obs, (unsigned long)t_sleep, (unsigned long)n_stop, (unsigned long)spurious,
           (unsigned long)rest_max, (unsigned long)sl_min, (unsigned long)sl_max);
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
  char b[64];
  snprintf(b, sizeof b, "S1,MULAI,%lu,%lu,%lu,%lu\n", (unsigned long)g_lse_ok, (unsigned long)SystemCoreClock,
           (unsigned long)S1_P_TICK, (unsigned long)S1_WORK_MS);
  link_tx(b);
}

void loop() {
  if (!g_lse_ok) { PB1_LO(); for (;;) {} }
  window();
}
