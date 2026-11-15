#include "l3_capture.h"

// Selang konversi INA226 nominal: (1,1 ms shunt + 1,1 ms bus) = 2,2 ms (~454 cuplikan/s).
static const uint32_t NOMINAL_US = 2200;

bool l3_run(INA226& ina, uint32_t duration_ms, L3Stats& st) {
  const uint32_t hz = plat_hz();
  const uint32_t cyc_per_ms = hz / 1000u;
  if (duration_ms == 0 || duration_ms > 0xFFFFFFFFu / cyc_per_ms) return false;
  st = L3Stats();

  const uint32_t dur_cycles = duration_ms * cyc_per_ms;
  const uint32_t cyc_per_us = hz / 1000000u;
  const uint32_t late_thr   = (NOMINAL_US * 3u / 2u) * cyc_per_us;   // 1,5 x nominal
  const uint32_t nom_cyc    = NOMINAL_US * cyc_per_us;

  bool r = false;
  ina.ready(r);                                   // buang flag lama (membaca MASK menghapusnya)

  const uint32_t t0 = plat_cycles();
  uint32_t t_prev = 0;
  bool have_prev = false;
  while ((uint32_t)(plat_cycles() - t0) < dur_cycles) {
    const uint32_t ts = plat_cycles();            // awal pembacaan MASK yang akan menemukan flag
    if (!ina.ready(r)) { st.i2c_err++; continue; }
    st.polls++;
    if (!r) continue;
    int16_t sh; uint16_t bus;
    if (!ina.readShuntBus(sh, bus)) { st.i2c_err++; continue; }

    const uint32_t t_rel = ts - t0;
    if (have_prev) {
      const uint32_t dt = t_rel - t_prev;
      if (dt < st.dt_min) st.dt_min = dt;
      if (dt > st.dt_max) st.dt_max = dt;
      st.dt_sum += dt;
      if (dt > late_thr) {
        st.late++;
        st.missed_est += (dt + nom_cyc / 2) / nom_cyc - 1;   // berapa konversi kira-kira terlewat
      }
    }
    t_prev = t_rel;
    have_prev = true;

    st.n++;
    if (sh < st.sh_min) st.sh_min = sh;
    if (sh > st.sh_max) st.sh_max = sh;
    st.sh_sum += sh;
    if (bus < st.bus_min) st.bus_min = bus;
    if (bus > st.bus_max) st.bus_max = bus;
    st.bus_sum += bus;
    if (st.n_first < 8) st.first[st.n_first++] = { t_rel, sh, bus };
  }
  st.elapsed = plat_cycles() - t0;
  return true;
}

// Bilangan bulat saja (tanpa printf float).
void l3_print(const L3Stats& st, Print& out, uint32_t ua_per_lsb) {
  const uint64_t hz = plat_hz();
  out.printf("L3: n=%lu polls=%lu i2c_err=%lu elapsed=%lu ms\r\n",
             (unsigned long)st.n, (unsigned long)st.polls, (unsigned long)st.i2c_err,
             (unsigned long)((uint64_t)st.elapsed * 1000u / hz));
  if (st.polls > 0) {
    out.printf("periode polling rata2 = %lu us\r\n",
               (unsigned long)((uint64_t)st.elapsed * 1000000u / hz / st.polls));
  }
  if (st.n < 2) { out.print(F("cuplikan < 2, tidak ada statistik selang\r\n")); return; }
  const uint64_t pairs = st.n - 1;
  const uint32_t dt_min_us10 = (uint32_t)((uint64_t)st.dt_min * 10000000u / hz);   // 0,1 us
  const uint32_t dt_max_us10 = (uint32_t)((uint64_t)st.dt_max * 10000000u / hz);
  const uint32_t dt_avg_us10 = (uint32_t)(st.dt_sum * 10000000u / hz / pairs);
  const uint32_t rate_x100   = (uint32_t)((uint64_t)pairs * hz * 100u / st.dt_sum);
  out.printf("selang cuplik: min=%lu.%lu us  rata2=%lu.%lu us  maks=%lu.%lu us\r\n",
             (unsigned long)(dt_min_us10 / 10), (unsigned long)(dt_min_us10 % 10),
             (unsigned long)(dt_avg_us10 / 10), (unsigned long)(dt_avg_us10 % 10),
             (unsigned long)(dt_max_us10 / 10), (unsigned long)(dt_max_us10 % 10));
  out.printf("laju = %lu.%02lu cuplikan/s (nominal ~454)  terlambat(>1,5x)=%lu  terlewat(perkiraan)=%lu\r\n",
             (unsigned long)(rate_x100 / 100), (unsigned long)(rate_x100 % 100),
             (unsigned long)st.late, (unsigned long)st.missed_est);
  const long sh_mean = (long)(st.sh_sum / (int64_t)st.n);
  out.printf("shunt raw min=%ld rata2=%ld maks=%ld  (arus %ld .. %ld uA, rata2 %ld uA)\r\n",
             (long)st.sh_min, sh_mean, (long)st.sh_max,
             (long)st.sh_min * (long)ua_per_lsb, (long)st.sh_max * (long)ua_per_lsb,
             sh_mean * (long)ua_per_lsb);
  const unsigned long bus_mean = (unsigned long)(st.bus_sum / st.n);
  out.printf("bus raw min=%u rata2=%lu maks=%u  (%lu .. %lu mV, rata2 %lu mV)\r\n",
             (unsigned)st.bus_min, bus_mean, (unsigned)st.bus_max,
             (unsigned long)st.bus_min * 5 / 4, (unsigned long)st.bus_max * 5 / 4, bus_mean * 5 / 4);
  for (uint8_t i = 0; i < st.n_first; i++) {
    const uint32_t us = (uint32_t)((uint64_t)st.first[i].t_rel * 1000000u / hz);
    out.printf("  #%u t=%lu us shunt=%d bus=%u\r\n", (unsigned)i, (unsigned long)us,
               (int)st.first[i].shunt, (unsigned)st.first[i].bus);
  }
}
