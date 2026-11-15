#include "ina226.h"

bool INA226::read(uint8_t reg, uint16_t& v) {
  w_.beginTransmission(addr_);
  w_.write(reg);
  uint8_t e = w_.endTransmission(false);               // repeated start
  if (e != 0) { lastErr = e; return false; }
  if (w_.requestFrom(addr_, (uint8_t)2) != 2) { lastErr = 0x80; return false; }
  uint16_t hi = (uint8_t)w_.read();
  uint16_t lo = (uint8_t)w_.read();
  v = (uint16_t)((hi << 8) | lo);
  lastErr = 0;
  return true;
}

bool INA226::write(uint8_t reg, uint16_t v) {
  w_.beginTransmission(addr_);
  w_.write(reg);
  w_.write((uint8_t)(v >> 8));
  w_.write((uint8_t)(v & 0xFF));
  uint8_t e = w_.endTransmission();
  lastErr = e;
  return e == 0;
}

bool INA226::init(uint16_t cfg, uint16_t cal) {
  ok_ = false;
  uint16_t m = 0, d = 0, c = 0, k = 0;
  if (!read(REG_MFG, m) || !read(REG_DIE, d)) return false;
  mfg = m;
  die = d;
  if (!write(REG_CFG, cfg) || !write(REG_CAL, cal)) return false;
  if (!read(REG_CFG, c) || !read(REG_CAL, k)) return false;
  cfgRb = c;
  calRb = k;
  ok_ = (m == 0x5449 && d == 0x2260 && c == cfg && k == cal);
  return ok_;
}

bool INA226::ready(bool& r) {
  uint16_t m;
  if (!read(REG_MASK, m)) return false;
  r = (m & MASK_CVRF) != 0;
  return true;
}

bool INA226::readShuntBus(int16_t& shunt, uint16_t& bus) {
  uint16_t s, b;
  if (!read(REG_SHUNT, s) || !read(REG_BUS, b)) return false;
  shunt = (int16_t)s;
  bus   = b;
  return true;
}
