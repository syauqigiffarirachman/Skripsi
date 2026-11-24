// Alat ukur bersama untuk firmware utama (DS28E38) dan softwaretes (micro-ecc),
// supaya latensi, RAM, dan hashing diukur dengan cara yang identik.
// Sertakan di tepat satu file .cpp per firmware.
#pragma once
#include <Arduino.h>
#include <SHA256.h>   // rweather/Crypto

extern "C" char *sbrk(int i);
extern char _sdata, _end, _estack;   // simbol linker: awal RAM, akhir .bss, puncak stack

#define STACK_PAINT  0xA5A5A5A5UL
#define HEAP_MARGIN  256              // jarak aman di atas heap saat mengecat stack

static inline uint32_t get_sp() {
  uint32_t sp;
  __asm__ volatile ("mov %0, sp" : "=r" (sp));
  return sp;
}

// ---------- Latensi: DWT cycle counter (TRCENA + CYCCNTENA) ----------
static inline bool cyccnt_init() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  uint32_t a = DWT->CYCCNT;
  __NOP(); __NOP(); __NOP(); __NOP();
  return DWT->CYCCNT != a;
}

static inline uint32_t cycles() { return DWT->CYCCNT; }

static inline uint32_t cyc_to_us(uint32_t c) {
  return (uint32_t)((uint64_t)c * 1000000ULL / SystemCoreClock);
}

// ---------- RAM: stack yang dipakai satu operasi ----------
// stack_paint() mengecat area bebas di bawah stack; setelah operasi selesai,
// (SP pemanggil - stack_low_water()) = byte stack terdalam yang dipakai operasi itu.
static uint32_t *g_paint_lo = nullptr;

static void __attribute__((noinline)) stack_paint() {
  uint32_t *lo = (uint32_t *)(((uint32_t)sbrk(0) + HEAP_MARGIN + 3) & ~3UL);
  uint32_t *hi = (uint32_t *)((get_sp() - 16) & ~3UL);
  if (lo >= hi) return;
  g_paint_lo = lo;
  for (uint32_t *p = lo; p < hi; p++) *p = STACK_PAINT;
}

static uint32_t stack_low_water() {
  uint32_t *p = g_paint_lo;
  if (!p) return get_sp();
  while (*p == STACK_PAINT) p++;
  return (uint32_t)p;
}

static inline uint32_t ram_static_bytes() { return (uint32_t)&_end - (uint32_t)&_sdata; }
static inline uint32_t heap_bytes()       { return (uint32_t)sbrk(0) - (uint32_t)&_end; }

// ---------- Hash pesan di host: SHA-256 ----------
static void sha256_msg(const void *data, size_t len, uint8_t out[32]) {
  SHA256 h;
  h.update(data, len);
  h.finalize(out, 32);
}
