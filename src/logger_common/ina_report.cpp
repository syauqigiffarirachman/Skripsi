#include "ina_report.h"

bool ina_report_single(INA226& ina, uint16_t cal, uint32_t ua_per_lsb, Print& out) {
  const bool init_ok = ina.init(INA226::CFG_CONT_1P1MS, cal);
  out.printf("INA226 ok=%u init=%u err=%u mfg=%04X die=%04X cfg=%04X cal=%04X\r\n",
             (unsigned)ina.ok(), (unsigned)init_ok, (unsigned)ina.lastErr,
             (unsigned)ina.mfg, (unsigned)ina.die, (unsigned)ina.cfgRb, (unsigned)ina.calRb);
  if (!init_ok) { out.print(F("init gagal, tidak ada pembacaan (coba pindai bus)\r\n")); return false; }
  delay(5);                                        // >= satu siklus konversi (2 x 1,1 ms)
  int16_t sh; uint16_t bus, pwr = 0, cur = 0;
  if (!ina.readShuntBus(sh, bus) || !ina.read(INA226::REG_PWR, pwr) || !ina.read(INA226::REG_CUR, cur)) {
    out.printf("baca gagal err=%u\r\n", (unsigned)ina.lastErr);
    return true;                                   // init sendiri berhasil
  }
  const long shunt_uv = (long)sh * 5 / 2;                         // 2,5 uV/LSB
  const long bus_mv   = (long)((unsigned long)bus * 5 / 4);       // 1,25 mV/LSB
  out.printf("shunt raw=%d (%ld uV)  bus raw=%u (%ld mV)\r\n", (int)sh, shunt_uv, (unsigned)bus, bus_mv);
  out.printf("arus raw=%d (%ld uA)  daya raw=%u (%ld uW)  arus dari shunt=%ld uA\r\n",
             (int)(int16_t)cur, (long)(int16_t)cur * (long)ua_per_lsb,
             (unsigned)pwr, (long)pwr * 625L, (long)sh * (long)ua_per_lsb);
  return true;
}

void i2c_scan(TwoWire& w, Print& out) {
  out.print(F("pindai I2C:"));
  uint8_t n = 0;
  for (uint8_t a = 0x08; a < 0x78; a++) {
    w.beginTransmission(a);
    if (w.endTransmission() == 0) { out.printf(" 0x%02X", (unsigned)a); n++; }
  }
  out.printf("  (%u perangkat)\r\n", (unsigned)n);
}
