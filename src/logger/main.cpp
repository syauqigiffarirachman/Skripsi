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
// A1 (15 Nov): pemulihan bus I2C (INA226::recover = Wire.end + Wire.begin + init ulang). Dipanggil di
//   L3 bila gagal >= 3 kali beruntun, dan di laporan bila init gagal. Kejadian 15 Nov (err=4 setelah
//   cabut-pasang kabel, hanya pulih lewat reset SWD) adalah alasan tahap ini.
// L5 (15 Nov): USART1 (PA9 TX -> PA10 host, PA10 RX <- PA9 host, 115200) = jalur kendali run ke host
//   (protokol di src/host_common/host_link.h): logger mengirim ?, S<detik>, W<detik>,<p>; host
//   membalas >START, >END + hasil, >L latensi, >DONE.
// L6 (15 Nov): perekaman satu run + keluaran CSV streaming ke PC (USB IN). Baris:
//     #RUN,BEGIN,<no>,<cmd>,<arg>,<tik_hz>,<build>      awal run
//     s,<t>,<shunt_raw>,<bus_raw>                        satu cuplikan INA226 (t = tik 72 MHz sejak awal rekam)
//     e,<t>,<1|0>                                        tepi pin status host di PA0 (1 = naik)
//     h,<t>,<baris dari host>                            baris USART1 dari host apa adanya
//     #SUM,...                                           ringkasan logger (jumlah cuplikan, galat, checksum)
//     #RUN,END,<no>,<status>                             akhir run (OK / ERR_...)
//   Semua cap waktu memakai SATU basis: TIM2 (kristal 72 MHz) + penghitung overflow = 64-bit.
//   Rekaman dimulai ~100 ms SEBELUM perintah dikirim ke host dan berlanjut sampai >DONE, jadi jendela
//   host (tepi naik pertama PB1 = t0, tepi turun terakhir = akhir) selalu tercakup; PC yang memotong.
// PERINTAH DARI PC: jalur OUT USB klon ini gagal, jadi perintah dikirim lewat SET_LINE_CODING (nilai
//   baud, lewat endpoint kontrol): baud = 1.000.000 + cmd x 1000 + arg. Setelah dijalankan PC
//   mengembalikan baud ke 115200. cmd: 1 info, 2 laporan uji 5 s (L3+L4 lama), 3 tanya identitas host,
//   4 run S<arg> (attacker nyata), 5 run W<arg>,1000, 6 run W<arg>,100, 7 rekam saja <arg> detik.
//   Laporan otomatis tiap 15 s DIHAPUS (bentrok dengan run); gantinya detak "#HB" tiap 2 s saat diam.
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
#include "usbd_cdc.h"

#define ser     SerialUSB
#define HostSer Serial1                         // USART1 PA9 (TX) / PA10 (RX) ke host (-DENABLE_HWSERIAL1)
extern "C" USBD_CDC_LineCodingTypeDef linecoding;   // core: diisi SET_LINE_CODING (kanal perintah PC)
static const uint8_t PIN_LED = PC13;            // LED Blue Pill, aktif rendah

// Dibaca lewat SWD untuk verifikasi (volatile + non-static agar simbolnya muncul di nm).
volatile uint32_t g_beat      = 0;              // naik tiap 500 ms
volatile uint32_t g_sysclk_hz = 0;              // SystemCoreClock hasil konfigurasi
volatile uint32_t g_pclk1_hz  = 0;
volatile uint32_t g_pclk2_hz  = 0;
volatile uint8_t  g_clk_hse   = 0;              // 1 = SYSCLK dari HSE+PLL, 0 = fallback HSI
volatile uint32_t g_build_id  = 0x4C364131;     // 'L6A1' = F103 tahap L1..L6 + A1 (pemulihan I2C)

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
volatile uint32_t g_i2c_recover  = 0;           // total pemanggilan INA226::recover() sejak boot
volatile uint32_t g_l3_rate_x100 = 0;           // cuplikan/s x 100
volatile uint32_t g_auto_runs    = 0;           // berapa kali laporan otomatis selesai
volatile uint32_t g_t2_ovf       = 0;           // jumlah overflow TIM2 (L4)
volatile uint32_t g_edge_rise    = 0;           // tepi naik PA0 sejak boot
volatile uint32_t g_edge_fall    = 0;
volatile uint32_t g_run_no       = 0;           // nomor run sejak boot (L6)
volatile uint32_t g_cmd_n        = 0;           // perintah PC yang diterima
volatile uint32_t g_last_baud    = 0;           // nilai baud terakhir dari PC (diagnosis kanal perintah)

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
// L6: cincin tepi ISR -> loop utama (dikuras tiap putaran rekam, ~0,2 ms). Penuh = dihitung er_drop.
struct EdgeRec { uint64_t t; uint8_t rising; };
#define ER_N 128                                // pangkat dua
static EdgeRec ering[ER_N];
static volatile uint16_t er_w = 0, er_r = 0;
static volatile uint32_t er_drop = 0;
static volatile bool     er_on = false;
static inline void er_push(uint64_t t, bool rising) {
  if (!er_on) return;
  const uint16_t w = er_w;
  if ((uint16_t)(w - er_r) >= ER_N) { er_drop++; return; }
  ering[w & (ER_N - 1)].t = t;
  ering[w & (ER_N - 1)].rising = rising ? 1 : 0;
  er_w = (uint16_t)(w + 1);
}
static void cb_rise() { g_edge_rise++; const uint64_t t = ext_ts((uint16_t)TIM2->CCR1); edge.feed(t, true);  er_push(t, true); }
static void cb_fall() { g_edge_fall++; const uint64_t t = ext_ts((uint16_t)TIM2->CCR2); edge.feed(t, false); er_push(t, false); }

// Waktu sekarang dalam basis yang sama dengan tepi (tik TIM2 64-bit). Dipanggil dari loop utama.
static inline uint64_t now_ticks() {
  noInterrupts();
  const uint16_t c = (uint16_t)TIM2->CNT;
  uint32_t hi = g_t2_ovf;
  if ((TIM2->SR & TIM_SR_UIF) && c < 0x8000) hi++;
  interrupts();
  return ((uint64_t)hi << 16) | c;
}
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
  g_i2c_recover = ina.recoverCount;
}

// Init INA226, satu pembacaan mentah, lalu rekam 5 s dan cetak statistik (sama dengan N + C di F030).
static void run_report() {
  static L3Stats st;                                // static: tidak membebani stack
  bool ok = ina_report_single(ina, INA_CAL_V, INA_UA_PER_LSB, ser);
  if (!ok) {                                        // A1: bus macet? pulihkan lalu coba sekali lagi
    const bool rec = ina.recover();
    ser.printf("pulihkan bus I2C: %s (total pulih=%lu)\r\n", rec ? "OK" : "GAGAL", (unsigned long)ina.recoverCount);
    ok = ina_report_single(ina, INA_CAL_V, INA_UA_PER_LSB, ser);
  }
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

// ---------------------------------------------------------------------------
// L5 + L6: satu run = rekam INA226 + tepi PA0 + baris host, dialirkan ke PC sebagai CSV.
// ---------------------------------------------------------------------------
static char* put_u64(char* p, uint64_t v) {     // desimal tanpa printf (newlib-nano tak punya %llu)
  char t[21];
  int n = 0;
  do { t[n++] = (char)('0' + (unsigned)(v % 10)); v /= 10; } while (v);
  while (n) *p++ = t[--n];
  return p;
}
static char* put_i32(char* p, int32_t v) {
  if (v < 0) { *p++ = '-'; return put_u64(p, (uint64_t)(-(int64_t)v)); }
  return put_u64(p, (uint64_t)v);
}

static const uint32_t RUN_PRE_MS      = 100;    // rekam dulu sebelum perintah dikirim ke host
static const uint32_t RUN_START_TO_MS = 1500;   // batas menunggu >START
static const uint32_t RUN_EXTRA_MS    = 6000;   // batas menunggu >END setelah <detik> jendela
static const uint32_t RUN_DONE_TO_MS  = 4000;   // batas menunggu >DONE setelah >END
static const uint32_t RUN_LATE_US     = 3300;   // selang cuplik > 1,5 x 2,2 ms = terlambat

// cmd: 4 = S<arg>, 5 = W<arg>,1000, 6 = W<arg>,100, 7 = rekam saja <arg> detik (tanpa host)
static void run_capture(uint32_t cmd, uint32_t arg) {
  char b[260];
  const uint32_t no = ++g_run_no;
  if (!ina.init(INA226::CFG_CONT_1P1MS, INA_CAL_V) && !ina.recover()) {
    ina_publish();
    ser.printf("#RUN,END,%lu,ERR_INA,%u\r\n", (unsigned long)no, (unsigned)ina.lastErr);
    return;
  }
  ina_publish();
  while (HostSer.available() > 0) (void)HostSer.read();          // buang sisa baris lama
  ser.printf("#RUN,BEGIN,%lu,%lu,%lu,%lu,%08lX\r\n", (unsigned long)no, (unsigned long)cmd,
             (unsigned long)arg, (unsigned long)tim_hz, (unsigned long)g_build_id);

  const uint64_t ms_t = tim_hz / 1000u;         // tik per ms
  const uint64_t late_thr = (uint64_t)RUN_LATE_US * (tim_hz / 1000000u);
  noInterrupts(); edge = EdgeStats(); er_w = er_r = 0; er_drop = 0; er_on = true; interrupts();
  const uint64_t t0 = now_ticks();

  enum { ST_PRE, ST_WAIT_START, ST_WINDOW, ST_POST, ST_END } st = ST_PRE;
  const char* status = "OK";
  uint64_t t_state = 0;                         // waktu (relatif) masuk keadaan sekarang
  uint32_t n = 0, late = 0, i2c_err = 0, rec = 0, rec_ok = 0, consec = 0, h_lines = 0, e_lines = 0, h_ovf = 0;
  int64_t  sh_sum = 0;
  uint64_t bus_sum = 0, t_prev = 0, dt_max = 0, t_last_rec = 0;
  bool have_prev = false, rec_once = false, r = false;
  char hl[220];                                 // baris >END host ~110 karakter
  uint8_t hn = 0;
  ina.ready(r);                                 // buang flag konversi lama

  while (st != ST_END) {
    // --- INA226: polling flag konversi; cap waktu = awal pembacaan MASK yang menemukan flag ---
    const uint64_t ts = now_ticks() - t0;
    bool err = false;
    if (!ina.ready(r)) err = true;
    else if (r) {
      int16_t sh; uint16_t bus;
      if (!ina.readShuntBus(sh, bus)) err = true;
      else {
        consec = 0;
        if (have_prev) {
          const uint64_t dt = ts - t_prev;
          if (dt > dt_max) dt_max = dt;
          if (dt > late_thr) late++;
        }
        t_prev = ts; have_prev = true;
        n++; sh_sum += sh; bus_sum += bus;
        char* p = b;
        *p++ = 's'; *p++ = ',';
        p = put_u64(p, ts);          *p++ = ',';
        p = put_i32(p, sh);          *p++ = ',';
        p = put_u64(p, bus);         *p++ = '\r'; *p++ = '\n';
        ser.write((const uint8_t*)b, (size_t)(p - b));
      }
    } else consec = 0;
    if (err) {
      i2c_err++;
      if (++consec >= 3 && (!rec_once || (ts - t_last_rec) >= 10 * ms_t)) {   // A1: pulihkan bus
        rec++;
        if (ina.recover()) rec_ok++;
        t_last_rec = now_ticks() - t0; rec_once = true; consec = 0;
      }
    }

    // --- tepi PA0 dari ISR ---
    while (er_r != er_w) {
      const EdgeRec e = ering[er_r & (ER_N - 1)];
      er_r = (uint16_t)(er_r + 1);
      char* p = b;
      *p++ = 'e'; *p++ = ',';
      p = put_u64(p, e.t - t0);      *p++ = ',';
      *p++ = e.rising ? '1' : '0';   *p++ = '\r'; *p++ = '\n';
      ser.write((const uint8_t*)b, (size_t)(p - b));
      e_lines++;
    }

    // --- baris dari host (USART1) ---
    const uint64_t now = now_ticks() - t0;
    while (HostSer.available() > 0) {
      const char c = (char)HostSer.read();
      if (c == '\r') continue;
      if (c != '\n') { if (hn < sizeof hl - 1) hl[hn++] = c; else h_ovf++; continue; }
      hl[hn] = '\0';
      if (hn > 0) {
        char* p = b;
        *p++ = 'h'; *p++ = ',';
        p = put_u64(p, now);         *p++ = ',';
        memcpy(p, hl, hn); p += hn;  *p++ = '\r'; *p++ = '\n';
        ser.write((const uint8_t*)b, (size_t)(p - b));
        h_lines++;
        if (st == ST_WAIT_START && !strncmp(hl, ">START", 6)) { st = ST_WINDOW; t_state = now; }
        else if (st == ST_WINDOW && !strncmp(hl, ">END", 4))  { st = ST_POST;   t_state = now; }
        else if (st == ST_POST && !strncmp(hl, ">DONE", 5))   { st = ST_END; }
        else if (!strncmp(hl, ">ERR", 4))                     { status = "ERR_HOST"; st = ST_END; }
      }
      hn = 0;
    }

    // --- mesin keadaan + batas waktu ---
    const uint64_t in_state = now - t_state;
    if (st == ST_PRE && in_state >= RUN_PRE_MS * ms_t) {
      if (cmd == 7) { st = ST_WINDOW; t_state = now; }
      else {
        if (cmd == 4)      HostSer.printf("S%lu\n", (unsigned long)arg);
        else if (cmd == 5) HostSer.printf("W%lu,1000\n", (unsigned long)arg);
        else               HostSer.printf("W%lu,100\n", (unsigned long)arg);
        st = ST_WAIT_START; t_state = now;
      }
    } else if (st == ST_WAIT_START && in_state >= RUN_START_TO_MS * ms_t) {
      status = "ERR_NOSTART"; st = ST_END;
    } else if (st == ST_WINDOW) {
      if (cmd == 7) { if (in_state >= (uint64_t)arg * 1000u * ms_t) st = ST_END; }
      else if (in_state >= ((uint64_t)arg * 1000u + RUN_EXTRA_MS) * ms_t) { status = "ERR_NOEND"; st = ST_END; }
    } else if (st == ST_POST && in_state >= RUN_DONE_TO_MS * ms_t) {
      status = "ERR_NODONE"; st = ST_END;
    }
  }
  const uint64_t t_stop = now_ticks() - t0;
  noInterrupts(); er_on = false; interrupts();
  static EdgeStats snap;
  noInterrupts(); snap = edge; interrupts();

  // Ringkasan: PC mencocokkan jumlah baris s/e/h dan jumlah shunt/bus (deteksi baris hilang di USB).
  char* p = b;
  memcpy(p, "#SUM,", 5); p += 5;
  p = put_u64(p, no);                 *p++ = ',';
  p = put_u64(p, t_stop);             *p++ = ',';   // lama rekam (tik)
  p = put_u64(p, n);                  *p++ = ',';   // cuplikan
  p = put_u64(p, late);               *p++ = ',';
  p = put_u64(p, dt_max);             *p++ = ',';   // selang cuplik terbesar (tik)
  p = put_u64(p, i2c_err);            *p++ = ',';
  p = put_u64(p, rec_ok);             *p++ = ',';
  p = put_u64(p, rec);                *p++ = ',';
  if (sh_sum < 0) { *p++ = '-'; p = put_u64(p, (uint64_t)(-sh_sum)); } else p = put_u64(p, (uint64_t)sh_sum);
  *p++ = ',';
  p = put_u64(p, bus_sum);            *p++ = ',';
  p = put_u64(p, e_lines);            *p++ = ',';   // tepi terkirim
  p = put_u64(p, er_drop);            *p++ = ',';   // tepi terbuang (cincin penuh)
  p = put_u64(p, snap.seq_err);       *p++ = ',';
  p = put_u64(p, h_lines);            *p++ = ',';
  p = put_u64(p, h_ovf);              *p++ = '\r'; *p++ = '\n';
  ser.write((const uint8_t*)b, (size_t)(p - b));
  ser.printf("#RUN,END,%lu,%s\r\n", (unsigned long)no, status);
}

// Tanya identitas host (perintah '?'); balasan dicetak sebagai baris h.
static void host_ping() {
  while (HostSer.available() > 0) (void)HostSer.read();
  HostSer.print("?\n");
  char hl[64];
  uint8_t hn = 0;
  const uint32_t t = millis();
  while ((uint32_t)(millis() - t) < 300) {
    if (HostSer.available() <= 0) continue;
    const char c = (char)HostSer.read();
    if (c == '\r') continue;
    if (c == '\n') { hl[hn] = '\0'; ser.printf("h,0,%s\r\n", hl); return; }
    if (hn < sizeof hl - 1) hl[hn++] = c;
  }
  ser.printf("#ERR,host_diam,%u\r\n", (unsigned)hn);
}

static void cmd_info() {
  const uint32_t* uid = (const uint32_t*)0x1FFFF7E8;   // UID 96-bit
  ser.printf("LOGGER F103 tahap L1..L6+A1 (build %08lX)\n", (unsigned long)g_build_id);
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
  HostSer.begin(115200);                           // L5: USART1 ke host (PA9 TX, PA10 RX)
  Wire.setSDA(PB7);
  Wire.setSCL(PB6);
  Wire.begin();
  Wire.setClock(ina.i2cHz);                        // 400 kHz; recover() memasang ulang nilai yang sama
  ina.init(INA226::CFG_CONT_1P1MS, INA_CAL_V);     // hasil di g_ina_* (SWD tanpa USB)
  ina_publish();
  l4_start();
}

// Jalankan satu perintah PC (dari nilai baud atau, bila jalur OUT bekerja, dari teks "!cmd,arg").
static void exec_cmd(uint32_t cmd, uint32_t arg) {
  g_cmd_n++;
  ser.printf("#CMD,%lu,%lu\r\n", (unsigned long)cmd, (unsigned long)arg);
  switch (cmd) {
    case 1: cmd_info(); break;
    case 2: run_report(); break;
    case 3: host_ping(); break;
    case 4: case 5: case 6:
      if (arg < 1 || arg > 120 || (cmd == 4 && arg > 80)) { ser.print(F("#ERR,arg\r\n")); break; }
      run_capture(cmd, arg);
      break;
    case 7:
      if (arg < 1 || arg > 600) { ser.print(F("#ERR,arg\r\n")); break; }
      run_capture(cmd, arg);
      break;
    default: ser.print(F("#ERR,cmd\r\n"));
  }
  ser.print(F("#CMD,SELESAI\r\n"));
}

void loop() {
  static uint32_t t_beat = 0;
  if ((uint32_t)(millis() - t_beat) >= 5000) t_beat = millis() - 500;   // sehabis run panjang: jangan kejar ketinggalan
  if ((uint32_t)(millis() - t_beat) >= 500) {
    t_beat += 500;
    g_beat++;
    digitalWrite(PIN_LED, (g_beat & 1) ? LOW : HIGH);
    // Detak tiap 2 s saat diam (bukti port hidup + diagnosis kanal perintah). Hanya bila port dibuka.
    if ((g_beat & 3) == 0 && ser.dtr())
      ser.printf("#HB,%lu,%lu,%u\r\n", (unsigned long)millis(), (unsigned long)linecoding.bitrate, (unsigned)ser.rts());
  }
  // Kanal perintah PC: perubahan nilai baud ke 1.000.000 + cmd x 1000 + arg.
  const uint32_t baud = linecoding.bitrate;
  if (baud != g_last_baud) {
    g_last_baud = baud;
    if (baud >= 1000000UL && baud < 2000000UL) exec_cmd((baud - 1000000UL) / 1000u, baud % 1000u);
  }
  // Cadangan bila jalur OUT USB bekerja: baris teks "!cmd,arg".
  static char line[16];
  static uint8_t ln = 0;
  while (ser.available() > 0) {
    const char c = (char)ser.read();
    if (c == '\r') continue;
    if (c != '\n') { if (ln < sizeof line - 1) line[ln++] = c; continue; }
    line[ln] = '\0'; ln = 0;
    if (line[0] == '!') {
      const char* comma = strchr(line, ',');
      exec_cmd((uint32_t)atoi(line + 1), comma ? (uint32_t)atoi(comma + 1) : 0);
    }
  }
}
