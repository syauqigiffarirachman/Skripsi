// ============================================================================
// Host (K1 dan K2): penerima permintaan NYATA dari attacker lewat USART2 (PA3 = RX).
//
// Penerimaan: DMA1 kanal 6 mengisi satu frame 36 B; interupsi selesai-transfer (TC) terjadi
// SEKALI per frame, jadi satu frame = satu bangun CPU (paper). Deteksi idle-line lewat interupsi
// USART2 tak dipakai karena core Arduino sudah mendefinisikan USART2_IRQHandler (tabrakan
// simbol); gantinya: bila frame tak valid (sinkron geser, byte hilang), ISR membuang byte sampai
// flag IDLE USART terangkat (atau 10 ms) lalu memasang ulang DMA, sehingga perataan frame pulih
// pada jeda antar-frame (attacker >= 100 ms antar frame).
//
// Frame (src/attacker/attacker.cpp): [0]=0xA5 [1..2]=seq uint16 LE [3..34]=isi [35]=XOR [0..34].
// Isi frame TIDAK dipakai: yang ditandatangani tetap DIGEST konstan (variabel kontrol K1/K2).
// Hilang (g_rx_lost) = selisih nomor urut, yaitu frame yang tak pernah sampai.
//
// Pemakaian: rx_init() sekali di setup(); rx_start() / rx_stop() di awal / akhir jendela.
// Frame sah menaikkan g_arrived dan menyalakan g_pending, jadi main.cpp harus mendefinisikan
// kedua variabel itu dan meng-include host_time.h (hw_us) sebelum berkas ini.
// ============================================================================
#pragma once
#include "host_time.h"

#define RX_FRAME_LEN 36
#define RX_SOF       0xA5
#define RX_BAUD      115200UL

static uint8_t g_rx[RX_FRAME_LEN];
static volatile uint32_t g_rx_ok, g_rx_bad, g_rx_lost, g_rx_first_seq, g_rx_last_seq;
static volatile bool     g_rx_have;
static uint8_t g_rx_badfirst[RX_FRAME_LEN];   // salinan frame bad pertama (diagnosis)
static volatile uint32_t g_rx_badfirst_wait;      // lama menunggu IDLE di ISR (us) pada bad pertama

static void rx_arm() {
  DMA1_Channel6->CCR &= ~DMA_CCR_EN;
  DMA1_Channel6->CNDTR = RX_FRAME_LEN;
  DMA1_Channel6->CCR |= DMA_CCR_EN;
}

static void rx_init() {                         // sekali di setup; penerimaan baru aktif di rx_start()
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  GPIOA->CRL = (GPIOA->CRL & ~(0xFUL << 12)) | (0x4UL << 12);   // PA3: input floating
  USART2->CR1 = 0; USART2->CR2 = 0;
  USART2->BRR = (HAL_RCC_GetPCLK1Freq() + RX_BAUD / 2) / RX_BAUD;
  USART2->CR3 = USART_CR3_DMAR;
  USART2->CR1 = USART_CR1_UE;                   // RE dinyalakan di rx_start()
  DMA1_Channel6->CCR = 0;
  DMA1_Channel6->CPAR = (uint32_t)&USART2->DR;
  DMA1_Channel6->CMAR = (uint32_t)g_rx;
  DMA1_Channel6->CCR = DMA_CCR_MINC | DMA_CCR_TCIE;
  NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
  NVIC_EnableIRQ(DMA1_Channel6_IRQn);
}

static void rx_start() {
  g_rx_ok = g_rx_bad = g_rx_lost = g_rx_first_seq = g_rx_last_seq = 0;
  g_rx_have = false;
  g_rx_badfirst_wait = 0;
  memset(g_rx_badfirst, 0, RX_FRAME_LEN);
  (void)USART2->SR; (void)USART2->DR;           // buang sisa
  DMA1->IFCR = DMA_IFCR_CGIF6;
  NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
  USART2->CR1 |= USART_CR1_RE;
  rx_arm();
#ifdef K1_RX_TEST_MISALIGN
  // HANYA UJI: frame pertama diharapkan 35 B (bukan 36) -> sengaja salah rata, untuk membuktikan
  // pemulihan (bad=1 lalu semua frame berikutnya ok). Aktifkan: PLATFORMIO_BUILD_FLAGS=-DK1_RX_TEST_MISALIGN
  DMA1_Channel6->CCR &= ~DMA_CCR_EN;
  DMA1_Channel6->CNDTR = RX_FRAME_LEN - 1;
  DMA1_Channel6->CCR |= DMA_CCR_EN;
#endif
}

static void rx_stop() {
  DMA1_Channel6->CCR &= ~DMA_CCR_EN;
  USART2->CR1 &= ~USART_CR1_RE;
}

extern "C" void DMA1_Channel6_IRQHandler(void) {
  DMA1->IFCR = DMA_IFCR_CGIF6;
  uint8_t x = 0;
  for (int i = 0; i < RX_FRAME_LEN - 1; i++) x ^= g_rx[i];
  if (g_rx[0] == RX_SOF && x == g_rx[RX_FRAME_LEN - 1]) {
    const uint16_t seq = (uint16_t)g_rx[1] | ((uint16_t)g_rx[2] << 8);
    if (g_rx_have) g_rx_lost += (uint16_t)(seq - (uint16_t)(g_rx_last_seq + 1));
    else { g_rx_first_seq = seq; g_rx_have = true; }
    g_rx_last_seq = seq;
    g_rx_ok++;
    g_arrived++;
    g_pending = true;
  } else {
    if (g_rx_bad++ == 0) memcpy(g_rx_badfirst, g_rx, RX_FRAME_LEN);
    // PENTING: matikan permintaan DMA USART selama resync. Bila tidak, byte sisa yang tiba saat
    // kanal nonaktif membuat permintaan DMA 'terkunci'; saat kanal dipasang ulang DMA membaca DR
    // basi (byte checksum) sebagai byte pertama frame berikutnya dan semua frame jadi bad (terbukti
    // di uji K1_RX_TEST_MISALIGN, 14 Nov). DMAR dinyalakan lagi SESUDAH kanal dipasang ulang.
    USART2->CR3 &= ~USART_CR3_DMAR;
    (void)USART2->SR; (void)USART2->DR;         // hapus IDLE basi
    const uint32_t t = hw_us();
    while (!(USART2->SR & USART_SR_IDLE) && (uint32_t)(hw_us() - t) < 10000UL) {
      if (USART2->SR & USART_SR_RXNE) (void)USART2->DR;
    }
    if (g_rx_bad == 1) g_rx_badfirst_wait = (uint32_t)(hw_us() - t);
    (void)USART2->SR; (void)USART2->DR;
    rx_arm();
    USART2->CR3 |= USART_CR3_DMAR;
    return;
  }
  rx_arm();
}
