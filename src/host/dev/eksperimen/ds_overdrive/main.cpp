// ============================================================================
// EKSPERIMEN (17 Nov 2026): DS28E38 dengan OVERDRIVE dan tunggu sign berbasis polling (bukan 130 ms tetap).
// Bukan firmware pengukuran tahap D; K2 baseline (src/host/dev/tahap_d/k2_hardware) TIDAK disentuh.
//
// Tujuan: (1) memangkas waktu bus (kirim ~24 ms + baca ~46 ms pada kecepatan standar) dengan overdrive;
//         (2) memangkas tunggu 130 ms: chip menghitung ~107 ms lalu mengirim; saat sibuk bus terbaca 1,
//             jadi bus diintip per bit dan hasil dibaca begitu siap (cara yang dulu dipakai firmware 'sederhana').
// Bit-banging 1-Wire sendiri di PA15 (open-drain + dorong tinggi aktif saat menulis 1 / menahan kuat), waktu
// dari DWT CYCCNT 48 MHz, supaya timing overdrive bisa disetel. Tanpa library OneWire.
//
// Siklus (diulang tiap ~6 s; hasil lewat USART1 -> logger, baris 'h,' ... dibaca tools/dev/tahap_stop/ds_od.py):
//   RC,<ns>                      waktu naik bus (pull-up) sampai terbaca tinggi
//   INFO,ROM|MANID|P0|PX|PY,hex  data chip (untuk verifikasi tanda tangan di PC)
//   SIGN,<mode>,<kirim_us>,<sibuk_us>,<baca_us>,<total_us>,<err>,<hex 64 B chip: s||r>
//        mode = STD130 (tunggu tetap 130 ms, seperti K2) | STDPOLL | ODPOLL | OD130
//   ODCFG,<i>,<A_ns>,<E_ns>,<H_ns>,<masuk OD>,<ok dari 3>
// Build: pio run -e host_ds_od   Flash: python tools/1_akuisisi/flash_stop.py .pio/build/host_ds_od/firmware.elf
// ============================================================================
#include <Arduino.h>
#include "../../../bersama/host_util.h"

#define PIN_DQ_BSRR_LOW  GPIO_BSRR_BR15
#define PIN_DQ_BSRR_HIGH GPIO_BSRR_BS15
#define OW_LOW()   (GPIOA->BSRR = PIN_DQ_BSRR_LOW)
#define OW_HIGH()  (GPIOA->BSRR = PIN_DQ_BSRR_HIGH)
#define OW_RD()    ((GPIOA->IDR & GPIO_IDR_IDR15) != 0)
static inline void ow_mode_od() { GPIOA->CRH = (GPIOA->CRH & ~(0xFUL << 28)) | (0x7UL << 28); }   // open-drain 50 MHz
static inline void ow_mode_pp() { GPIOA->CRH = (GPIOA->CRH & ~(0xFUL << 28)) | (0x3UL << 28); }   // push-pull 50 MHz

// ---- waktu (ns) dan tabel siklus ----
struct OwT { uint32_t A, B, C, D, E, F, H, I, J; };
static const OwT T_STD = {6000, 64000, 60000, 10000, 9000, 55000, 480000, 70000, 410000};
static OwT g_t = T_STD;
static uint32_t cA, cB, cC, cD, cE, cF, cH, cI, cJ;
static bool g_od = false;
static inline uint32_t ns2cy(uint32_t ns) { return (uint32_t)(((uint64_t)ns * (SystemCoreClock / 1000000UL)) / 1000ULL); }
static void ow_set(const OwT &t, bool od) {
  g_t = t; g_od = od;
  cA = ns2cy(t.A); cB = ns2cy(t.B); cC = ns2cy(t.C); cD = ns2cy(t.D); cE = ns2cy(t.E);
  cF = ns2cy(t.F); cH = ns2cy(t.H); cI = ns2cy(t.I); cJ = ns2cy(t.J);
}
static inline void until(uint32_t t0, uint32_t n) { while ((uint32_t)(cycles() - t0) < n) {} }

// ---- primitif slot ----
static void ow_wbit(int b) {
  __disable_irq();
  const uint32_t t0 = cycles();
  OW_LOW();
  if (b) {
    until(t0, cA);
    OW_HIGH(); ow_mode_pp();                      // naik cepat (aktif) di sisa slot
    until(t0, cA + cB);
    ow_mode_od();
  } else {
    until(t0, cC);
    OW_HIGH(); ow_mode_pp();
    until(t0, cC + cD);
    ow_mode_od();
  }
  __enable_irq();
}
static int ow_rbit() {
  __disable_irq();
  const uint32_t t0 = cycles();
  OW_LOW();
  until(t0, cA);
  OW_HIGH();                                      // lepas (open-drain), pull-up luar menaikkan
  until(t0, cA + cE);
  const int b = OW_RD();
  until(t0, cA + cE + cF);
  __enable_irq();
  return b;
}
static void ow_wbyte(uint8_t v) { for (int i = 0; i < 8; i++) { ow_wbit(v & 1); v >>= 1; } }
static uint8_t ow_rbyte() { uint8_t v = 0; for (int i = 0; i < 8; i++) v |= (uint8_t)(ow_rbit() << i); return v; }
static inline void ow_hold() { OW_HIGH(); ow_mode_pp(); }          // tahan kuat tinggi (daya parasit saat chip menghitung)
static bool ow_reset() {
  ow_mode_od(); OW_HIGH();
  const uint32_t tw = cycles();
  while (!OW_RD() && (uint32_t)(cycles() - tw) < ns2cy(2000000)) {}   // tunggu bus tinggi (maks 2 ms)
  __disable_irq();
  const uint32_t t0 = cycles();
  OW_LOW();
  until(t0, cH);
  OW_HIGH();
  until(t0, cH + cI);
  const bool present = !OW_RD();
  until(t0, cH + cI + cJ);
  __enable_irq();
  return present;
}

// ---- CRC (Maxim) ----
static uint16_t crc16(const uint8_t *p, int n, uint16_t c) {
  for (int i = 0; i < n; i++) { c ^= p[i]; for (int k = 0; k < 8; k++) c = (c & 1) ? (c >> 1) ^ 0xA001 : c >> 1; }
  return c;
}
static uint8_t crc8(const uint8_t *p, int n) {
  uint8_t c = 0;
  for (int i = 0; i < n; i++) { uint8_t b = p[i]; for (int k = 0; k < 8; k++) { uint8_t m = (c ^ b) & 1; c >>= 1; if (m) c ^= 0x8C; b >>= 1; } }
  return c;
}

// ---- XPC DS28E38 ----
struct Tm { uint32_t send_us, busy_us, read_us, total_us; };
#ifndef DS_POLL_START_MS
#define DS_POLL_START_MS 90
#endif
#define POLL_START_MS DS_POLL_START_MS
#define POLL_GAP_US   20
#define POLL_MAX_MS   600
// fixed_ms > 0: tunggu tetap lalu baca (seperti K2). fixed_ms == 0: intip per bit mulai POLL_START_MS.
// Hasil: dat = byte data (tanpa byte hasil), *nd = jumlah. Kode: 0 ok, 1 tak ada presence, 2 CRC perintah,
// 3 chip tak selesai, 4 panjang tak sah, 5 CRC balasan, 6 byte hasil != 0xAA.
static int __attribute__((noinline)) xpc(uint8_t cmd, const uint8_t *par, int plen, uint32_t fixed_ms,
                                         uint8_t *dat, int *nd, Tm *tm) {
  const uint32_t us = SystemCoreClock / 1000000UL;
  const uint32_t c0 = cycles();
  if (!ow_reset()) return 1;
  ow_wbyte(0xCC);
  const uint8_t hdr[3] = {0x66, (uint8_t)(plen + 1), cmd};
  for (int i = 0; i < 3; i++) ow_wbyte(hdr[i]);
  for (int i = 0; i < plen; i++) ow_wbyte(par[i]);
  uint8_t cr[2] = {ow_rbyte(), ow_rbyte()};
  uint16_t c = crc16(hdr, 3, 0); c = crc16(par, plen, c);
  if ((uint16_t)~c != (uint16_t)((cr[1] << 8) | cr[0])) return 2;
  ow_wbyte(0xAA);                                 // lepas: chip mulai memproses
  const uint32_t c_rel = cycles();
  tm->send_us = (c_rel - c0) / us;
  uint8_t len = 0;
  if (fixed_ms) {
    ow_hold();
    delay(fixed_ms);
    ow_mode_od();
    tm->busy_us = (cycles() - c_rel) / us;
    (void)ow_rbyte();
    len = ow_rbyte();
  } else {
    ow_hold();
    delay(POLL_START_MS);
    bool siap = false;
    const uint32_t t_lim = millis() + POLL_MAX_MS;
    for (;;) {
      ow_mode_od();
      const int b = ow_rbit();
      if (b == 0) { siap = true; break; }
      ow_hold();
      if ((int32_t)(millis() - t_lim) > 0) break;
      const uint32_t t1 = cycles(); until(t1, POLL_GAP_US * us);
    }
    tm->busy_us = (cycles() - c_rel) / us;
    if (!siap) return 3;
    len = 0x01;                                   // bit0 = 1, bit1 = 0 (baru terbaca)
    for (int i = 2; i < 8; i++) len |= (uint8_t)(ow_rbit() << i);
  }
  if (len < 3 || len >= 0xFE || len > 70) return 4;
  uint8_t body[72];
  for (int i = 0; i < len + 2; i++) body[i] = ow_rbyte();
  uint16_t rc = crc16(&len, 1, 0); rc = crc16(body, len, rc);
  const uint32_t c_end = cycles();
  tm->read_us = (c_end - c_rel) / us - tm->busy_us;
  tm->total_us = (c_end - c0) / us;
  if ((uint16_t)~rc != (uint16_t)((body[len + 1] << 8) | body[len])) return 5;
  if (body[0] != 0xAA) return 6;
  *nd = len - 1;
  memcpy(dat, body + 1, len - 1);
  return 0;
}

static const uint8_t DIGEST[32] = {
  0xD7, 0x2C, 0x10, 0x39, 0x78, 0x44, 0x3C, 0x67, 0xEA, 0xD7, 0x54, 0x29, 0x10, 0xF0, 0xEB, 0xF1,
  0xC8, 0xBA, 0xF6, 0x31, 0x82, 0xC0, 0x7E, 0x06, 0x2B, 0xCC, 0x6F, 0xC2, 0xE9, 0xA8, 0xA5, 0x0E
};

// ---- laporan lewat USART1 (logger) ----
static void link_init() {
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
  (void)RCC->APB2ENR;
  AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;   // PA15 bebas, SWD tetap
  GPIOA->CRH = (GPIOA->CRH & ~(0xFUL << 4)) | (0xAUL << 4);                         // PA9 AF push-pull
  USART1->CR1 = 0; USART1->CR2 = 0; USART1->CR3 = 0;
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + 57600) / 115200;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}
static void rep(const char *fmt, ...) {
  char b[200];
  va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
  for (const char *s = b; *s; s++) { while (!(USART1->SR & USART_SR_TXE)) {} USART1->DR = (uint8_t)*s; }
  while (!(USART1->SR & USART_SR_TC)) {}
}
static void hexs(char *o, const uint8_t *p, int n) { static const char H[] = "0123456789abcdef"; for (int i = 0; i < n; i++) { o[2*i] = H[p[i] >> 4]; o[2*i+1] = H[p[i] & 15]; } o[2*n] = 0; }

// ---- pengukuran waktu naik bus ----
static uint32_t rise_ns() {
  ow_mode_pp(); OW_LOW();
  { const uint32_t t = cycles(); until(t, ns2cy(30000)); }
  __disable_irq();
  ow_mode_od(); OW_HIGH();
  const uint32_t t0 = cycles();
  while (!OW_RD() && (uint32_t)(cycles() - t0) < ns2cy(50000)) {}
  const uint32_t dt = cycles() - t0;
  __enable_irq();
  return (uint32_t)((uint64_t)dt * 1000ULL / (SystemCoreClock / 1000000UL));
}

static bool read_rom(uint8_t *rom) {
  if (!ow_reset()) return false;
  ow_wbyte(0x33);
  for (int i = 0; i < 8; i++) rom[i] = ow_rbyte();
  return crc8(rom, 7) == rom[7];
}

static void sign_rep(const char *mode, uint32_t fixed_ms, int n_rep) {
  for (int k = 0; k < n_rep; k++) {
    uint8_t par[33], d[80]; int nd = 0; Tm tm = {0, 0, 0, 0};
    par[0] = 0; memcpy(par + 1, DIGEST, 32);
    const int e = xpc(0xA5, par, 33, fixed_ms, d, &nd, &tm);
    char hx[130] = "";
    if (e == 0 && nd >= 64) hexs(hx, d, 64);
    rep("SIGN,%s,%lu,%lu,%lu,%lu,%d,%s\n", mode, (unsigned long)tm.send_us, (unsigned long)tm.busy_us,
        (unsigned long)tm.read_us, (unsigned long)tm.total_us, e, hx);
    delay(30);
  }
}

// Masuk overdrive: reset standar, Overdrive Skip ROM (0x3C) pada kecepatan standar, lalu reset pada kecepatan OD.
static bool od_enter(const OwT &od) {
  ow_set(T_STD, false);
  if (!ow_reset()) return false;
  ow_wbyte(0x3C);
  ow_set(od, true);
  return ow_reset();
}
static void od_leave() { ow_set(T_STD, false); (void)ow_reset(); delay(2); (void)ow_reset(); }

static OwT make_od(uint32_t A, uint32_t E, uint32_t H, uint32_t I) {
  OwT t;
  t.A = A; t.B = 7000; t.C = 7500; t.D = 2500; t.E = E;
  t.F = (A + E < 9000) ? 9000 - A - E : 1000;     // slot baca total ~9 us
  t.H = H; t.I = I; t.J = 40000;
  return t;
}
static OwT g_od_ok;                               // konfigurasi OD pertama yang lulus
static bool g_od_found = false;

static void od_sweep() {
  static const uint32_t cfg[][4] = {   // A, E, H, I (ns)
    {1000, 1000, 70000, 8500}, {1000, 1500, 70000, 8500}, {1500, 1500, 70000, 8500},
    {2000, 2000, 70000, 8500}, {1000, 2500, 70000, 8500}, {1000, 1000, 80000, 10000},
    {1500, 2500, 80000, 10000}, {3000, 3000, 80000, 10000},
  };
  g_od_found = false;
  for (unsigned i = 0; i < sizeof cfg / sizeof cfg[0]; i++) {
    const OwT t = make_od(cfg[i][0], cfg[i][1], cfg[i][2], cfg[i][3]);
    const bool in = od_enter(t);
    int ok = 0;
    if (in) {
      for (int k = 0; k < 3; k++) {
        uint8_t par[1] = {0}, d[80]; int nd = 0; Tm tm;
        if (xpc(0xAA, par, 1, 30, d, &nd, &tm) == 0 && nd >= 9) ok++;
      }
    }
    rep("ODCFG,%u,%lu,%lu,%lu,%d,%d\n", i, (unsigned long)cfg[i][0], (unsigned long)cfg[i][1],
        (unsigned long)cfg[i][2], in ? 1 : 0, ok);
    od_leave();
    if (ok == 3 && !g_od_found) { g_od_ok = t; g_od_found = true; }
    delay(5);
  }
}

#ifdef DS_STRESS
// Uji ketahanan: DS_STRESS_N tanda tangan ODPOLL berturut-turut pada konfigurasi OD titik tengah
// (A=1,0 us, sampel 2,5 us), lalu STDPOLL pembanding; polling mulai DS_POLL_START_MS (lihat xpc).
#ifndef DS_STRESS_N
#define DS_STRESS_N 100
#endif
static void cycle() {
  ow_set(T_STD, false);
  rep("RC,%lu\n", (unsigned long)rise_ns());
  uint8_t rom[8], d[80], par[1] = {0}; int nd = 0; Tm tm; char hx[140];
  const bool rom_ok = read_rom(rom);
  hexs(hx, rom, 8); rep("INFO,ROM,%s,%d\n", hx, rom_ok ? 1 : 0);
  int e = xpc(0xAA, par, 1, 30, d, &nd, &tm);
  if (e == 0 && nd >= 9) { hexs(hx, d + 7, 2); rep("INFO,MANID,%s\n", hx); } else rep("INFO,MANID,ERR%d\n", e);
  const uint8_t pages[3] = {0, 4, 5};
  const char *nm[3] = {"P0", "PX", "PY"};
  for (int i = 0; i < 3; i++) {
    par[0] = pages[i];
    e = xpc(0x44, par, 1, 30, d, &nd, &tm);
    if (e == 0 && nd >= 32) { hexs(hx, d, 32); rep("INFO,%s,%s\n", nm[i], hx); } else rep("INFO,%s,ERR%d\n", nm[i], e);
  }
  const OwT mid = make_od(1000, 1500, 70000, 8500);
  if (od_enter(mid)) sign_rep("ODPOLL", 0, DS_STRESS_N); else rep("ODCFG,TITIKTENGAH,GAGAL\n");
  od_leave();
  sign_rep("STDPOLL", 0, 10);
  rep("SIKLUS,SELESAI\n");
}
#else
static void cycle() {
  ow_set(T_STD, false);
  rep("RC,%lu\n", (unsigned long)rise_ns());
  uint8_t rom[8];
  const bool rom_ok = read_rom(rom);
  char hx[140];
  hexs(hx, rom, 8); rep("INFO,ROM,%s,%d\n", hx, rom_ok ? 1 : 0);
  uint8_t d[80], par[1] = {0}; int nd = 0; Tm tm;
  int e = xpc(0xAA, par, 1, 30, d, &nd, &tm);
  if (e == 0 && nd >= 9) { hexs(hx, d + 7, 2); rep("INFO,MANID,%s\n", hx); } else rep("INFO,MANID,ERR%d\n", e);
  const uint8_t pages[3] = {0, 4, 5};
  const char *nm[3] = {"P0", "PX", "PY"};
  for (int i = 0; i < 3; i++) {
    par[0] = pages[i];
    e = xpc(0x44, par, 1, 30, d, &nd, &tm);
    if (e == 0 && nd >= 32) { hexs(hx, d, 32); rep("INFO,%s,%s\n", nm[i], hx); } else rep("INFO,%s,ERR%d\n", nm[i], e);
  }
  sign_rep("STD130", 130, 3);
  sign_rep("STDPOLL", 0, 3);
  od_sweep();
  if (g_od_found) {
    if (od_enter(g_od_ok)) {
      sign_rep("ODPOLL", 0, 3);
      sign_rep("OD130", 130, 2);
    } else rep("ODCFG,ULANG,GAGAL\n");
    od_leave();
  }
  rep("SIKLUS,SELESAI\n");
}

#endif   // DS_STRESS

void setup() {
  cyccnt_start();
  link_init();
  pinMode(PB1, OUTPUT); digitalWrite(PB1, LOW);
  ow_mode_od(); OW_HIGH();
  delay(2000);
}

void loop() {
  cycle();
  delay(6000);
}
