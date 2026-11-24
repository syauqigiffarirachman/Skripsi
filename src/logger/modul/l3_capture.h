// ============================================================================
// L3: cuplik INA226 kontinu + cap waktu (BEBAS dari jenis STM32).
//
// Platform (F030 / F103) wajib menyediakan dua fungsi berikut:
//   plat_cycles() : pencacah siklus CPU 32-bit (F030: SysTick, F103: DWT CYCCNT), wrap dipakai modular.
//   plat_hz()     : frekuensi pencacah itu (Hz), mis. 48000000.
//
// Cara kerja: polling register MASK (CVRF). Tiap kali konversi baru selesai, shunt+bus dibaca dan
// diberi cap waktu = saat pembacaan MASK yang menemukan flag itu dimulai. Ketidakpastian cap waktu
// = satu periode polling (diukur dan dilaporkan), karena tidak ada pin ALERT yang dipakai.
// Hasil dikumpulkan sebagai statistik berjalan (tanpa menyimpan semua cuplikan, RAM kecil).
// A1 (15 Nov): bila I2C gagal >= 3 kali berturut-turut, ina.recover() dipanggil (paling cepat tiap
// 10 ms). Celah cuplikan selama macet TIDAK disembunyikan: muncul sebagai selang panjang (late/missed).
// ============================================================================
#pragma once
#include "ina226.h"

uint32_t plat_cycles();
uint32_t plat_hz();

struct L3Sample { uint32_t t_rel; int16_t shunt; uint16_t bus; };   // t_rel = siklus sejak awal rekam

struct L3Stats {
  uint32_t n = 0;                 // cuplikan terbaca
  uint32_t polls = 0;             // jumlah pembacaan MASK sukses
  uint32_t i2c_err = 0;           // kegagalan I2C selama rekam
  uint32_t recover = 0;           // A1: pemulihan bus dipanggil (setelah >= 3 gagal berturut-turut)
  uint32_t recover_ok = 0;        //     ... dan berhasil (INA226 terinit ulang, ID cocok)
  uint32_t late = 0;              // selang cuplik > 1,5 x nominal (2,2 ms)
  uint32_t missed_est = 0;        // perkiraan konversi terlewat dari selang yang terlalu panjang
  uint32_t dt_min = 0xFFFFFFFFu;  // selang antar cuplikan (siklus)
  uint32_t dt_max = 0;
  uint64_t dt_sum = 0;
  uint32_t elapsed = 0;           // lama rekam sebenarnya (siklus)
  int32_t  sh_min = 32767, sh_max = -32768;
  int64_t  sh_sum = 0;
  uint16_t bus_min = 65535, bus_max = 0;
  uint64_t bus_sum = 0;
  L3Sample first[8];              // 8 cuplikan pertama untuk dilihat mata
  uint8_t  n_first = 0;
};

// Rekam selama duration_ms (maks ~59 detik pada 72 MHz). false bila parameter tak sah.
bool l3_run(INA226& ina, uint32_t duration_ms, L3Stats& st);
// ua_per_lsb = mikroampere per LSB register shunt (R=0,100 Ohm -> 25 uA).
void l3_print(const L3Stats& st, Print& out, uint32_t ua_per_lsb);
