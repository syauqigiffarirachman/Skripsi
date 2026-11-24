// ============================================================================
// HOST deep sleep, LANGKAH 4 = S0a: uji kristal LSE 32,768 kHz (Epson MC-306) untuk RTC.
// Rencana: docs/rencana_deep_sleep.md. BELUM ada Stop mode di tahap ini (CPU tetap jalan / WFI).
//
// Fungsi tahap ini satu: membuktikan LSE selalu start dan frekuensinya benar, karena seluruh varian
// Stop (pewaktu tidur, alarm bangun, akhir jendela) akan bergantung pada RTC ber-LSE.
//
//   1. Domain backup di-reset (BDRST) supaya LSE start dari nol di SETIAP reset (bukan hanya
//      power-cycle), lalu LSEON; waktu sampai LSERDY diukur dengan DWT (jam HSI, galat ~0,5 %:
//      cukup untuk waktu start). Batas tunggu 6 s.
//   2. RTC dari LSE, PRL = 4095 -> interupsi detik (SECF) 8 Hz. ISR membalik PB1 -> gelombang
//      kotak 4 Hz (periode 250 ms) di PB1 -> logger PA0 (kabel yang sudah ada). Logger mengukur
//      periodenya dengan kristal 72 MHz: perintah 7 (rekam saja), dianalisis tools/dev/tahap_stop/s0a_lse.py.
//   3. RTCCLK/64 = 512 Hz dikeluarkan di PC13 (BKP_RTCCR.CCO) untuk osiloskop; LED PC13 ikut
//      menyala redup = tanda LSE hidup. JANGAN probe PC14/PC15 langsung.
//   4. Status tiap 1 s lewat USART1 ke logger (tercatat sebagai baris h):
//        S0A,<tik>,<lse_ok>,<lse_start_ms>,<rtc_cnt>,<bdcr_hex>
//      dan '?' dari logger dijawab ">ID,S0A,0,<SystemCoreClock>,<lse_ok>" (run_bench.py ping).
//
// Tanpa LSE (gagal start): PB1 tetap rendah, LED PC13 mati, status lse_ok=0.
// Verifikasi tanpa logger: baca g_lse_ok, g_lse_start_ms, g_tick lewat SWD.
//
// Build: pio run -e host_s0a_lse    Upload: pio run -e host_s0a_lse -t upload   (ST-Link '$')
// ============================================================================
#include <Arduino.h>
#include "../../../bersama/host_util.h"
#include <proto_host_logger.h>

#define LSE_TIMEOUT_MS 6000UL
#define RTC_PRL        4095UL                   // 32768 / (4095 + 1) = 8 Hz

volatile uint32_t g_lse_ok = 0, g_lse_start_ms = 0, g_tick = 0, g_bdcr = 0;

extern "C" void RTC_IRQHandler(void) {
  if (RTC->CRL & RTC_CRL_SECF) {
    RTC->CRL &= ~RTC_CRL_SECF;                  // rc_w0
    GPIOB->ODR ^= GPIO_ODR_ODR1;
    g_tick++;
  }
}

static void rtc_wait_write() { while (!(RTC->CRL & RTC_CRL_RTOFF)) {} }

static bool lse_rtc_start() {
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  (void)RCC->APB1ENR;
  PWR->CR |= PWR_CR_DBP;                        // izinkan tulis domain backup
  RCC->BDCR |= RCC_BDCR_BDRST;                  // uji: LSE start dari nol tiap reset
  RCC->BDCR &= ~RCC_BDCR_BDRST;
#ifdef S0A_COLD_WAIT_MS
  // Uji START DINGIN tanpa power-cycle (17 Nov): LSE dimatikan BDRST di atas, lalu ditunggu sampai
  // getaran kristal habis (Q tinggi -> peluruhan orde 1 s) sebelum LSEON. Alasan: mencabut USB-C tidak
  // mematikan host (daya balik lewat kabel sinyal logger/attacker, LED PC13 tetap redup), dan reset
  // biasa menyalakan LSE lagi selagi kristal masih bergetar (start ~300 ms, bukan start dingin).
  delay(S0A_COLD_WAIT_MS);
#endif
  const uint32_t c0 = cycles();
  const uint32_t lim = (SystemCoreClock / 1000UL) * LSE_TIMEOUT_MS;   // 288 M siklus < 2^32
  RCC->BDCR |= RCC_BDCR_LSEON;
  while (!(RCC->BDCR & RCC_BDCR_LSERDY)) {
    if ((uint32_t)(cycles() - c0) > lim) { g_bdcr = RCC->BDCR; return false; }
  }
  g_lse_start_ms = (uint32_t)(cycles() - c0) / (SystemCoreClock / 1000UL);

  RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_0;    // RTCSEL = 01: LSE
  RCC->BDCR |= RCC_BDCR_RTCEN;
  g_bdcr = RCC->BDCR;

  RTC->CRL &= ~RTC_CRL_RSF;                     // sinkron register RTC ke APB1
  while (!(RTC->CRL & RTC_CRL_RSF)) {}
  rtc_wait_write();
  RTC->CRL |= RTC_CRL_CNF;
  RTC->PRLH = (RTC_PRL >> 16) & 0xF;
  RTC->PRLL = RTC_PRL & 0xFFFF;
  RTC->CNTH = 0; RTC->CNTL = 0;
  RTC->CRL &= ~RTC_CRL_CNF;
  rtc_wait_write();

  BKP->RTCCR = BKP_RTCCR_CCO;                   // RTCCLK/64 = 512 Hz di PC13

  RTC->CRL &= ~RTC_CRL_SECF;
  rtc_wait_write();
  RTC->CRH = RTC_CRH_SECIE;
  rtc_wait_write();
  NVIC_ClearPendingIRQ(RTC_IRQn);
  NVIC_SetPriority(RTC_IRQn, 0);
  NVIC_EnableIRQ(RTC_IRQn);
  return true;
}

static uint32_t rtc_cnt() {
  uint32_t h, l, h2;
  do { h = RTC->CNTH; l = RTC->CNTL; h2 = RTC->CNTH; } while (h != h2);
  return (h << 16) | l;
}

// --- USART1 ke logger (register langsung, sama dengan host_link.h) ---
static void link_init() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  GPIOA->CRH = (GPIOA->CRH & ~((0xFUL << 4) | (0xFUL << 8))) | (0xAUL << 4) | (0x8UL << 8);
  GPIOA->BSRR = GPIO_BSRR_BS10;
  USART1->CR1 = 0; USART1->CR2 = 0; USART1->CR3 = 0;
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + LINK_BAUD / 2) / LINK_BAUD;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void link_tx(const char *s) {
  while (*s) { while (!(USART1->SR & USART_SR_TXE)) {} USART1->DR = (uint8_t)*s++; }
}

static void link_poll() {                       // jawab '?' (identitas)
  const uint32_t sr = USART1->SR;
  if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) { (void)USART1->DR; return; }
  if (!(sr & USART_SR_RXNE)) return;
  if ((char)USART1->DR == LINK_CMD_ID[0]) {
    char b[64];
    snprintf(b, sizeof b, LINK_R_ID ",S0A,0,%lu,%lu\n", (unsigned long)SystemCoreClock, (unsigned long)g_lse_ok);
    link_tx(b);
  }
}

void setup() {
  ser.begin(115200);
  cyccnt_start();
  pinMode(PB1, OUTPUT);
  GPIOB->BSRR = GPIO_BSRR_BR1;
  link_init();
  g_lse_ok = lse_rtc_start() ? 1 : 0;
}

void loop() {
  link_poll();
  static uint32_t last = 0;
  const uint32_t t = g_tick;
  const bool due = g_lse_ok ? (t - last >= 8) : (millis() - last >= 1000);
  if (!due) return;
  last = g_lse_ok ? t : millis();
  char b[96];
  snprintf(b, sizeof b, "S0A,%lu,%lu,%lu,%lu,%08lX\n", (unsigned long)t, (unsigned long)g_lse_ok,
           (unsigned long)g_lse_start_ms, (unsigned long)(g_lse_ok ? rtc_cnt() : 0), (unsigned long)g_bdcr);
  link_tx(b);
  if (ser) ser.print(b);
}
