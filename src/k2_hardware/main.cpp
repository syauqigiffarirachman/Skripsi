// ============================================================================
// K2 hardware (DS28E38, ECDSA P-256 di IC) — TAHAP 1: BASE (dibangun ulang 14 Nov, "build from base")
//
// Aturan "build from base": tahap ini HANYA membuktikan hal berikut, tidak ada yang lain.
//   (1) satu tanda tangan ECDSA P-256 dari DS28E38 atas challenge konstan 32 B, valid di PC
//       (tools/k2_check.py);
//   (2) latensinya terukur akurat dengan DWT CYCCNT pada 48 MHz, dengan jendela ukur yang
//       didefinisikan eksplisit (di bawah), SAMA JENISNYA dengan K1 (tools/k1_check.py);
//   (3) protokol 1-Wire (XPC) ditulis sekecil mungkin, tanpa LED, tanpa mode manual, tanpa
//       pengukuran RAM/hash (itu tahap berikutnya).
// Pengganti firmware monolit lama src/main.cpp (tetap ada sebagai referensi, tidak dipakai).
//
// Yang berbeda dari firmware lama (dan harus dinyatakan di laporan):
//   - Pencarian ROM (ds.search) dilakukan SEKALI di setup(), bukan di setiap sign. Firmware lama
//     melakukannya tiap sign dan itu ikut dihitung di total 224 ms (sekitar 25 ms: search + reset
//     + select), padahal itu bukan bagian operasi sign.
//   - Pemilihan chip memakai Skip ROM (0xCC): hanya satu perangkat di bus.
//   - TANPA percobaan ulang otomatis: kegagalan CRC/hasil dihitung dan dicetak sebagai ERR,
//     supaya tidak tersembunyi di dalam angka latensi.
//   - Pustaka kripto tidak dipakai sama sekali (hash di dalam chip).
//
// Jendela ukur latensi = satu transaksi XPC penuh: reset + Skip ROM + kirim perintah 0xA5
// (+ CRC) + penantian chip (130 ms, nilai yang sama dengan firmware lama) + baca balasan 69 B
// sampai CRC terverifikasi. Pencetakan dan pencarian ROM di LUAR jendela.
// Catatan perbandingan dengan K1: DS28E38 menandatangani SHA-256(ROM||page0||challenge||
// page#||MANID) (hash dihitung chip), sedangkan K1 menandatangani digest langsung. Keduanya
// ECDSA P-256 atas 32 B hash; PC memverifikasi masing-masing dengan pesan yang sesuai.
// TAHAP 2 (14 Nov): + perintah M = RAM puncak, metode identik K1 (lihat blok "Tahap 2" di bawah).
//   Jalur sign tidak diubah.
// TAHAP 3 (14 Nov): + perintah W = jendela pengamatan dengan WFI, pewaktu hardware, pin PB1, %S_cpu;
//   penantian chip 130 ms juga tidur (WFI + TIM1). Mekanisme identik K1 (lihat blok "Tahap 3").
//   Jalur sign sama; satu-satunya perubahan di dalamnya: delay() -> wait_ms() (delay() di luar jendela).
// TAHAP 4 (14 Nov): + perintah A = jendela dengan permintaan NYATA dari attacker lewat USART2 (DMA,
//   satu bangun per frame; blok kode IDENTIK K1, lihat "Tahap 4"); setup() tidak lagi menunggu USB.
// Belum ada: handshake "papan siap" dan RUN_START/RUN_END (tahap 5).
//
// Perintah (USB CDC 115200, akhiri newline):
//   I        info: clock, ROM ID, MANID, halaman 0, kunci publik X||Y (halaman 4,5), challenge
//   R[n]     n kali sign (default 30, maks 100), CSV: i,cyc_sign,ms_sign,r||s(hex)
//   M[n]     ukur RAM puncak: n pengukuran (default 10, maks 100), CSV + ringkasan
//   W[s][,p] jendela s detik (default 10, maks 120), permintaan sintetis tiap p ms (default 1000,
//            100..6000); laporan T_obs, T_sleep, %S_cpu, dilayani/terlewat. Cetak setelah jendela.
//
// Build: pio run -e k2_hw   |  Upload: pio run -e k2_hw -t upload (cabut ST-Link setelah upload)
// ============================================================================
#include <Arduino.h>
#include <OneWire.h>

#define ser SerialUSB

#define PIN_ONEWIRE PA15

#define N_DEFAULT 30            // run per kelompok (Bab 3)
#define N_MAX     100
#define CMD_MAX   32

#define XPC_CMD         0x66
#define CMD_READ_STATUS 0xAA
#define CMD_READ_MEM    0x44
#define CMD_COMP_AUTH   0xA5
#define RESULT_OK       0xAA

#define DLY_READ_MS   30        // sama dengan firmware lama (baca status/memori)
#define DLY_SIGN_MS   130       // sama dengan firmware lama (sign); tak ada polling

// Challenge = SHA-256("Skripsi ECDSA P-256 software vs DS28E38"), sama dengan K1.
static const uint8_t DIGEST[32] = {
  0xD7, 0x2C, 0x10, 0x39, 0x78, 0x44, 0x3C, 0x67, 0xEA, 0xD7, 0x54, 0x29, 0x10, 0xF0, 0xEB, 0xF1,
  0xC8, 0xBA, 0xF6, 0x31, 0x82, 0xC0, 0x7E, 0x06, 0x2B, 0xCC, 0x6F, 0xC2, 0xE9, 0xA8, 0xA5, 0x0E
};

static OneWire ds(PIN_ONEWIRE);
static uint8_t g_rom[8];
static bool    g_rom_ok = false;

// ---------------------------------------------------------------------------
// DWT CYCCNT
static bool cyccnt_start() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  uint32_t a = DWT->CYCCNT;
  __NOP(); __NOP(); __NOP(); __NOP();
  return DWT->CYCCNT != a;
}

static inline uint32_t cycles() { return DWT->CYCCNT; }

// ---------------------------------------------------------------------------
// Protokol XPC DS28E38. Kode galat (0 = sukses):
//   1 tak ada presence   2 CRC perintah salah   3 panjang balasan tak sah
//   4 CRC balasan salah  5 byte hasil != 0xAA
static void wait_ms(uint32_t ms);                // didefinisikan di blok Tahap 3
static uint16_t crc16_maxim(const uint8_t *hdr, int hdr_len, const uint8_t *dat, int dat_len) {
  uint16_t crc = OneWire::crc16(hdr, hdr_len, 0);
  crc = OneWire::crc16(dat, dat_len, crc);
  return ~crc;
}

// reset + Skip ROM + (0x66, len, cmd, param) + CRC + lepas-kuat (0xAA, pull-up kuat) + tunggu
// dly_ms + baca (dummy, len, hasil, data, CRC). *n = jumlah byte data. noinline agar kompiler
// tidak menggeser kode ke luar kurung CYCCNT.
static int __attribute__((noinline)) xpc(uint8_t cmd, const uint8_t *par, int plen, uint32_t dly_ms,
                                         uint8_t *dat, int *n) {
  if (!ds.reset()) return 1;
  ds.skip();

  const uint8_t hdr[3] = { XPC_CMD, (uint8_t)(plen + 1), cmd };
  // power=1: jalur ditahan HIGH di antara byte (write_bytes() justru melepasnya)
  for (int i = 0; i < 3; i++) ds.write(hdr[i], 1);
  for (int i = 0; i < plen; i++) ds.write(par[i], 1);

  uint8_t crc[2];
  ds.read_bytes(crc, 2);
  ds.write(0xAA, 1);                             // lepas: chip mulai memproses (pull-up kuat)
  if (((crc[1] << 8) | crc[0]) != crc16_maxim(hdr, 3, par, plen)) return 2;

  wait_ms(dly_ms);                               // di luar jendela: delay(); di jendela: WFI (tahap 3)

  (void)ds.read();                               // byte dummy
  const int len = ds.read() + 2;
  const uint8_t result = ds.read();
  if (len < 3 || len >= 0xFE) return 3;
  const int nd = len - 3;
  ds.read_bytes(dat, nd);
  uint8_t rc[3];                                 // 2 byte CRC + 1 byte penutup (dibuang)
  ds.read_bytes(rc, 3);
  const uint8_t rh[2] = { (uint8_t)(len - 2), result };
  if (((rc[1] << 8) | rc[0]) != crc16_maxim(rh, 2, dat, nd)) return 4;
  *n = nd;
  return result == RESULT_OK ? 0 : 5;
}

// Satu operasi "sign": challenge 32 B, halaman 0 -> r||s (64 B). Chip mengirim s lalu r.
static int __attribute__((noinline)) sign_digest(const uint8_t *chal, uint8_t *sig) {
  uint8_t par[33], d[80];
  int n = 0;
  par[0] = 0;                                    // halaman 0, mode non-anonim (ROM ikut ditandatangani)
  memcpy(par + 1, chal, 32);
  int ret = xpc(CMD_COMP_AUTH, par, 33, DLY_SIGN_MS, d, &n);
  if (ret == 0 && n < 64) ret = 6;               // balasan terlalu pendek
  if (ret == 0) { memcpy(sig, d + 32, 32); memcpy(sig + 32, d, 32); }   // r || s
  return ret;
}

// ---------------------------------------------------------------------------
// Cetak (tanpa floating point)
static void print_hex(const uint8_t *p, size_t n) {
  static const char H[] = "0123456789ABCDEF";
  for (size_t i = 0; i < n; i++) { ser.write(H[p[i] >> 4]); ser.write(H[p[i] & 0xF]); }
}

// siklus -> "xxx.yyy" ms (mikro-detik dibulatkan ke bawah), 64-bit agar tidak meluap
static void print_ms(uint32_t cyc) {
  uint64_t us = (uint64_t)cyc * 1000000ULL / SystemCoreClock;
  ser.printf("%lu.%03lu", (unsigned long)(us / 1000), (unsigned long)(us % 1000));
}

static uint32_t parse_u32(const char *p, uint32_t def) {
  if (*p < '0' || *p > '9') return def;
  uint32_t v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (uint32_t)(*p++ - '0');
  return v;
}

static void cmd_info() {
  ser.printf("# MCU clock : %lu Hz\n", (unsigned long)SystemCoreClock);
  ser.println("# Chip      : DS28E38 (ECDSA P-256 di IC, hash di dalam chip), OneWire PA15, Skip ROM");
  ser.println("# Protokol  : sign = 0xA5 halaman 0 non-anonim, tunggu tetap 130 ms, tanpa retry");

  uint8_t d[80], par[1];
  int n = 0, ret;
  par[0] = 0;
  ret = xpc(CMD_READ_STATUS, par, 1, DLY_READ_MS, d, &n);
  if (ret != 0 || n < 9) { ser.printf("# GALAT status: %d\n", ret); return; }
  const uint8_t manid[2] = { d[7], d[8] };
  ser.print("# ROM ID    : "); print_hex(g_rom, 8); ser.println();
  ser.print("# MANID     : "); print_hex(manid, 2); ser.println();

  uint8_t page0[32], px[32], py[32];
  const uint8_t pages[3] = { 0, 4, 5 };
  uint8_t *dest[3] = { page0, px, py };
  for (int i = 0; i < 3; i++) {
    par[0] = pages[i];
    ret = xpc(CMD_READ_MEM, par, 1, DLY_READ_MS, d, &n);
    if (ret != 0 || n < 32) { ser.printf("# GALAT baca halaman %u: %d\n", pages[i], ret); return; }
    memcpy(dest[i], d, 32);
  }
  ser.print("# Page0     : "); print_hex(page0, 32); ser.println();
  ser.print("# Digest    : "); print_hex(DIGEST, 32); ser.println();
  ser.print("# PubKey XY : "); print_hex(px, 32); print_hex(py, 32); ser.println();
}

// n kali sign. Urutan per iterasi: ukur -> (di luar ukur) cetak.
static void cmd_run(uint32_t n) {
  uint8_t sig[64];
  uint64_t sum = 0;
  uint32_t mn = 0xFFFFFFFF, mx = 0, fail = 0;

  ser.println("i,cyc_sign,ms_sign,rs_hex");
  for (uint32_t i = 1; i <= n; i++) {
    uint32_t t0 = cycles();
    int ret = sign_digest(DIGEST, sig);
    uint32_t cyc = cycles() - t0;               // selisih tak bertanda: aman untuk 1 limpahan

    if (ret != 0) { fail++; ser.printf("%lu,ERR,%d\n", (unsigned long)i, ret); continue; }
    sum += cyc;
    if (cyc < mn) mn = cyc;
    if (cyc > mx) mx = cyc;
    ser.printf("%lu,%lu,", (unsigned long)i, (unsigned long)cyc);
    print_ms(cyc); ser.write(','); print_hex(sig, 64); ser.println();
  }

  uint32_t ok = n - fail;
  ser.printf("# n=%lu ok=%lu gagal=%lu\n", (unsigned long)n, (unsigned long)ok, (unsigned long)fail);
  if (ok) {
    ser.print("# mean="); print_ms((uint32_t)(sum / ok));
    ser.print(" ms  min="); print_ms(mn);
    ser.print(" ms  max="); print_ms(mx);
    ser.printf(" ms  (spread=%lu siklus)\n", (unsigned long)(mx - mn));
  }
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
// Tahap 2: RAM puncak, metode IDENTIK dengan K1 (src/k1_software/main.cpp, tahap 2):
//   statik = _end - _sdata; heap puncak = sbrk(0) - _end setelah operasi pemanasan; stack puncak =
//   area bebas di atas heap dicat 0xA5A5A5A5 sesudah pemanasan, satu operasi penuh (sign_digest,
//   yaitu transaksi XPC lengkap) dijalankan, lalu alamat terendah yang tertimpa dibaca SEBELUM
//   ada pencetakan (beda dari firmware lama yang membaca setelah printf).
extern "C" char *sbrk(int i);
extern char _sdata, _end, _estack;

#define STACK_PAINT 0xA5A5A5A5UL
#define HEAP_MARGIN 256

static inline uint32_t get_sp() {
  uint32_t sp;
  __asm__ volatile ("mov %0, sp" : "=r" (sp));
  return sp;
}

static volatile uint32_t *__attribute__((noinline)) stack_paint() {
  volatile uint32_t *lo = (volatile uint32_t *)(((uint32_t)sbrk(0) + HEAP_MARGIN + 3) & ~3UL);
  volatile uint32_t *hi = (volatile uint32_t *)((get_sp() - 16) & ~3UL);
  if (lo >= hi) return nullptr;
  for (volatile uint32_t *p = lo; p < hi; p++) *p = STACK_PAINT;
  return lo;
}

static uint32_t stack_low_water(volatile uint32_t *lo) {
  volatile uint32_t *p = lo;
  while (*p == STACK_PAINT) p++;
  return (uint32_t)p;
}

static int __attribute__((noinline)) one_operation() {
  uint8_t sig[64];
  return sign_digest(DIGEST, sig);
}

static void cmd_ram(uint32_t n) {
  const uint32_t total  = (uint32_t)&_estack - (uint32_t)&_sdata;
  const uint32_t statik = (uint32_t)&_end - (uint32_t)&_sdata;
  const uint32_t heap0  = (uint32_t)sbrk(0) - (uint32_t)&_end;

  if (one_operation() != 0) { ser.println("# GAGAL: operasi pemanasan"); return; }
  const uint32_t heap_warm = (uint32_t)sbrk(0) - (uint32_t)&_end;

  ser.println("i,heap,stack_puncak,statik,pakai_total,sisa");
  uint32_t heap_max = 0, stack_max = 0, fail = 0, heap_naik = 0;
  for (uint32_t i = 1; i <= n; i++) {
    volatile uint32_t *lo = stack_paint();
    if (!lo) { ser.println("# GAGAL: area cat stack kosong"); return; }
    const uint32_t brk0 = (uint32_t)sbrk(0);
    int ret = one_operation();
    const uint32_t brk1 = (uint32_t)sbrk(0);
    const uint32_t stack = (uint32_t)&_estack - stack_low_water(lo);   // sebelum cetak apa pun
    const uint32_t heap  = brk1 - (uint32_t)&_end;
    if (ret != 0) { fail++; ser.printf("%lu,ERR,%d\n", (unsigned long)i, ret); continue; }
    if (brk1 != brk0) heap_naik++;
    if (heap > heap_max)   heap_max = heap;
    if (stack > stack_max) stack_max = stack;
    const uint32_t pakai = statik + heap + stack;
    ser.printf("%lu,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)i, (unsigned long)heap,
               (unsigned long)stack, (unsigned long)statik, (unsigned long)pakai,
               (unsigned long)(total - pakai));
  }
  const uint32_t pakai_max = statik + heap_max + stack_max;
  ser.printf("# RAM total=%lu statik=%lu heap_boot=%lu heap_pemanasan=%lu\n", (unsigned long)total,
             (unsigned long)statik, (unsigned long)heap0, (unsigned long)heap_warm);
  ser.printf("# PUNCAK heap=%lu stack=%lu statik=%lu -> pakai=%lu B (%lu.%lu%%) sisa=%lu B\n",
             (unsigned long)heap_max, (unsigned long)stack_max, (unsigned long)statik,
             (unsigned long)pakai_max, (unsigned long)(pakai_max * 100 / total),
             (unsigned long)(pakai_max * 1000 / total % 10), (unsigned long)(total - pakai_max));
  ser.printf("# n=%lu gagal=%lu heap_tumbuh_saat_ukur=%lu\n", (unsigned long)n, (unsigned long)fail,
             (unsigned long)heap_naik);
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
// Tahap 3: WFI + pewaktu hardware + pin status PB1 + %S_cpu (mekanisme IDENTIK dengan K1,
// src/k1_software/main.cpp tahap 3; lihat penjelasan lengkap di sana). Fungsi tahap ini satu:
// satu jendela pengamatan di mana CPU tidur di antara permintaan sintetis TIM4 (perintah W).
// Tambahan khusus K2 (sesuai paper): selama penantian chip (130 ms) CPU juga tidur lewat WFI,
// dibangunkan oleh pewaktu TIM1 (interupsi), bukan delay() sibuk. Fase 1-Wire kirim/baca
// (bit-banging) tetap aktif karena tak bisa tidur. Di luar jendela (I, R, M) wait_ms = delay().
#define PIN_STATUS_HI() (GPIOB->BSRR = GPIO_BSRR_BS1)
#define PIN_STATUS_LO() (GPIOB->BSRR = GPIO_BSRR_BR1)

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

static volatile uint32_t g_ticks, g_K, g_arrived, g_t_last, g_t_first, g_tick_min, g_tick_max;
static volatile bool     g_pending, g_end, g_wait_done;
static bool              g_in_window = false;
// Akuntansi tidur (dipakai oleh tidur menunggu permintaan DAN tidur menunggu chip)
static uint64_t g_t_sleep, g_act_cyc, g_uw_rem;
static uint32_t g_seg, g_wfi_n, g_spurious, g_pin_salah;
static HardwareTimer *g_t1 = nullptr;           // TIM1: penantian chip (satu tembakan)

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

static void wait_isr() { g_wait_done = true; }

static bool idle_for_request() { return !g_pending && !g_end; }
static bool idle_for_chip()    { return !g_wait_done; }

// Satu segmen tidur. Pemeriksaan "tak ada pekerjaan" dilakukan di dalam PRIMASK=1 agar tak ada
// race; WFI tetap bangun oleh interupsi pending walau PRIMASK=1. SysTick dimatikan selama tidur
// dan uwTick dikoreksi sebesar waktu tidur (timeout HAL tetap benar).
static void sleep_while(bool (*idle)()) {
  __disable_irq();
  if (idle()) {
    g_act_cyc += (uint32_t)(cycles() - g_seg);
    PIN_STATUS_LO();
    if (GPIOB->IDR & GPIO_IDR_IDR1) g_pin_salah++;   // baca balik: harus 0
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    const uint32_t ts = hw_us();
    __DSB();
    __WFI();
    const uint32_t tw = hw_us();
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    PIN_STATUS_HI();
    g_seg = cycles();
    const uint32_t dt = tw - ts;
    g_t_sleep += dt; g_wfi_n++;
    g_uw_rem += dt; uwTick += (uint32_t)(g_uw_rem / 1000); g_uw_rem %= 1000;
    __enable_irq();                             // ISR yang menunggu berjalan di sini
    __ISB();                                    // M3 bisa menunda masuk ISR 1 instruksi setelah CPSIE
    if (idle()) g_spurious++;                   // bangun tapi kondisi belum terpenuhi
    return;
  }
  __enable_irq();
}

static void wait_ms(uint32_t ms) {
  if (!g_in_window) { delay(ms); return; }
  g_wait_done = false;
  g_t1->pause();
  g_t1->setOverflow(ms * 1000UL, MICROSEC_FORMAT);
  g_t1->setCount(0);
  TIM1->SR = 0; NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
  g_t1->resume();
  while (!g_wait_done) sleep_while(idle_for_chip);
  g_t1->pause();
}

// ---------------------------------------------------------------------------
// Tahap 4: permintaan NYATA dari attacker lewat USART2 (PA3 = RX), menggantikan TIM4 sintetis
// (perintah A). Penerimaan: DMA1 kanal 6 mengisi satu frame 36 B; interupsi selesai-transfer (TC)
// terjadi SEKALI per frame, jadi satu frame = satu bangun CPU (paper). Deteksi idle-line lewat
// interupsi USART2 tak dipakai karena core Arduino sudah mendefinisikan USART2_IRQHandler
// (tabrakan simbol); gantinya: bila frame tak valid (sinkron geser, byte hilang), ISR membuang
// byte sampai flag IDLE USART terangkat (atau 10 ms) lalu memasang ulang DMA, sehingga
// perataan frame pulih pada jeda antar-frame (attacker >= 100 ms antar frame).
// Frame (src/attacker/attacker.cpp): [0]=0xA5 [1..2]=seq uint16 LE [3..34]=isi [35]=XOR [0..34].
// Isi frame TIDAK dipakai: yang ditandatangani tetap DIGEST konstan (variabel kontrol K1/K2).
// Jendela berakhir oleh SATU interupsi TIM4 satu-tembakan (bukan tik tiap detik, agar tak ada
// bangun tambahan). Permintaan sah yang tiba saat sudah ada yang menunggu menyatu (buffer satu
// frame): terlewat = datang - dilayani; hilang = selisih seq (frame yang tak pernah sampai).
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

static void end_isr() { g_end = true; }

static void cmd_window(uint32_t secs, uint32_t p_ms, bool uart = false) {
  uint32_t K = secs * 1000UL / p_ms;
  if (K < 1 || uart) K = 1;
  static HardwareTimer t4(TIM4);                // statis: tanpa alokasi heap
  static HardwareTimer t1(TIM1);
  g_t1 = &t1;
  t1.attachInterrupt(wait_isr);
  t4.pause();
  if (uart) {
    t4.setOverflow(secs * 1000000UL, MICROSEC_FORMAT);   // satu tembakan = akhir jendela
    t4.setCount(0);
    t4.attachInterrupt(end_isr);
  } else {
    t4.setOverflow(p_ms * 1000UL, MICROSEC_FORMAT);
    t4.setCount(0);
    t4.attachInterrupt(tick_isr);
  }

  g_ticks = 0; g_K = K; g_arrived = 0; g_pending = false; g_end = false; g_wait_done = false;
  g_t_first = 0; g_tick_min = 0xFFFFFFFFUL; g_tick_max = 0;
  g_t_sleep = 0; g_act_cyc = 0; g_uw_rem = 0; g_wfi_n = 0; g_spurious = 0; g_pin_salah = 0;
  uint32_t served = 0, gagal = 0, lat_min = 0xFFFFFFFFUL, lat_max = 0;
  uint64_t lat_sum = 0;

  TIM4->SR = 0; NVIC_ClearPendingIRQ(TIM4_IRQn);   // buang update palsu akibat setOverflow
  // USB CDC (hanya saat tersambung PC) memicu interupsi SOF tiap 1 ms yang membangunkan WFI terus-
  // menerus. Di bangku ukur sebenarnya USB-C hanya daya (tanpa D+/D-), jadi interupsi itu tak ada.
  // Selama jendela SOF/ESOF di-mask agar uji lewat USB sama dengan kondisi ukur; dipulihkan setelahnya.
  const uint32_t usb_cntr = USB->CNTR;
  USB->CNTR = usb_cntr & ~(USB_CNTR_SOFM | USB_CNTR_ESOFM);

  g_in_window = true;
  const uint32_t t0 = hw_us(), c0 = cycles();
  g_t_last = t0;
  g_seg = c0;
  PIN_STATUS_HI();
  if (uart) rx_start();
  t4.resume();

  for (;;) {
    if (idle_for_request()) { sleep_while(idle_for_request); continue; }   // tidur menunggu permintaan
    if (g_end) break;
    __disable_irq(); g_pending = false; __enable_irq();
    uint8_t sig[64];
    const uint32_t s0 = cycles();
    const int ret = sign_digest(DIGEST, sig);    // penantian chip di dalamnya tidur lewat TIM1
    const uint32_t lat = cycles() - s0;
    if (ret == 0) {
      served++; lat_sum += lat;
      if (lat < lat_min) lat_min = lat;
      if (lat > lat_max) lat_max = lat;
    } else gagal++;
  }
  g_act_cyc += (uint32_t)(cycles() - g_seg);
  const uint32_t t_end = hw_us();
  const uint32_t c_end = cycles();
  g_in_window = false;
  PIN_STATUS_LO();
  if (uart) rx_stop();
  t4.pause();
  t4.detachInterrupt();
  t1.pause();
  t1.detachInterrupt();
  USB->CNTR = usb_cntr;

  const uint64_t t_obs = (uint32_t)(t_end - t0);
  const uint64_t act_us = g_act_cyc * 1000000ULL / SystemCoreClock;
  const uint64_t cyc_total_us = (uint64_t)(uint32_t)(c_end - c0) * 1000000ULL / SystemCoreClock;
  const int64_t resid = (int64_t)t_obs - (int64_t)g_t_sleep - (int64_t)act_us;
  const uint64_t sp = t_obs ? g_t_sleep * 10000ULL / t_obs : 0;      // persen x100

  if (uart) ser.printf("# WINDOW attacker (UART2 DMA) %lu s, akhir jendela oleh TIM4 satu-tembakan\n",
                       (unsigned long)secs);
  else      ser.printf("# WINDOW periode=%lu ms K=%lu (tik ke-K mengakhiri jendela)\n", (unsigned long)p_ms,
                       (unsigned long)K);
  ser.printf("# T_obs=%lu us  T_sleep=%lu us  aktif(CYCCNT)=%lu us  sisa=%ld us\n",
             (unsigned long)t_obs, (unsigned long)g_t_sleep, (unsigned long)act_us, (long)resid);
  ser.printf("# S_cpu=%lu.%02lu %%\n", (unsigned long)(sp / 100), (unsigned long)(sp % 100));
  ser.printf("# permintaan: datang=%lu dilayani=%lu gagal_sign=%lu terlewat=%lu\n",
             (unsigned long)g_arrived, (unsigned long)served, (unsigned long)gagal,
             (unsigned long)(g_arrived - served - gagal));
  if (served) {
    ser.print("# latensi sign di jendela: mean="); print_ms((uint32_t)(lat_sum / served));
    ser.print(" ms min="); print_ms(lat_min);
    ser.print(" ms max="); print_ms(lat_max);
    ser.println(" ms (penantian chip lewat WFI)");
  }
  ser.printf("# tidur: WFI=%lu bangun_sebelum_waktunya=%lu pin_PB1_salah_baca=%lu\n",
             (unsigned long)g_wfi_n, (unsigned long)g_spurious, (unsigned long)g_pin_salah);
  if (uart) {
    const uint32_t sent = g_rx_have ? (uint16_t)(g_rx_last_seq - g_rx_first_seq) + 1UL : 0;
    ser.printf("# frame: ok=%lu bad=%lu hilang_seq=%lu seq=%lu..%lu terkirim=%lu\n",
               (unsigned long)g_rx_ok, (unsigned long)g_rx_bad, (unsigned long)g_rx_lost,
               (unsigned long)g_rx_first_seq, (unsigned long)g_rx_last_seq, (unsigned long)sent);
    if (g_rx_bad) {
      ser.print("# bad pertama: "); print_hex(g_rx_badfirst, RX_FRAME_LEN);
      ser.printf(" (tunggu IDLE %lu us)\n", (unsigned long)g_rx_badfirst_wait);
    }
  } else {
    ser.printf("# tik: pertama=%lu us min=%lu us maks=%lu us\n", (unsigned long)g_t_first,
               (unsigned long)(g_tick_min == 0xFFFFFFFFUL ? 0 : g_tick_min), (unsigned long)g_tick_max);
  }
  ser.printf("# CYCCNT sepanjang jendela=%lu us (bandingkan T_obs: selisih >0 = CYCCNT berhenti saat WFI)\n",
             (unsigned long)cyc_total_us);
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
void setup() {
  ser.begin(115200);
  bool dwt_ok = cyccnt_start();
  ds.reset_search();
  g_rom_ok = ds.search(g_rom) && OneWire::crc8(g_rom, 7) == g_rom[7];   // sekali, di luar jendela ukur
  const bool hw_ok = hw_time_init();
  pinMode(PB1, OUTPUT);
  PIN_STATUS_LO();
  rx_init();

  // TIDAK menunggu koneksi USB (tahap 4): di bangku ukur USB-C hanya daya, dan menunggu sampai
  // 10 s merusak jendela 60 s. Banner hanya dicetak bila PC sudah terhubung saat boot.
  if (ser) {
    ser.println("K2 HARDWARE tahap 4 (DS28E38, base + RAM + WFI + UART2) siap. Perintah: I | R[n] | M[n] | W[s][,p] | A[s]");
    if (!dwt_ok)   ser.println("# PERINGATAN: DWT CYCCNT tidak berjalan");
    if (!g_rom_ok) ser.println("# PERINGATAN: DS28E38 tidak ditemukan di bus (PA15)");
    if (!hw_ok)    ser.println("# PERINGATAN: jam pewaktu APB1 bukan kelipatan 1 MHz");
  }
}

void loop() {
  static char line[CMD_MAX];
  static int n = 0;
  if (ser.available() <= 0) return;
  char c = ser.read();
  if (c == '\r') return;
  if (c != '\n') { if (n < CMD_MAX - 1) line[n++] = c; return; }
  line[n] = '\0'; n = 0;

  if (line[0] && !g_rom_ok) { ser.println("-1"); return; }
  switch (line[0]) {
    case 'I': cmd_info(); break;
    case 'R': {
      uint32_t k = parse_u32(line + 1, N_DEFAULT);
      if (k < 1 || k > N_MAX) { ser.println("-1"); break; }
      cmd_run(k);
      break;
    }
    case 'M': {
      uint32_t k = parse_u32(line + 1, 10);
      if (k < 1 || k > N_MAX) { ser.println("-1"); break; }
      cmd_ram(k);
      break;
    }
    case 'W': {
      // W[detik][,periode_ms]  default 10 s, 1000 ms. Contoh: W60 | W6,6000 (tanpa permintaan = idle)
      uint32_t s = parse_u32(line + 1, 10), p = 1000;
      const char *comma = strchr(line, ',');
      if (comma) p = parse_u32(comma + 1, 1000);
      if (s < 1 || s > 120 || p < 100 || p > 6000) { ser.println("-1"); break; }
      cmd_window(s, p);
      break;
    }
    case 'A': {
      // A[detik]  jendela dengan permintaan NYATA dari attacker (UART2), default 60 s, maks 80 s
      uint32_t s = parse_u32(line + 1, 60);
      if (s < 1 || s > 80) { ser.println("-1"); break; }
      cmd_window(s, 1000, true);
      break;
    }
    case '\0': break;
    default: ser.println("-1");
  }
}
