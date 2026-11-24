#include "edge_stats.h"

void EdgeStats::feed(uint64_t t, bool rising) {
  if (n_first < 8) first[n_first++] = { t, (uint8_t)rising };
  if (last == (rising ? 1 : 0)) seq_err++;           // dua tepi sejenis berturut-turut
  if (rising) {
    n_rise++;
    if (last == 0) low.add(t - t_fall);              // turun -> naik
    if (n_rise > 1) period.add(t - t_rise);          // naik -> naik (periode)
    t_rise = t;
    last = 1;
  } else {
    n_fall++;
    if (last == 1) high.add(t - t_rise);             // naik -> turun
    t_fall = t;
    last = 0;
  }
}

// ticks -> nanodetik (64-bit aman untuk ticks < ~1,8e10 pada 72 MHz, ~250 s)
static uint64_t ticks_ns(uint64_t ticks, uint32_t hz) { return ticks * 1000000000ull / hz; }

static void print_ns(Print& out, uint64_t ns) {
  out.printf("%lu.%03lu", (unsigned long)(ns / 1000u), (unsigned long)(ns % 1000u));
}

static void print_acc(Print& out, const char* name, const EdgeAcc& a, uint32_t hz) {
  out.printf("%s n=%lu", name, (unsigned long)a.n);
  if (a.n == 0) { out.print(F("\r\n")); return; }
  out.print(F(" min=")); print_ns(out, ticks_ns(a.mn, hz));
  out.print(F(" rata2=")); print_ns(out, ticks_ns(a.sum / a.n, hz));
  out.print(F(" maks=")); print_ns(out, ticks_ns(a.mx, hz));
  out.print(F(" us\r\n"));
}

void EdgeStats::print(Print& out, uint32_t hz) const {
  out.printf("L4: tepi naik=%lu turun=%lu salah_urut=%lu (tik=%lu Hz)\r\n",
             (unsigned long)n_rise, (unsigned long)n_fall, (unsigned long)seq_err, (unsigned long)hz);
  print_acc(out, "HIGH   ", high, hz);
  print_acc(out, "LOW    ", low, hz);
  print_acc(out, "periode", period, hz);
  if (high.n > 0 && low.n > 0) {
    // duty = rata2 HIGH / (rata2 HIGH + rata2 LOW). Jumlah HIGH dan LOW bisa beda 1 (jendela mulai/berakhir
    // di tengah periode), jadi JANGAN pakai high.sum/(high.sum+low.sum): 15 Nov itu memberi 30,04% untuk 30,00%.
    // Bentuk silang tanpa pembagian antara: aman 64-bit untuk jendela puluhan detik.
    const uint64_t h = high.sum * low.n, l = low.sum * high.n;
    const uint32_t duty_x100 = (uint32_t)(h * 10000u / (h + l));         // duty x100 (persen, 2 desimal)
    out.printf("duty = %lu.%02lu %%", (unsigned long)(duty_x100 / 100), (unsigned long)(duty_x100 % 100));
    if (period.n > 0) {
      const uint32_t f_x1000 = (uint32_t)((uint64_t)hz * 1000u / (period.sum / period.n));
      out.printf("  frekuensi rata2 = %lu.%03lu Hz", (unsigned long)(f_x1000 / 1000), (unsigned long)(f_x1000 % 1000));
    }
    out.print(F("\r\n"));
  }
  for (uint8_t i = 0; i < n_first; i++) {
    const uint64_t ns = ticks_ns(first[i].t, hz);
    out.printf("  #%u %s t=", (unsigned)i, first[i].rising ? "naik " : "turun");
    print_ns(out, ns);
    out.print(F(" us\r\n"));
  }
}
