// ============================================================================
// HOST deep sleep, LANGKAH 5 = S0: Stop mode minimal -> LANTAI arus papan host.
// Rencana: docs/referensi/rencana_deep_sleep.md. Tanpa bangun (tidak ada sumber EXTI/RTC): Stop selamanya.
//
// Fungsi tahap ini satu: mengukur arus papan saat MCU di Stop dengan semua yang bisa dimatikan mati.
//   1. Jendela aman 5 s setelah boot (PB1 = 1, CPU sibuk): SWD masih bisa tersambung untuk flash ulang.
//      Selama Stop SWD TIDAK tersambung; pemulihan: tekan RESET host lalu flash dalam 5 s.
//   2. Domain backup di-reset (mematikan LSE/RTC/keluaran PC13 sisa firmware S0a).
//   3. DBGMCU_CR = 0 (OpenOCD menyalakan DBG_STOP yang menahan regulator + jam di Stop).
//   4. Semua GPIO A/B/C = input analog (CRL/CRH = 0), KECUALI PA13/PA14 (SWD) supaya flash ulang tetap
//      bisa. JTAG dilepas (SWJ_CFG = 010) agar PA15/PB3/PB4 boleh dianalogkan. PB1 analog -> ditarik
//      rendah oleh pull-down PA0 logger (= 0, "tidur").
//   5. Semua jam periferal mati, SysTick mati, NVIC/EXTI bersih, lalu Stop (PDDS = 0, LPDS = 1).
//   Tepi turun PB1 di logger = saat masuk Stop (penanda untuk memotong data INA226).
//
// Build TANPA USB CDC (inisialisasi USB tidak terjadi sama sekali).
// Build: pio run -e host_s0_stop    Upload: pio run -e host_s0_stop -t upload   (ST-Link '$')
// Ukur:  python tools/dev/tahap_stop/s0_stop.py --logger COM18 60
// ============================================================================
#include <Arduino.h>

#define SAFE_MS 5000UL

volatile uint32_t g_stage = 0;                  // 1 = jendela aman, 2 = siap Stop, 3 = bangun (tak terduga)
volatile uint32_t g_wake = 0;                   // jumlah keluar dari WFI (harus 0)

static void all_pins_analog() {
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  (void)RCC->APB2ENR;
  AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;   // SWD saja
  // PA13/PA14 (CRH nibble 5 dan 6) dipertahankan; sisanya 0 = input analog.
  GPIOA->CRL = 0;
  GPIOA->CRH = GPIOA->CRH & ((0xFUL << 20) | (0xFUL << 24));
  GPIOB->CRL = 0; GPIOB->CRH = 0;
  GPIOC->CRL = 0; GPIOC->CRH = 0;
  GPIOA->ODR = 0; GPIOB->ODR = 0; GPIOC->ODR = 0;
}

static void stop_forever() {
  __disable_irq();
  SysTick->CTRL = 0;
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
  for (int i = 0; i < 8; i++) { NVIC->ICER[i] = 0xFFFFFFFFUL; NVIC->ICPR[i] = 0xFFFFFFFFUL; }
  EXTI->IMR = 0; EXTI->EMR = 0; EXTI->PR = 0x000FFFFFUL;

  RCC->APB1ENR = RCC_APB1ENR_PWREN;             // hanya PWR (perlu untuk menulis PWR_CR)
  (void)RCC->APB1ENR;
  PWR->CR = (PWR->CR & ~PWR_CR_PDDS) | PWR_CR_LPDS | PWR_CR_CWUF;
  RCC->APB2ENR = 0;                             // GPIO/AFIO: konfigurasi pin tetap tersimpan
  RCC->APB1ENR = 0;
  RCC->AHBENR = RCC_AHBENR_SRAMEN | RCC_AHBENR_FLITFEN;   // bawaan reset

  g_stage = 2;
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  for (;;) {
    __DSB();
    __WFI();
    g_wake++;                                   // tak ada sumber bangun: seharusnya tak pernah ke sini
    g_stage = 3;
  }
}

void setup() {
  g_stage = 1;
  pinMode(PB1, OUTPUT);
  GPIOB->BSRR = GPIO_BSRR_BS1;                  // PB1 = 1 selama jendela aman (logger: tepi naik)

  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  (void)RCC->APB1ENR;
  PWR->CR |= PWR_CR_DBP;
  RCC->BDCR |= RCC_BDCR_BDRST;                  // LSE, RTC, CCO PC13 mati
  RCC->BDCR &= ~RCC_BDCR_BDRST;
  PWR->CR &= ~PWR_CR_DBP;

  const uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < SAFE_MS) {}   // CPU sibuk: arus "aktif" pembanding

  DBGMCU->CR = 0;
  GPIOB->BSRR = GPIO_BSRR_BR1;                  // PB1 = 0 (tepi turun = masuk Stop)
  all_pins_analog();
  stop_forever();
}

void loop() {}
