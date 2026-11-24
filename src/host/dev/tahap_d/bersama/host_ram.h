// ============================================================================
// Host (K1 dan K2): ukur RAM puncak satu operasi sign (perintah M). Metode IDENTIK di kedua
// arsitektur karena satu berkas.
//
//   RAM puncak = statik (.data + .bss) + heap puncak + stack puncak.
//   - statik : _end - _sdata (simbol linker).
//   - heap   : newlib tidak pernah mengembalikan memori ke sistem, jadi (sbrk(0) - _end) setelah
//              operasi = jejak heap puncak (termasuk overhead header malloc), bukan hanya byte
//              yang diminta.
//   - stack  : area bebas di atas heap dicat pola 0xA5A5A5A5 SESUDAH satu operasi pemanasan (heap
//              sudah di puncak), lalu satu operasi penuh dijalankan; alamat terendah yang tertimpa
//              = stack puncak. Dibaca SEBELUM ada pencetakan. Termasuk jejak ISR (USB) bila
//              kebetulan menyela.
//
// Pemakaian: main.cpp mendefinisikan `static int one_operation()` (satu operasi penuh, 0 = sukses)
// SEBELUM meng-include berkas ini.
// ============================================================================
#pragma once
#include "../../../bersama/host_util.h"

// Simbol linker: awal RAM, akhir .bss, puncak stack.
extern "C" char *sbrk(int i);
extern char _sdata, _end, _estack;

#define STACK_PAINT 0xA5A5A5A5UL
#define HEAP_MARGIN 256                        // jarak aman di atas heap saat mengecat stack

static inline uint32_t get_sp() {
  uint32_t sp;
  __asm__ volatile ("mov %0, sp" : "=r" (sp));
  return sp;
}

// Cat area bebas [sbrk(0)+margin, SP-16). noinline agar SP yang dibaca = SP fungsi ini
// (frame pemanggil di atasnya tidak ikut tertimpa). Mengembalikan batas bawah cat.
static volatile uint32_t *__attribute__((noinline)) stack_paint() {
  volatile uint32_t *lo = (volatile uint32_t *)(((uint32_t)sbrk(0) + HEAP_MARGIN + 3) & ~3UL);
  volatile uint32_t *hi = (volatile uint32_t *)((get_sp() - 16) & ~3UL);
  if (lo >= hi) return nullptr;
  for (volatile uint32_t *p = lo; p < hi; p++) *p = STACK_PAINT;
  return lo;
}

// Alamat terendah yang sudah tertimpa (awal stack puncak).
static uint32_t stack_low_water(volatile uint32_t *lo) {
  volatile uint32_t *p = lo;
  while (*p == STACK_PAINT) p++;
  return (uint32_t)p;
}

// Perintah M: n pengukuran, CSV per pengukuran + ringkasan puncak.
static void cmd_ram(uint32_t n) {
  const uint32_t total  = (uint32_t)&_estack - (uint32_t)&_sdata;    // seluruh RAM
  const uint32_t statik = (uint32_t)&_end - (uint32_t)&_sdata;       // .data + .bss
  const uint32_t heap0  = (uint32_t)sbrk(0) - (uint32_t)&_end;       // heap setelah boot

  // Pemanasan: satu operasi penuh supaya heap mencapai puncaknya sebelum stack dicat.
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
    if (brk1 != brk0) heap_naik++;              // heap tumbuh = cat stack tak sah untuk iterasi ini
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
