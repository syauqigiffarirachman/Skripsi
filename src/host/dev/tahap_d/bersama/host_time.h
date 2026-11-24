// ============================================================================
// Host (K1 dan K2): waktu hardware, pin status PB1, dan sumber permintaan jendela pengamatan.
//
//   - Waktu hardware 32-bit, resolusi 1 us: TIM2 (16-bit, PSC -> 1 MHz) -> TRGO update -> TIM3
//     (mode clock eksternal 1, ITR1) = 16 bit atas. Tanpa interupsi, tak membangunkan CPU, dan
//     TETAP BERJALAN saat WFI (dipakai mengakumulasi T_sleep).
//   - Pin status PB1: 1 saat CPU aktif, 0 saat akan WFI. Logger menangkap tepinya (PA0) sebagai
//     pembanding independen %S_cpu.
//   - Keadaan permintaan jendela: g_pending (ada permintaan menunggu) dan g_end (jendela usai).
//     Hanya ada SATU flag, tanpa antrean: permintaan yang tiba saat sudah ada yang menunggu
//     menyatu, jadi terlewat = datang - dilayani. Ini variabel kontrol (sama di K1 dan K2).
//   - Permintaan sintetis (perintah W): TIM4 interupsi tiap p ms (tick_isr). Tik ke-K mengakhiri
//     jendela dan bukan permintaan.
//   - Permintaan nyata (perintah A / S): frame attacker lewat USART2 (host_rx.h); TIM4 hanya
//     satu tembakan di akhir jendela (end_isr), supaya tak ada bangun tambahan.
// ============================================================================
#pragma once
#include <Arduino.h>

#define PIN_STATUS_HI() (GPIOB->BSRR = GPIO_BSRR_BS1)
#define PIN_STATUS_LO() (GPIOB->BSRR = GPIO_BSRR_BR1)

// false bila jam pewaktu APB1 bukan kelipatan 1 MHz (resolusi 1 us tak tercapai).
static bool hw_time_init() {
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  const uint32_t tclk  = (RCC->CFGR & RCC_CFGR_PPRE1) ? 2 * pclk1 : pclk1;   // jam pewaktu APB1
  if (tclk == 0 || (tclk % 1000000UL) != 0) return false;
  TIM2->CR1 = 0; TIM3->CR1 = 0;
  TIM2->PSC = tclk / 1000000UL - 1;
  TIM2->ARR = 0xFFFF;
  TIM2->CR2 = TIM_CR2_MMS_1;                    // TRGO = update
  TIM2->EGR = TIM_EGR_UG;                       // muat PSC (TIM3 belum jadi budak)
  TIM3->PSC = 0;
  TIM3->ARR = 0xFFFF;
  TIM3->SMCR = TIM_SMCR_TS_0;                   // trigger = ITR1 (TIM2 TRGO)
  TIM3->SMCR |= TIM_SMCR_SMS;                   // SMS=111: clock eksternal mode 1
  TIM2->CNT = 0; TIM3->CNT = 0;
  TIM3->CR1 = TIM_CR1_CEN;
  TIM2->CR1 = TIM_CR1_CEN;
  return true;
}

// Waktu hardware 32-bit (us). Ulangi bila bagian atas berubah di tengah baca, atau bila bagian
// bawah baru saja membungkus (TRGO ke TIM3 butuh beberapa siklus).
static inline uint32_t hw_us() {
  uint32_t hi, lo, hi2;
  do { hi = TIM3->CNT; lo = TIM2->CNT; hi2 = TIM3->CNT; } while (hi != hi2 || lo < 4);
  return (hi << 16) | lo;
}

// ---------------------------------------------------------------------------
// Keadaan permintaan satu jendela (ditulis ISR, dibaca loop jendela di main.cpp).
static volatile uint32_t g_ticks, g_K, g_arrived, g_t_last, g_t_first, g_tick_min, g_tick_max;
static volatile bool     g_pending, g_end;

// Permintaan sintetis: TIM4 tiap p ms. Juga mencatat selang tik (pertama/min/maks) untuk diagnosis.
static void tick_isr() {
  const uint32_t t = hw_us();
  const uint32_t dt = t - g_t_last;
  g_t_last = t;
  if (g_ticks == 0) g_t_first = dt;
  else { if (dt < g_tick_min) g_tick_min = dt; if (dt > g_tick_max) g_tick_max = dt; }
  g_ticks++;
  if (g_ticks >= g_K) g_end = true;
  else { g_arrived++; g_pending = true; }
}
