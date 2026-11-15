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
// L2 + L3 (dipindah dari versi F030 setelah L1 lulus di F103, 14 Nov): INA226 di I2C1 (SCL PB6,
//   SDA PB7, 400 kHz) alamat 0x44 (terukur), shunt 0,100 Ohm; kodenya di src/logger_common/
//   (driver, cuplik L3, laporan) dan dipakai bersama dengan versi F030. Di sini hanya bagian
//   khusus F103: jam, plat_cycles() = DWT->CYCCNT, keluaran USB CDC, peta pin.
//   PERINTAH DARI PC LEWAT USB BELUM BISA (jalur OUT klon ini gagal, lihat memori/logbook), jadi
//   laporan berjalan OTOMATIS: begitu port dibuka (DTR), 2 s kemudian tercetak laporan INA226 + rekam
//   5 s, lalu diulang tiap 15 s. Perintah I/N/S/C tetap ada untuk bila jalur OUT nanti bekerja.
// L4 (14 Nov): input-capture pin status host di PA0 (TIM2_CH1 naik + CH2->TI1 turun, 16-bit + overflow
//   jadi 64-bit), statistik tepi di src/logger_common/edge_stats.*; dilaporkan bersama L3 dalam jendela 5 s.
//   Generator uji PWM 100 Hz/30% di PA6 (jumper ke PA0) HANYA di env logger_selftest (-DL4_SELFTEST_PWM);
//   env logger biasa membiarkan PA6 input. Uji mandiri LULUS 15 Nov (HIGH 2999,944 / LOW 6999,972 us).
// TIDAK ada UART1 ke host, CSV (tahap berikutnya).
//
// setup() TIDAK menunggu koneksi USB (pelajaran 13 Nov: blokir di setup merusak pewaktuan).
// Verifikasi tanpa USB: baca variabel g_* lewat OpenOCD/SWD (alamat dari arm-none-eabi-nm).
//
// Build: pio run -e logger    Upload: pio run -e logger -t upload   (ST-Link logger, serial X)
// ============================================================================
#include <Arduino.h>
#include <Wire.h>
#include "../logger_common/ina226.h"
#include "../logger_common/l3_capture.h"
#include "../logger_common/ina_report.h"
#include "../logger_common/edge_stats.h"
#include <HardwareTimer.h>

#define ser SerialUSB
static const uint8_t PIN_LED = PC13;            // LED Blue Pill, aktif rendah

// Dibaca lewat SWD untuk verifikasi (volatile + non-static agar simbolnya muncul di nm).
volatile uint32_t g_beat      = 0;              // naik tiap 500 ms
volatile uint32_t g_sysclk_hz = 0;              // SystemCoreClock hasil konfigurasi
volatile uint32_t g_pclk1_hz  = 0;
volatile uint32_t g_pclk2_hz  = 0;
volatile uint8_t  g_clk_hse   = 0;              // 1 = SYSCLK dari HSE+PLL, 0 = fallback HSI
volatile uint32_t g_build_id  = 0x4C4F4734;     // 'LOG4' = F103 tahap L1 + L2 + L3 + L4

// --- INA226 (L2/L3) ---
static const uint8_t  INA_ADDR       = 0x44;    // terukur 14 Nov: A1 ke VS, A0 ke GND (paper v3 menulis 0x40)
static const uint16_t INA_CAL_V      = 0x0800;  // Current_LSB 25 uA untuk shunt 0,100 Ohm
static const uint32_t INA_UA_PER_LSB = 25;      // = 2,5 uV / 0,100 Ohm
static INA226 ina(Wire, INA_ADDR);
volatile uint8_t  g_ina_ok    = 0;
volatile uint8_t  g_i2c_err   = 0;
volatile uint16_t g_ina_mfg   = 0;
volatile uint16_t g_ina_die   = 0;
volatile uint16_t g_ina_cfg   = 0;
volatile uint16_t g_ina_cal   = 0;
volatile uint32_t g_l3_n         = 0;
volatile uint32_t g_l3_late      = 0;
volatile uint32_t g_l3_i2c_err   = 0;
volatile uint32_t g_l3_rate_x100 = 0;           // cuplikan/s x 100
volatile uint32_t g_auto_runs    = 0;           // berapa kali laporan otomatis selesai
volatile uint32_t g_t2_ovf       = 0;           // jumlah overflow TIM2 (L4)
volatile uint32_t g_edge_rise    = 0;           // tepi naik PA0 sejak boot
volatile uint32_t g_edge_fall    = 0;

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

// ---------------------------------------------------------------------------
// L4: input-capture pin status host di PA0 = TIM2_CH1. TIM2 di F103 hanya 16-BIT (bukan 32-bit):
// di 72 MHz overflow tiap 0,91 ms, jadi timestamp diperluas jadi 64-bit dengan penghitung overflow
// (g_t2_ovf). CH1 menangkap tepi NAIK, CH2 (dipetakan ke TI1 = PA0, CC2S=10) menangkap tepi TURUN
// dari pin yang sama. Handler TIM2_IRQHandler dimiliki HardwareTimer (core), jadi dipakai lewat
// callback; HAL memanggil callback CC1/CC2 SEBELUM callback update dalam satu ISR, jadi bila UIF
// masih terpasang saat capture dibaca: capture kecil (<0x8000) terjadi SETELAH overflow (hi+1),
// capture besar terjadi SEBELUM overflow. Syarat: latensi ISR TIM2 < ~0,45 ms (prioritas 0).
// Statistik dihitung langsung di ISR (EdgeStats::feed, tanpa pembagian).
// ---------------------------------------------------------------------------
static HardwareTimer* cap = nullptr;
static HardwareTimer* gen = nullptr;                // generator uji PWM (TIM3_CH1 = PA6)
static EdgeStats edge;                              // ditulis di ISR; dibaca thread dengan irq dimatikan
static uint32_t tim_hz = 72000000;                  // frekuensi tik TIM2 (timer APB1 = 2 x PCLK1)

static inline uint64_t ext_ts(uint16_t c) {
  uint32_t hi = g_t2_ovf;
  if ((TIM2->SR & TIM_SR_UIF) && c < 0x8000) hi++;  // overflow sudah terjadi tetapi belum dihitung
  return ((uint64_t)hi << 16) | c;
}
static void cb_rise() { g_edge_rise++; edge.feed(ext_ts((uint16_t)TIM2->CCR1), true); }
static void cb_fall() { g_edge_fall++; edge.feed(ext_ts((uint16_t)TIM2->CCR2), false); }
static void cb_ovf()  { g_t2_ovf++; }

static void l4_start() {
  pinMode(PA0, INPUT_PULLDOWN);                     // tidak tersambung -> terbaca 0
  cap = new HardwareTimer(TIM2);
  const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  tim_hz = ((RCC->CFGR & RCC_CFGR_PPRE1) != 0) ? pclk1 * 2 : pclk1;
  TIM2->CR1   &= ~TIM_CR1_CEN;
  TIM2->PSC    = 0;
  TIM2->ARR    = 0xFFFF;
  TIM2->CCMR1  = (1u << 0) | (2u << 8);             // CC1S=01 (IC1<-TI1), CC2S=10 (IC2<-TI1), tanpa filter
  TIM2->CCER   = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC2P;   // CH1 naik, CH2 turun
  TIM2->CNT    = 0;
  TIM2->EGR    = TIM_EGR_UG;
  cap->attachInterrupt(cb_ovf);
  cap->attachInterrupt(1, cb_rise);
  cap->attachInterrupt(2, cb_fall);
  cap->setInterruptPriority(0, 0);                  // tertinggi: batasi latensi (USB, I2C di bawahnya)
  TIM2->SR     = 0;                                 // buang UIF dari UG
  TIM2->CR1   |= TIM_CR1_CEN;
#ifdef L4_SELFTEST_PWM
  // Generator uji: 100 Hz, duty 30% di PA6 (jumper PA6 -> PA0, env logger_selftest). PB1 host HARUS
  // dilepas dari PA0: 15 Nov dua keluaran saling melawan (PB1 host rendah) -> capture hanya lonjakan.
  gen = new HardwareTimer(TIM3);
  gen->setPWM(1, digitalPinToPinName(PA6), 100, 30);
#else
  // Default: PA6 dibiarkan input (high-Z) supaya aman walau jumper PA6-PA0 lupa dilepas saat PA0
  // disambung ke PB1 host.
  pinMode(PA6, INPUT);
#endif
}

// Antarmuka platform untuk src/logger_common/l3_capture.*  (F103: DWT, 32-bit, wrap ~59,6 s di 72 MHz)
uint32_t plat_cycles() { return DWT->CYCCNT; }
uint32_t plat_hz()     { return SystemCoreClock; }

static void ina_publish() {                         // salin keadaan driver ke g_* (untuk SWD)
  g_ina_ok  = ina.ok() ? 1 : 0;
  g_i2c_err = ina.lastErr;
  g_ina_mfg = ina.mfg;
  g_ina_die = ina.die;
  g_ina_cfg = ina.cfgRb;
  g_ina_cal = ina.calRb;
}

// Init INA226, satu pembacaan mentah, lalu rekam 5 s dan cetak statistik (sama dengan N + C di F030).
static void run_report() {
  static L3Stats st;                                // static: tidak membebani stack
  bool ok = ina_report_single(ina, INA_CAL_V, INA_UA_PER_LSB, ser);
  ina_publish();
  if (!ok) return;
  ser.print(F("rekam 5 detik (L3 INA226 + L4 tepi PA0 bersamaan)...\r\n"));
  noInterrupts(); edge = EdgeStats(); interrupts();   // jendela L4 mulai dari nol
  const int pa0_awal = digitalRead(PA0);
  if (!l3_run(ina, 5000, st)) { ser.print(F("parameter rekam tak sah\r\n")); return; }
  g_l3_n       = st.n;
  g_l3_late    = st.late;
  g_l3_i2c_err = st.i2c_err;
  g_l3_rate_x100 = (st.n > 1 && st.dt_sum > 0)
                     ? (uint32_t)((uint64_t)(st.n - 1) * plat_hz() * 100u / st.dt_sum) : 0;
  ina_publish();
  l3_print(st, ser, INA_UA_PER_LSB);
  static EdgeStats snap;
  noInterrupts(); snap = edge; interrupts();
  ser.printf("PA0 awal=%d akhir=%d  overflow TIM2 total=%lu  generator PA6=%s\r\n", pa0_awal, digitalRead(PA0), (unsigned long)g_t2_ovf,
#ifdef L4_SELFTEST_PWM
             "AKTIF (uji mandiri)"
#else
             "mati"
#endif
             );
  snap.print(ser, tim_hz);
}

static void cmd_info() {
  const uint32_t* uid = (const uint32_t*)0x1FFFF7E8;   // UID 96-bit
  ser.printf("LOGGER F103 tahap L1+L2+L3 (build %08lX)\n", (unsigned long)g_build_id);
  ser.printf("jam=%s SYSCLK=%lu PCLK1=%lu PCLK2=%lu Hz  CYCCNT=%lu\n",
             g_clk_hse ? "HSE+PLL(x9)" : "HSI-fallback",
             (unsigned long)g_sysclk_hz, (unsigned long)g_pclk1_hz, (unsigned long)g_pclk2_hz,
             (unsigned long)DWT->CYCCNT);
  ser.printf("UID=%08lX%08lX%08lX beat=%lu t_ms=%lu\n",
             (unsigned long)uid[2], (unsigned long)uid[1], (unsigned long)uid[0],
             (unsigned long)g_beat, (unsigned long)millis());
}

// Chip logger ini (kemungkinan klon) punya UID NOL. Core hanya mengisi string nomor seri USB bila
// UID0+UID2 != 0, jadi tanpa ini deskriptor seri berisi NUL dan Windows menolak enumerasi (alamat
// diberikan, lalu gagal: DADDR != 0 tetapi tidak ada COM). Isi nomor seri tetap bila UID nol.
extern "C" uint8_t USBD_StringSerial[];             // usbd_desc.c: [0]=panjang(26), [1]=3, lalu 12 UTF-16
static void usb_serial_fix() {
  const uint32_t* uid = (const uint32_t*)0x1FFFF7E8;
  if ((uid[0] + uid[2]) != 0) return;               // UID normal: core sudah mengisi sendiri
  const char* s = "LOGGER000001";                   // 12 karakter = 24 byte UTF-16
  for (uint8_t i = 0; i < 12; i++) {
    USBD_StringSerial[2 + 2 * i] = (uint8_t)s[i];
    USBD_StringSerial[3 + 2 * i] = 0;
  }
}

void setup() {
  usb_serial_fix();                                 // HARUS sebelum ser.begin() (USB mulai di sana)
  dwt_enable();
  g_sysclk_hz = HAL_RCC_GetSysClockFreq();
  g_pclk1_hz  = HAL_RCC_GetPCLK1Freq();
  g_pclk2_hz  = HAL_RCC_GetPCLK2Freq();
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);                     // padam
  ser.begin(115200);                               // tidak menunggu koneksi PC
  Wire.setSDA(PB7);
  Wire.setSCL(PB6);
  Wire.begin();
  Wire.setClock(400000);
  ina.init(INA226::CFG_CONT_1P1MS, INA_CAL_V);     // hasil di g_ina_* (SWD tanpa USB)
  ina_publish();
  l4_start();
}

void loop() {
  static uint32_t t_beat = 0;
  if ((uint32_t)(millis() - t_beat) >= 500) {
    t_beat += 500;
    g_beat++;
    digitalWrite(PIN_LED, (g_beat & 1) ? LOW : HIGH);
    // Detak 1 Hz ke USB (uji jalur IN logger -> PC tanpa perlu menulis dari PC). Hanya bila port
    // dibuka (DTR), supaya tidak memblokir loop saat tidak ada pembaca.
    if (!(g_beat & 1) && ser) ser.printf("LOG1 beat=%lu t_ms=%lu\r\n", (unsigned long)g_beat, (unsigned long)millis());
  }
  // Laporan otomatis (pengganti perintah PC): 2 s setelah port dibuka, lalu tiap 15 s.
  static bool was_open = false, armed = false;
  static uint32_t t_auto = 0;
  const bool is_open = (bool)ser;
  if (is_open && !was_open) { armed = true; t_auto = millis() + 2000; }
  if (!is_open) armed = false;
  was_open = is_open;
  if (armed && (int32_t)(millis() - t_auto) >= 0) {
    run_report();
    g_auto_runs++;
    t_auto = millis() + 15000;
  }
  if (ser.available() > 0) {
    char c = (char)ser.read();
    if (c == 'I' || c == 'i') cmd_info();
    else if (c == 'N' || c == 'n') { ina_report_single(ina, INA_CAL_V, INA_UA_PER_LSB, ser); ina_publish(); }
    else if (c == 'S' || c == 's') i2c_scan(Wire, ser);
    else if (c == 'C' || c == 'c') run_report();
  }
}
