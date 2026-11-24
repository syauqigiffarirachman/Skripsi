// ============================================================================
// Driver DS28E38 di PA15 dengan OVERDRIVE dan tunggu sign berbasis polling (17 Nov 2026).
// Diturunkan HARFIAH dari src/host/dev/eksperimen/ds_overdrive/main.cpp (terbukti: 269/269 tanda tangan valid di PC,
// overdrive A=1,0 us sampel 2,5 us, total 115,6 ms vs 192,5 ms kecepatan standar + tunggu 130 ms).
// Perbedaan: hook g_ds_wait(ms) menggantikan delay() selama chip menghitung (K2S: tidur Stop dengan alarm RTC).
// Bit-banging memakai DWT CYCCNT (ns -> siklus, 48 MHz); pin: open-drain + dorong tinggi aktif; DQ kuat tinggi
// (push-pull) selama chip menghitung. Prasyarat: PA15 bebas JTAG (SWJ_CFG = JTAGDISABLE), cyccnt_start().
// Pemakaian: ds_init(); ds_od_enter() sekali (lalu ds_od_ensure() per permintaan); xpc(0xA5, ...) untuk sign.
// ============================================================================
#pragma once
#include <Arduino.h>
#include "../../bersama/host_util.h"

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
#define DS_POLL_START_MS 100                       // sibuk chip 105,9-107,3 ms (n=269): polling mulai 100 ms
#endif
#define POLL_START_MS DS_POLL_START_MS
// Hook tunggu chip (strong pull-up sudah ditahan): bawaan delay(); K2S menggantinya dengan tidur Stop.
static void ds_wait_default(uint32_t ms) { delay(ms); }
static void (*g_ds_wait)(uint32_t) = ds_wait_default;
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
    g_ds_wait(fixed_ms);
    ow_mode_od();
    tm->busy_us = (cycles() - c_rel) / us;
    (void)ow_rbyte();
    len = ow_rbyte();
  } else {
    ow_hold();
    g_ds_wait(POLL_START_MS);
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


// ---- API tingkat atas ----
static const uint8_t DS_DIGEST[32] = {
  0xD7, 0x2C, 0x10, 0x39, 0x78, 0x44, 0x3C, 0x67, 0xEA, 0xD7, 0x54, 0x29, 0x10, 0xF0, 0xEB, 0xF1,
  0xC8, 0xBA, 0xF6, 0x31, 0x82, 0xC0, 0x7E, 0x06, 0x2B, 0xCC, 0x6F, 0xC2, 0xE9, 0xA8, 0xA5, 0x0E
};

static void ds_init() {                           // bebaskan PA15 (JTDI), bus lepas (open-drain tinggi)
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
  (void)RCC->APB2ENR;
  AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
  OW_HIGH(); ow_mode_od();
  ow_set(T_STD, false);
}
static bool ds_read_rom(uint8_t *rom) {
  ow_set(T_STD, false);
  if (!ow_reset()) return false;
  ow_wbyte(0x33);
  for (int i = 0; i < 8; i++) rom[i] = ow_rbyte();
  return crc8(rom, 7) == rom[7];
}
// Konfigurasi OD titik tengah hasil sapuan 17 Nov: A = 1,0 us, sampel 2,5 us (rentang sah sampel 2,0-3,0 us).
static OwT ds_od_cfg() {
  OwT t;
  t.A = 1000; t.B = 7000; t.C = 7500; t.D = 2500; t.E = 1500; t.F = 6500; t.H = 70000; t.I = 8500; t.J = 40000;
  return t;
}
// Masuk overdrive: reset standar + Overdrive Skip ROM (0x3C), lalu reset pada kecepatan OD (presence).
static bool ds_od_enter() {
  ow_set(T_STD, false);
  if (!ow_reset()) return false;
  ow_wbyte(0x3C);
  ow_set(ds_od_cfg(), true);
  return ow_reset();
}
// Pastikan chip masih di OD (reset OD punya presence); bila tidak, masuk ulang.
static bool ds_od_ensure() {
  if (g_od && ow_reset()) return true;
  return ds_od_enter();
}
static void ds_od_leave() { ow_set(T_STD, false); (void)ow_reset(); delay(2); (void)ow_reset(); }

// Sign halaman 0 non-anonim atas challenge 32 B; sig = r||s (64 B). Kode galat xpc() (0 = sukses; 7 = balasan pendek).
static int ds_sign(const uint8_t *chal, uint8_t *sig, Tm *tm) {
  uint8_t par[33], d[80];
  int nd = 0;
  par[0] = 0;
  memcpy(par + 1, chal, 32);
  const int e = xpc(0xA5, par, 33, 0, d, &nd, tm);
  if (e) return e;
  if (nd < 64) return 7;
  memcpy(sig, d + 32, 32); memcpy(sig + 32, d, 32);
  return 0;
}
