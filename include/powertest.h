// Alat ukur DAYA & SLEEP bersama untuk kelompok K1/K3 (software) dan K2/K4 (DS28E38),
// supaya kebijakan tidur (WFI) dan penghitungan %S_cpu IDENTIK di kedua arsitektur
// (variabel kontrol Bab 3.3.1 c.5). Sertakan di tepat satu file .cpp per firmware,
// SETELAH bench.h (memakai cyccnt_init()/DWT yang sama).
//
// Pin ukur (semua di port B; hubungkan ke osiloskop / logic analyzer / pencatat arus):
//   PB0  = TRIG    HIGH selama SATU operasi tanda tangan (sinkron profil arus & latensi DWT)
//   PB1  = SLEEP   HIGH selama CPU berada di dalam WFI (pembanding independen untuk %S_cpu)
//   PB10 = WINDOW  HIGH selama seluruh jendela pengamatan (penanda awal/akhir 60 detik)
// PB0/PB1/PB10 tidak bentrok dengan USB (PA11/12), SWD (PA13/14), OneWire (PA15),
// LED (PB5/PB6/PC13) maupun ADC (PA0) pada firmware lain di proyek ini.
#pragma once
#include <Arduino.h>

#define PT_PORT        GPIOB
#define PT_BIT_TRIG    0
#define PT_BIT_SLEEP   1
#define PT_BIT_WINDOW  10

static inline void pt_set(uint32_t bit) { PT_PORT->BSRR = (1UL << bit); }
static inline void pt_clr(uint32_t bit) { PT_PORT->BSRR = (1UL << (bit + 16)); }

static inline void pt_pins_init() {
  pinMode(PB0, OUTPUT); pinMode(PB1, OUTPUT); pinMode(PB10, OUTPUT);   // sekaligus menyalakan clock GPIOB
  pt_clr(PT_BIT_TRIG); pt_clr(PT_BIT_SLEEP); pt_clr(PT_BIT_WINDOW);
}

// ---------- Jam siklus 64-bit (CYCCNT 32-bit melimpah tiap 2^32/f detik = 89 s @ 48 MHz) ----------
static uint32_t pt_last_cyc = 0;
static uint64_t pt_hi_cyc   = 0;

// Harus dipanggil minimal sekali per < 89 s; semua loop di bawah memanggilnya tiap <= 1 ms.
static inline uint64_t pt_cyc64() {
  uint32_t n = DWT->CYCCNT;
  if (n < pt_last_cyc) pt_hi_cyc += (1ULL << 32);
  pt_last_cyc = n;
  return pt_hi_cyc | n;
}

// ---------- Kebijakan tidur: SATU-SATUNYA tempat WFI dipanggil ----------
// Interupsi di-mask (PRIMASK) saat memeriksa `pending()` dan saat WFI: interupsi yang masuk
// di antara pemeriksaan dan WFI membuat WFI langsung kembali (tidak ada tidur terlewat),
// dan waktu ISR (SysTick/USB) setelah bangun TIDAK dihitung sebagai tidur karena CYCCNT
// dibaca sebelum interupsi di-unmask.
static uint64_t pt_sleep_cyc = 0;   // total siklus di dalam WFI sejak pt_win_begin()
static uint32_t pt_wfi_count = 0;   // berapa kali WFI dimasuki

typedef bool (*pt_pending_fn)();    // true = ada pekerjaan -> jangan tidur

static inline void pt_idle(pt_pending_fn pending) {
  __disable_irq();
  if (!pending()) {
    pt_set(PT_BIT_SLEEP);
    uint32_t t0 = DWT->CYCCNT;
    __WFI();
    uint32_t t1 = DWT->CYCCNT;
    pt_clr(PT_BIT_SLEEP);
    pt_sleep_cyc += (uint32_t)(t1 - t0);
    pt_wfi_count++;
  }
  __enable_irq();
  pt_cyc64();                        // jaga ekstensi 64-bit tetap mutakhir
}

// ---------- Jendela pengamatan ----------
typedef struct {
  uint64_t total_cyc;   // panjang jendela, siklus DWT
  uint64_t sleep_cyc;   // siklus di dalam WFI
  uint32_t wfi_count;
  uint32_t ms_millis;   // panjang jendela menurut millis() (pembanding independen)
} PtWindow;

static uint64_t pt_win_c0; static uint32_t pt_win_ms0;

static inline void pt_win_begin() {
  pt_sleep_cyc = 0; pt_wfi_count = 0;
  pt_win_ms0 = millis();
  pt_win_c0  = pt_cyc64();
  pt_set(PT_BIT_WINDOW);
}

static inline PtWindow pt_win_end() {
  PtWindow w;
  uint64_t c1 = pt_cyc64();
  pt_clr(PT_BIT_WINDOW);
  w.total_cyc  = c1 - pt_win_c0;
  w.sleep_cyc  = pt_sleep_cyc;
  w.wfi_count  = pt_wfi_count;
  w.ms_millis  = millis() - pt_win_ms0;
  return w;
}

// %S_cpu dalam satuan 0,001 % (mis. 12345 = 12,345 %). Rumus Bab 3.5.1 butir 3.
static inline uint32_t pt_sleep_milli_pct(const PtWindow &w) {
  return w.total_cyc ? (uint32_t)(w.sleep_cyc * 100000ULL / w.total_cyc) : 0;
}
