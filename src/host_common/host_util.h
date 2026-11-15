// ============================================================================
// Host (K1 dan K2): alat dasar yang dipakai semua bagian lain.
//   - DWT CYCCNT  : pencacah siklus inti untuk latensi sign.
//   - cetak       : heksadesimal dan milidetik tanpa floating point, ke USB CDC.
//   - parse_u32   : bilangan desimal dari baris perintah.
//
// Berkas ini satu-satunya tempat `ser` didefinisikan (USB CDC host).
// ============================================================================
#pragma once
#include <Arduino.h>

#define ser SerialUSB

// ---------------------------------------------------------------------------
// DWT CYCCNT. Latensi = selisih dua pembacaan sebagai bilangan bulat tak bertanda, jadi tetap
// benar walau pencacah melimpah satu kali di antaranya (32 bit = 89 s pada 48 MHz).
static bool cyccnt_start() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  uint32_t a = DWT->CYCCNT;
  __NOP(); __NOP(); __NOP(); __NOP();
  return DWT->CYCCNT != a;                      // false = pencacah tidak berjalan
}

static inline uint32_t cycles() { return DWT->CYCCNT; }

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

// Bilangan desimal di awal p; def bila p tidak diawali angka.
static uint32_t parse_u32(const char *p, uint32_t def) {
  if (*p < '0' || *p > '9') return def;
  uint32_t v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (uint32_t)(*p++ - '0');
  return v;
}
