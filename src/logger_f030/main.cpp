// ============================================================================
// LOGGER (pencatat) - STM32F030C8T6 (Cortex-M0), tahap L1 + L2 + L3 (build from base)
//
// Sejak 14 Nov 2026 logger SEMENTARA memakai board F030C8 (STM32F103C8T6 dipesan). Paper v3 Tabel 3.3
// menyebut Blue Pill F103; versi F103 ada di src/logger/ (env `logger`, belum pernah dijalankan).
// Bagian yang BEBAS chip (driver INA226, logika cuplik L3) ada di src/logger_common/ dan dipakai
// ulang di F103. File ini hanya berisi bagian yang bergantung F030: jam, pencacah siklus, jalur
// keluar (USART2 -> CH340), peta pin.
//
// Peran logger (Bab 3.4): membaca INA226 lewat I2C, menangkap tepi pin status host dengan
// input-capture, berkomunikasi dengan host lewat USART1, dan mengirim CSV ke PC. Logger =
// PEMBANDING INDEPENDEN untuk waktu, jadi jamnya harus dari kristal, bukan HSI host/attacker.
//
// Perbedaan F030C8 terhadap F103 (alasan desain berubah):
//   - SYSCLK maks 48 MHz (HSE 8 MHz x6), bukan 72 MHz.
//   - Tidak ada USB: konsol/CSV ke PC lewat USART2 PA2(TX)/PA3(RX) -> CH340, 115200 8N1.
//   - Tidak ada DWT CYCCNT (Cortex-M0): pencacah siklus dari SysTick (plat_cycles()).
//   - Tidak ada TIM2: input-capture nanti (L4) memakai TIM3/TIM1 yang 16-bit, perlu overflow.
//
// L1: jam kristal. SystemClock_Config sendiri; kristal DIUKUR dulu (TIM14 menangkap HSE/32 lewat
//     TIM14_OR terhadap HSI nominal 8 MHz), PLL x6 hanya bila 7-9 MHz, selain itu fallback HSI
//     48 MHz dan ditandai g_clk_hse = 0 (g_clk_why = sebab). Perintah I: info jam, UID, penghitung.
// L2: INA226 di I2C1 (SCL PB6, SDA PB7, 400 kHz), alamat 0x44 (terukur 14 Nov: A1 ke VS), shunt
//     0,100 Ohm (R100). cfg 0x4127, cal 0x0800 (Current_LSB 25 uA = resolusi shunt 2,5 uV / 0,1 Ohm).
//     Perintah N: init ulang + satu pembacaan; S: pindai alamat I2C.
// L3: cuplik INA226 kontinu + cap waktu (src/logger_common/l3_capture.*). Perintah C: rekam 5 detik,
//     cetak statistik (laju, selang, cuplikan terlambat/terlewat, shunt, bus). Belum ada CSV/streaming.
// TIDAK ada input-capture, UART1 ke host, CSV (tahap berikutnya).
//
// setup() tidak menunggu apa pun. Verifikasi tanpa serial: baca variabel g_* lewat OpenOCD/SWD.
//
// Build: pio run -e logger_f030    Upload: pio run -e logger_f030 -t upload   (ST-Link serial X)
// ============================================================================
#include <Arduino.h>
#include <Wire.h>
#include "../logger_common/ina226.h"
#include "../logger_common/l3_capture.h"
#include "../logger_common/ina_report.h"

#define ser Serial2                                // USART2 PA2/PA3 -> CH340 (eksplisit)

// Dibaca lewat SWD untuk verifikasi (volatile + non-static agar simbolnya muncul di nm).
volatile uint32_t g_beat        = 0;               // naik tiap 500 ms
volatile uint32_t g_sysclk_hz   = 0;               // SYSCLK hasil konfigurasi
volatile uint32_t g_pclk1_hz    = 0;
volatile uint32_t g_hse_meas_hz = 0;               // hasil ukur kristal (0 = HSE tidak start/gagal ukur)
volatile uint8_t  g_clk_hse     = 0;               // 1 = SYSCLK dari HSE+PLL, 0 = fallback HSI
volatile uint8_t  g_clk_why     = 0;               // 0 ok, 1 HSE tak start, 2 ukur gagal/di luar 7-9 MHz, 3 PLL gagal
volatile uint32_t g_build_id    = 0x4C463033;      // 'LF03' = logger F030 tahap L3

// --- INA226 ---
static const uint8_t  INA_ADDR   = 0x44;           // hasil pindai 14 Nov: A1 ke VS, A0 ke GND (paper v3 menulis 0x40)
static const uint16_t INA_CAL_V  = 0x0800;         // Current_LSB 25 uA untuk shunt 0,100 Ohm
static const uint32_t INA_UA_PER_LSB = 25;         // = 2,5 uV / 0,100 Ohm
static INA226 ina(Wire, INA_ADDR);
volatile uint8_t  g_ina_ok    = 0;                 // 1 = ID benar + cfg/cal terbaca balik sama
volatile uint8_t  g_i2c_err   = 0;                 // kode terakhir Wire (0x80 = requestFrom pendek)
volatile uint16_t g_ina_mfg   = 0;
volatile uint16_t g_ina_die   = 0;
volatile uint16_t g_ina_cfg   = 0;
volatile uint16_t g_ina_cal   = 0;
// --- L3 (hasil rekam terakhir) ---
volatile uint32_t g_cyc_back     = 0;              // hasil uji monotonik cycles() (harus 0)
volatile uint32_t g_l3_n         = 0;
volatile uint32_t g_l3_late      = 0;
volatile uint32_t g_l3_i2c_err   = 0;
volatile uint32_t g_l3_rate_x100 = 0;              // cuplikan/s x 100

// ---------------------------------------------------------------------------
// Ukur frekuensi HSE. SYSCLK saat ini HSI 8 MHz (kondisi reset), TIM14 berjalan dari HSI itu.
// TIM14_OR.TI1_RMP = 10 -> TI1 = HSE/32. Capture tiap 8 tepi (IC1PSC=11) -> 256 siklus HSE.
// Hasil nominal terhadap HSI 8 MHz (HSI sendiri +-1..2%), cukup untuk membedakan 8 vs 16 MHz.
// ---------------------------------------------------------------------------
static uint32_t measure_hse_hz() {
  RCC_OscInitTypeDef o = {};
  o.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  o.HSEState       = RCC_HSE_ON;
  o.PLL.PLLState   = RCC_PLL_NONE;                 // SYSCLK tetap HSI
  if (HAL_RCC_OscConfig(&o) != HAL_OK) { g_clk_why = 1; return 0; }

  __HAL_RCC_TIM14_CLK_ENABLE();
  TIM14->CR1   = 0;
  TIM14->PSC   = 0;
  TIM14->ARR   = 0xFFFF;
  TIM14->OR    = 2;                                // TI1_RMP: HSE/32
  TIM14->CCMR1 = (1u << 0) | (3u << 2);            // CC1S=01 (TI1), IC1PSC=11 (tiap 8 tepi)
  TIM14->CCER  = TIM_CCER_CC1E;                    // tepi naik
  TIM14->SR    = 0;
  TIM14->CR1   = TIM_CR1_CEN;

  const uint32_t N = 16;                           // jumlah selang capture yang dijumlah
  uint32_t guard = 2000000;
  uint32_t n = 0, ticks = 0;
  uint16_t prev = 0;
  bool ok = true;
  while (guard-- > 0) {
    uint32_t sr = TIM14->SR;
    if (sr & TIM_SR_CC1IF) {
      uint16_t cap = (uint16_t)TIM14->CCR1;        // baca CCR1 menghapus CC1IF
      if (sr & TIM_SR_CC1OF) { TIM14->SR = 0; n = 0; ticks = 0; prev = cap; continue; }  // capture terlewat, ulang
      if (n > 0) ticks += (uint16_t)(cap - prev);  // aman wrap 16-bit
      prev = cap;
      if (++n > N) break;
    }
  }
  if (n <= N || ticks == 0) ok = false;
  TIM14->CR1 = 0;
  TIM14->OR  = 0;
  __HAL_RCC_TIM14_CLK_DISABLE();
  if (!ok) { g_clk_why = 2; return 0; }

  // f_hse = 256 siklus HSE * N selang * f_tim / ticks   (f_tim = HSI nominal 8 MHz)
  const uint64_t f = (uint64_t)256u * N * 8000000ull / ticks;
  return (uint32_t)f;
}

static bool clock_try_hse() {
  g_hse_meas_hz = measure_hse_hz();
  if (g_hse_meas_hz < 7000000u || g_hse_meas_hz > 9000000u) {
    if (g_clk_why == 0) g_clk_why = 2;
    return false;
  }
  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState       = RCC_HSE_ON;
  osc.PLL.PLLState   = RCC_PLL_ON;
  osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  osc.PLL.PREDIV     = RCC_PREDIV_DIV1;
  osc.PLL.PLLMUL     = RCC_PLL_MUL6;               // 8 MHz x 6 = 48 MHz
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) { g_clk_why = 3; return false; }

  RCC_ClkInitTypeDef clk = {};
  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) { g_clk_why = 3; return false; }
  return true;
}

static void clock_fallback_hsi() {                 // identik dengan core: HSI/2 x12 = 48 MHz
  RCC_OscInitTypeDef osc = {};
  osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState            = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState        = RCC_PLL_ON;
  osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  osc.PLL.PLLMUL          = RCC_PLL_MUL12;
  osc.PLL.PREDIV          = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) return;

  RCC_ClkInitTypeDef clk = {};
  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1);
}

// Menimpa SystemClock_Config yang WEAK di core (dipanggil dari init()).
extern "C" void SystemClock_Config(void) {
  if (clock_try_hse()) {
    g_clk_hse = 1;
  } else {
    g_clk_hse = 0;                                 // g_clk_why sudah menjelaskan sebabnya
    clock_fallback_hsi();
  }
}

// ---------------------------------------------------------------------------
// Pencacah siklus 32-bit dari SysTick (M0 tidak punya DWT). Wrap tiap ~89 detik pada 48 MHz;
// pakai selisih tak bertanda untuk interval pendek. Di F103 diganti DWT->CYCCNT.
// ---------------------------------------------------------------------------
// Baca ulang sampai tik ms sebelum dan sesudah membaca VAL sama (tanpa COUNTFLAG, yang bisa
// dihapus pembaca lain seperti micros() core). Hanya aman dipanggil dari thread mode / ISR
// berprioritas lebih rendah dari SysTick. Uji: cmd_info mencetak cyc_back (harus 0).
static inline uint32_t cycles() {
  uint32_t ms1, ms2, val, load;
  do {
    ms1  = HAL_GetTick();
    val  = SysTick->VAL;
    load = SysTick->LOAD;
    ms2  = HAL_GetTick();
  } while (ms1 != ms2);
  return ms1 * (load + 1) + (load - val);
}

// Uji monotonik: berapa kali cycles() mundur dalam n panggilan beruntun.
static uint32_t cycles_selftest(uint32_t n) {
  uint32_t back = 0, prev = cycles();
  for (uint32_t i = 0; i < n; i++) {
    uint32_t c = cycles();
    if ((int32_t)(c - prev) < 0) back++;
    prev = c;
  }
  return back;
}

// Antarmuka platform untuk src/logger_common/l3_capture.*
uint32_t plat_cycles() { return cycles(); }
uint32_t plat_hz()     { return SystemCoreClock; }

// ---------------------------------------------------------------------------
static void ina_publish() {                        // salin keadaan driver ke g_* (untuk SWD)
  g_ina_ok  = ina.ok() ? 1 : 0;
  g_i2c_err = ina.lastErr;
  g_ina_mfg = ina.mfg;
  g_ina_die = ina.die;
  g_ina_cfg = ina.cfgRb;
  g_ina_cal = ina.calRb;
}

static bool ina_start() {
  bool ok = ina.init(INA226::CFG_CONT_1P1MS, INA_CAL_V);
  ina_publish();
  return ok;
}

static void cmd_ina() {
  ina_report_single(ina, INA_CAL_V, INA_UA_PER_LSB, ser);
  ina_publish();
}

static void cmd_scan() { i2c_scan(Wire, ser); }

static void cmd_capture() {
  if (!ina_start()) { ser.print(F("INA226 init gagal, rekam dibatalkan\r\n")); return; }
  ser.print(F("rekam 5 detik...\r\n"));
  static L3Stats st;                               // static: tidak membebani stack M0
  if (!l3_run(ina, 5000, st)) { ser.print(F("parameter rekam tak sah\r\n")); return; }
  g_l3_n       = st.n;
  g_l3_late    = st.late;
  g_l3_i2c_err = st.i2c_err;
  g_l3_rate_x100 = (st.n > 1 && st.dt_sum > 0)
                     ? (uint32_t)((uint64_t)(st.n - 1) * plat_hz() * 100u / st.dt_sum) : 0;
  ina_publish();
  l3_print(st, ser, INA_UA_PER_LSB);
}

static void cmd_info() {
  const uint32_t* uid = (const uint32_t*)0x1FFFF7AC;   // UID 96-bit F0
  const uint16_t flash_kb = *(const uint16_t*)0x1FFFF7CC;
  ser.printf("LOGGER F030 tahap L3 (build %08lX)\r\n", (unsigned long)g_build_id);
  ser.printf("jam=%s why=%u HSEukur=%lu Hz SYSCLK=%lu PCLK1=%lu Hz\r\n",
             g_clk_hse ? "HSE+PLL(x6)" : "HSI-fallback", (unsigned)g_clk_why,
             (unsigned long)g_hse_meas_hz, (unsigned long)g_sysclk_hz, (unsigned long)g_pclk1_hz);
  ser.printf("UID=%08lX%08lX%08lX flash=%uKB\r\n",
             (unsigned long)uid[2], (unsigned long)uid[1], (unsigned long)uid[0], (unsigned)flash_kb);
  ser.printf("cyc=%lu t_ms=%lu beat=%lu\r\n",
             (unsigned long)cycles(), (unsigned long)millis(), (unsigned long)g_beat);
  g_cyc_back = cycles_selftest(300000);            // ~0,3 s; harus 0
  ser.printf("uji monotonik cycles(): mundur=%lu dari 300000 panggilan\r\n", (unsigned long)g_cyc_back);
}

void setup() {
  g_sysclk_hz = HAL_RCC_GetSysClockFreq();
  g_pclk1_hz  = HAL_RCC_GetPCLK1Freq();
#ifdef LOGGER_LED_PIN
  pinMode(LOGGER_LED_PIN, OUTPUT);
#endif
  ser.begin(115200);
  Wire.setSDA(PB7);
  Wire.setSCL(PB6);
  Wire.begin();
  Wire.setClock(400000);
  ina_start();                                     // hasil di g_ina_* (bisa dibaca lewat SWD tanpa serial)
  ser.print(F("\r\n=== Logger F030 L3 siap. I = info jam, N = INA226, S = pindai I2C, C = rekam 5 s ===\r\n"));
}

void loop() {
  static uint32_t t_beat = 0;
  if ((uint32_t)(millis() - t_beat) >= 500) {
    t_beat += 500;
    g_beat++;
#ifdef LOGGER_LED_PIN
    digitalWrite(LOGGER_LED_PIN, g_beat & 1);
#endif
  }
  if (ser.available() > 0) {
    char c = (char)ser.read();
    if (c == 'I' || c == 'i') cmd_info();
    else if (c == 'N' || c == 'n') cmd_ina();
    else if (c == 'S' || c == 's') cmd_scan();
    else if (c == 'C' || c == 'c') cmd_capture();
  }
}
