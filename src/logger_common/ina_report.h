// ============================================================================
// Laporan teks INA226 (L2) dan pindai I2C - BEBAS dari jenis STM32. Keluaran ke objek Print apa
// pun (USB CDC di F103, USART2 di F030). Hanya bilangan bulat, tanpa printf float.
// ============================================================================
#pragma once
#include "ina226.h"

// Init (cfg 0x4127 + cal), baca balik ID, tunggu satu siklus konversi, baca shunt/bus/daya/arus
// mentah dan cetak beserta konversinya. true bila init berhasil.
bool ina_report_single(INA226& ina, uint16_t cal, uint32_t ua_per_lsb, Print& out);

// Pindai alamat 0x08..0x77 di bus yang diberikan dan cetak yang menjawab.
void i2c_scan(TwoWire& w, Print& out);
