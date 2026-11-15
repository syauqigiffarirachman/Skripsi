// ============================================================================
// L4: statistik tepi pin status (naik/turun) dari timestamp - BEBAS dari jenis STM32.
//
// feed(t, rising) dipanggil per tepi dengan t = timestamp 64-bit dalam tik timer (bisa dari ISR;
// hanya penjumlahan/min/maks, tanpa pembagian). Menghitung durasi HIGH (naik -> turun),
// LOW (turun -> naik) dan periode (naik -> naik berurutan), serta kesalahan urutan (dua tepi sejenis
// berturut-turut = tepi terlewat atau glitch). print() mengonversi ke mikrodetik dengan hz.
// ============================================================================
#pragma once
#include <Arduino.h>

struct EdgeAcc {
  uint32_t n = 0;
  uint64_t sum = 0, mn = 0xFFFFFFFFFFFFFFFFull, mx = 0;
  void add(uint64_t v) { n++; sum += v; if (v < mn) mn = v; if (v > mx) mx = v; }
};

struct EdgeEvent { uint64_t t; uint8_t rising; };

struct EdgeStats {
  uint32_t n_rise = 0, n_fall = 0, seq_err = 0;
  int8_t   last = -1;                 // -1 belum ada, 1 tepi terakhir naik, 0 turun
  uint64_t t_rise = 0, t_fall = 0;
  EdgeAcc  high, low, period;
  EdgeEvent first[8];                 // 8 tepi pertama (untuk dilihat mata)
  uint8_t  n_first = 0;

  void feed(uint64_t t, bool rising);
  // hz = frekuensi tik timer. Cetak bilangan bulat saja.
  void print(Print& out, uint32_t hz) const;
};
