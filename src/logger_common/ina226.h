// ============================================================================
// Driver INA226 (I2C) - BEBAS dari jenis STM32 (dipakai logger F030 dan F103).
// Hanya bergantung pada Arduino Wire. Satu objek per perangkat; alamat dan bus diberikan pemanggil.
// Register 16-bit big-endian; baca = tulis pointer, repeated start, baca 2 byte.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <Wire.h>

class INA226 {
 public:
  enum : uint8_t {
    REG_CFG = 0x00, REG_SHUNT = 0x01, REG_BUS = 0x02, REG_PWR = 0x03, REG_CUR = 0x04,
    REG_CAL = 0x05, REG_MASK = 0x06, REG_MFG = 0xFE, REG_DIE = 0xFF
  };
  // AVG=1, VBUSCT=VSHCT=1,1 ms, mode kontinu shunt+bus (sesuai paper: tanpa averaging, ~450 cuplikan/s).
  static const uint16_t CFG_CONT_1P1MS = 0x4127;
  static const uint16_t MASK_CVRF      = 1u << 3;   // conversion-ready flag; dihapus oleh pembacaan MASK

  INA226(TwoWire& wire, uint8_t addr) : w_(wire), addr_(addr) {}

  bool read(uint8_t reg, uint16_t& v);
  bool write(uint8_t reg, uint16_t v);
  // Tulis cfg+cal, baca balik, cek ID (0x5449 / 0x2260). true bila semuanya cocok.
  bool init(uint16_t cfg, uint16_t cal);
  // r = true bila satu konversi baru selesai sejak pembacaan MASK terakhir (membaca MASK menghapus flag).
  bool ready(bool& r);
  bool readShuntBus(int16_t& shunt, uint16_t& bus);
  // A1 (15 Nov): pulihkan bus I2C yang macet lalu init ulang dengan cfg/cal terakhir.
  // Wire.end() = HAL DeInit + reset periferal lewat RCC (menghapus BUSY yang tersangkut, errata F1);
  // Wire.begin() = recoverBus() (9 detak SCL + STOP bila SDA ditahan slave) + init ulang 100 kHz,
  // lalu clock dikembalikan ke i2cHz. Bebas dari jenis STM32 (hanya API Wire). true bila init lulus.
  bool recover();
  uint32_t i2cHz = 400000;       // clock I2C yang dipasang ulang oleh recover() (samakan dengan setup)
  uint32_t recoverCount = 0;     // berapa kali recover() dipanggil sejak boot

  bool     ok() const { return ok_; }
  uint8_t  lastErr = 0;          // kode terakhir Wire.endTransmission (0x80 = requestFrom kurang byte)
  uint16_t mfg = 0, die = 0, cfgRb = 0, calRb = 0;

 private:
  TwoWire& w_;
  uint8_t  addr_;
  bool     ok_ = false;
  uint16_t cfg_ = CFG_CONT_1P1MS, cal_ = 0;   // nilai init terakhir, dipakai recover()
};
