// ============================================================================
// LOGGER (pencatat) - TAHAP L1 (build from base) - STM32F103C8T6 (Blue Pill)
//
// Peran logger di paper v3 (Bab 3.4): membaca INA226 lewat I2C1, menangkap tepi pin status
// host dengan input-capture TIM2_CH1 (PA0), berkomunikasi dengan host lewat USART1 (PA9/PA10),
// dan mengirim CSV ke PC lewat USB CDC. Logger = PEMBANDING INDEPENDEN untuk waktu, jadi
// jamnya harus lebih teliti daripada host.
//
// Fungsi tahap ini SATU: membuktikan logger hidup dengan jam yang benar.
//   - SystemClock_Config sendiri: kristal HSE 8 MHz -> PLL x9 = 72 MHz (USB 48 MHz = PLL/1,5).
//     Core STM32duino bawaan memakai HSI (+-1-2%), tidak cukup sebagai pembanding waktu.
//     Bila HSE gagal start: fallback HSI/2 x12 = 48 MHz dan ditandai g_clk_hse = 0.
//   - DWT CYCCNT aktif (dipakai tahap berikutnya untuk profil ISR).
//   - LED PC13 berkedip 1 Hz (aktif rendah), penghitung g_beat naik tiap 500 ms.
//   - USB CDC; perintah I mencetak info jam, UID, dan penghitung.
// TIDAK ada INA226, input-capture, UART1, CSV (tahap berikutnya).
//
// setup() TIDAK menunggu koneksi USB (pelajaran 13 Nov: blokir di setup merusak pewaktuan).
// Verifikasi tanpa USB: baca variabel g_* lewat OpenOCD/SWD (alamat dari arm-none-eabi-nm).
//
// Build: pio run -e logger    Upload: pio run -e logger -t upload   (ST-Link logger, serial X)
// ============================================================================
#include <Arduino.h>

#define ser SerialUSB
static const uint8_t PIN_LED = PC13;            // LED Blue Pill, aktif rendah

// Dibaca lewat SWD untuk verifikasi (volatile + non-static agar simbolnya muncul di nm).
volatile uint32_t g_beat      = 0;              // naik tiap 500 ms
volatile uint32_t g_sysclk_hz = 0;              // SystemCoreClock hasil konfigurasi
volatile uint32_t g_pclk1_hz  = 0;
volatile uint32_t g_pclk2_hz  = 0;
volatile uint8_t  g_clk_hse   = 0;              // 1 = SYSCLK dari HSE+PLL, 0 = fallback HSI
volatile uint32_t g_build_id  = 0x4C4F4731;     // 'LOG1' = tahap L1

// ---------------------------------------------------------------------------
// Jam sistem. Menimpa SystemClock_Config yang WEAK di core (dipanggil dari init()).
// ---------------------------------------------------------------------------
static bool clock_try_hse() {
  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState       = RCC_HSE_ON;
  osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  osc.PLL.PLLState   = RCC_PLL_ON;
  osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMUL     = RCC_PLL_MUL9;              // 8 MHz x 9 = 72 MHz
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) return false;

  RCC_ClkInitTypeDef clk = {};
  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV2;             // 36 MHz (batas maksimum APB1)
  clk.APB2CLKDivider = RCC_HCLK_DIV1;             // 72 MHz
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) return false;

  RCC_PeriphCLKInitTypeDef pclk = {};
  pclk.PeriphClockSelection = RCC_PERIPHCLK_USB;
  pclk.UsbClockSelection    = RCC_USBCLKSOURCE_PLL_DIV1_5;   // 72 / 1,5 = 48 MHz
  return HAL_RCCEx_PeriphCLKConfig(&pclk) == HAL_OK;
}

static void clock_fallback_hsi() {                // identik dengan core (varian USB): 48 MHz
  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState            = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState        = RCC_PLL_ON;
  osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
  osc.PLL.PLLMUL          = RCC_PLL_MUL12;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) return;

  RCC_ClkInitTypeDef clk = {};
  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV2;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) return;

  RCC_PeriphCLKInitTypeDef pclk = {};
  pclk.PeriphClockSelection = RCC_PERIPHCLK_USB;
  pclk.UsbClockSelection    = RCC_USBCLKSOURCE_PLL;          // PLL 48 MHz langsung
  HAL_RCCEx_PeriphCLKConfig(&pclk);
}

extern "C" void SystemClock_Config(void) {
  if (clock_try_hse()) {
    g_clk_hse = 1;
  } else {
    // HSE tidak start (kristal tak ada/rusak) atau PLL gagal: SYSCLK masih HSI, konfigurasi ulang.
    g_clk_hse = 0;
    clock_fallback_hsi();
  }
}

// ---------------------------------------------------------------------------
static inline void dwt_enable() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

static void cmd_info() {
  const uint32_t* uid = (const uint32_t*)0x1FFFF7E8;   // UID 96-bit
  ser.printf("LOGGER tahap L1 (build %08lX)\n", (unsigned long)g_build_id);
  ser.printf("jam=%s SYSCLK=%lu PCLK1=%lu PCLK2=%lu Hz  CYCCNT=%lu\n",
             g_clk_hse ? "HSE+PLL(x9)" : "HSI-fallback",
             (unsigned long)g_sysclk_hz, (unsigned long)g_pclk1_hz, (unsigned long)g_pclk2_hz,
             (unsigned long)DWT->CYCCNT);
  ser.printf("UID=%08lX%08lX%08lX beat=%lu t_ms=%lu\n",
             (unsigned long)uid[2], (unsigned long)uid[1], (unsigned long)uid[0],
             (unsigned long)g_beat, (unsigned long)millis());
}

void setup() {
  dwt_enable();
  g_sysclk_hz = HAL_RCC_GetSysClockFreq();
  g_pclk1_hz  = HAL_RCC_GetPCLK1Freq();
  g_pclk2_hz  = HAL_RCC_GetPCLK2Freq();
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);                     // padam
  ser.begin(115200);                               // tidak menunggu koneksi PC
}

void loop() {
  static uint32_t t_beat = 0;
  if ((uint32_t)(millis() - t_beat) >= 500) {
    t_beat += 500;
    g_beat++;
    digitalWrite(PIN_LED, (g_beat & 1) ? LOW : HIGH);
  }
  if (ser.available() > 0) {
    char c = (char)ser.read();
    if (c == 'I') cmd_info();
  }
}
